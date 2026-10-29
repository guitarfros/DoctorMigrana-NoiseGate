// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros
// Tests do núcleo DSP (sen JUCE). Executar con: ctest --test-dir build --output-on-failure
#include "NoiseGate.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

static int failures = 0;

#define CHECK(cond, msg)                                                          \
    do {                                                                          \
        if (! (cond)) { std::printf ("FALLO: %s  [%s:%d]\n", msg, __FILE__, __LINE__); ++failures; } \
        else          { std::printf ("ok:    %s\n", msg); }                        \
    } while (0)

static constexpr double fs = 48000.0;

static std::vector<float> sine (double freq, double ampDb, int n)
{
    std::vector<float> v ((size_t) n);
    const double a = std::pow (10.0, ampDb / 20.0);
    for (int i = 0; i < n; ++i)
        v[(size_t) i] = (float) (a * std::sin (2.0 * M_PI * freq * i / fs));
    return v;
}

static std::vector<float> noise (double ampDb, int n, unsigned seed = 1)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);
    std::vector<float> v ((size_t) n);
    const float a = (float) std::pow (10.0, ampDb / 20.0);
    for (auto& s : v) s = a * d (rng);
    return v;
}

static double rmsDb (const std::vector<float>& v, int from, int to)
{
    double sum = 0.0;
    for (int i = from; i < to; ++i) sum += (double) v[(size_t) i] * v[(size_t) i];
    return 10.0 * std::log10 (sum / (to - from) + 1.0e-20);
}

static void run (calma::NoiseGate& g, std::vector<float>& x)
{
    float* ch[1] = { x.data() };
    g.process (ch, 1, (int) x.size());
}

int main()
{
    calma::GateSettings s;  // thr -55, decay 120, hold 25, depth 80

    {   // 1) Un tono forte pasa case sen cambios (unity) despois do ataque
        calma::NoiseGate g; g.prepare (fs); g.setSettings (s);
        auto x = sine (220.0, -20.0, (int) fs);
        const double inDb = rmsDb (x, 24000, 48000);
        run (g, x);
        const double outDb = rmsDb (x, 24000, 48000);
        CHECK (std::abs (outDb - inDb) < 0.3, "tono a -20 dB pasa sen alterar o nivel (<0.3 dB)");
    }

    {   // 2) Ruído por debaixo do threshold queda moi atenuado
        calma::NoiseGate g; g.prepare (fs); g.setSettings (s);
        auto x = noise (-70.0, (int) fs);
        const double inDb = rmsDb (x, 24000, 48000);
        run (g, x);
        const double outDb = rmsDb (x, 24000, 48000);
        CHECK (inDb - outDb > 40.0, "ruído a -70 dB atenuado en máis de 40 dB");
    }

    {   // 3) Tono -> silencio con ruído: pecha tras hold+decay, e abre ao volver o tono
        calma::NoiseGate g; g.prepare (fs); g.setSettings (s);
        const int seg = (int) fs;                        // 1 s por segmento
        auto a = sine (220.0, -20.0, seg);
        auto b = noise (-75.0, seg, 7);
        auto c = sine (220.0, -20.0, seg);
        std::vector<float> x; x.insert (x.end(), a.begin(), a.end());
        x.insert (x.end(), b.begin(), b.end()); x.insert (x.end(), c.begin(), c.end());
        auto ref = x;
        run (g, x);

        const double noiseIn  = rmsDb (ref, seg + 36000, 2 * seg);
        const double noiseOut = rmsDb (x,   seg + 36000, 2 * seg);
        CHECK (noiseIn - noiseOut > 40.0, "ruído tras o tono queda cortado (>40 dB)");

        const double toneIn  = rmsDb (ref, 2 * seg + 4800, 3 * seg);
        const double toneOut = rmsDb (x,   2 * seg + 4800, 3 * seg);
        CHECK (std::abs (toneOut - toneIn) < 0.3, "o tono volve pasar ao reabrir");
    }

    {   // 4) O hold mantén aberto: ao cortar o tono, a saída segue viva durante o hold
        calma::GateSettings h = s; h.holdMs = 100.0f; h.decayMs = 50.0f;
        calma::NoiseGate g; g.prepare (fs); g.setSettings (h);
        auto a = sine (220.0, -20.0, 24000);
        a.insert (a.end(), 2400, 0.0f);                       // 50 ms de silencio (< hold de 100 ms)
        run (g, a);
        CHECK (g.getCurrentGain() > 0.9f, "durante o hold (50 ms de 100) o gate segue aberto");

        std::vector<float> tail (12000, 0.0f);                // 250 ms máis: hold esgotado + decay
        run (g, tail);
        CHECK (g.getCurrentGain() < 0.05f, "pasado o hold e o decay o gate quedou pechado");
    }

    {   // 5) Bypass: sen gate, o ruído pasa case igual
        calma::GateSettings off = s; off.active = false;
        calma::NoiseGate g; g.prepare (fs); g.setSettings (off);
        auto x = noise (-70.0, (int) fs);
        const double inDb = rmsDb (x, 24000, 48000);
        run (g, x);
        const double outDb = rmsDb (x, 24000, 48000);
        CHECK (std::abs (outDb - inDb) < 0.3, "bypass: o ruído pasa sen alterar");
    }

    {   // 6) Latencia: un impulso sae exactamente `lookahead` mostras despois
        calma::GateSettings off = s; off.active = false;
        calma::NoiseGate g; g.prepare (fs); g.setSettings (off);
        std::vector<float> x (1000, 0.0f); x[100] = 1.0f;
        run (g, x);
        int peakIdx = 0; float peak = 0.0f;
        for (int i = 0; i < (int) x.size(); ++i)
            if (std::abs (x[(size_t) i]) > peak) { peak = std::abs (x[(size_t) i]); peakIdx = i; }
        CHECK (peakIdx == 100 + g.getLatencySamples(), "a latencia declarada coincide coa real");
    }

    {   // 7) Estéreo ligado e sen NaN/Inf con valores extremos
        calma::NoiseGate g; g.prepare (44100.0); g.setSettings (s);
        std::vector<float> l (4410, 1.0f), r (4410, 0.0f);
        float* ch[2] = { l.data(), r.data() };
        g.process (ch, 2, 4410);
        bool ok = true;
        for (float v : l) ok = ok && std::isfinite (v);
        for (float v : r) ok = ok && std::isfinite (v);
        CHECK (ok, "sen NaN/Inf con DC a fondo de escala en estéreo");
    }

    std::printf ("\n%s (%d fallos)\n", failures == 0 ? "TODO OK" : "HAI FALLOS", failures);
    return failures == 0 ? 0 : 1;
}

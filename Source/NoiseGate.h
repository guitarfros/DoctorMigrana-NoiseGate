// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros
#pragma once

// Núcleo DSP do noise gate. C++ puro (sen JUCE) para poder probalo con tests.
//
// Deseño: expander descendente de joelho suave con detector de pico, hold,
// release (decay) axustable e un pequeno lookahead para non "comer" o ataque da
// púa. Non é unha copia de ningún pedal comercial: está inspirado no obxectivo
// de pedais como o Boss NS-2 (deixar pasar o ataque e a cola natural da nota e
// quitar só o ruído), pero o algoritmo é propio.

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace calma
{

inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g)  { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }

struct GateSettings
{
    float thresholdDb = -55.0f;  // nivel por debaixo do cal o gate empeza a pechar
    float decayMs     = 120.0f;  // tempo aprox. en caer ~60 dB cando pecha
    float holdMs      = 25.0f;   // tempo que aguanta aberto despois de caer o nivel
    float depthDb     = 80.0f;   // atenuación máxima (80 = practicamente mute)
    bool  active      = true;    // false = bypass (o gate abre de todo)
};

class NoiseGate
{
public:
    static constexpr int   maxChannels = 2;
    static constexpr float lookaheadMs = 1.0f;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        lookahead  = (int) std::lround (sampleRate * lookaheadMs * 0.001);
        delayLen   = lookahead + 1;

        for (auto& d : delay)
            d.assign ((size_t) delayLen, 0.0f);

        envAttackCoef  = timeToCoef (0.00005);   // 0.05 ms
        envReleaseCoef = timeToCoef (0.005);     // 5 ms
        gainAttackCoef = timeToCoef (0.0004);    // 0.4 ms
        updateCoefs();
        reset();
    }

    int getLatencySamples() const { return lookahead; }

    void reset()
    {
        env = heldEnv = 0.0f;
        holdCounter = 0;
        gain = 1.0f;
        writePos = 0;
        for (auto& d : delay)
            std::fill (d.begin(), d.end(), 0.0f);
    }

    void setSettings (const GateSettings& s)
    {
        settings = s;
        updateCoefs();
    }

    float getCurrentGain() const { return gain; }

    // Procesa `numChannels` (1 ou 2) canles in-place. O detector ve a entrada
    // sen retardo; o audio sae retardado `lookahead` mostras.
    void process (float* const* channels, int numChannels, int numSamples)
    {
        numChannels = std::min (numChannels, maxChannels);

        for (int i = 0; i < numSamples; ++i)
        {
            float detector = 0.0f;
            for (int c = 0; c < numChannels; ++c)
                detector = std::max (detector, std::abs (channels[c][i]));

            const float g = computeGain (detector);

            for (int c = 0; c < numChannels; ++c)
            {
                auto& d = delay[(size_t) c];
                d[(size_t) writePos] = channels[c][i];

                int readPos = writePos - lookahead;
                if (readPos < 0)
                    readPos += delayLen;

                channels[c][i] = d[(size_t) readPos] * g;
            }

            if (++writePos >= delayLen)
                writePos = 0;
        }
    }

private:
    static constexpr float ratioMinusOne = 9.0f;  // expansor 10:1
    static constexpr float kneeDb        = 6.0f;

    double timeToCoef (double seconds) const
    {
        return std::exp (-1.0 / (seconds * sampleRate));
    }

    void updateCoefs()
    {
        // decayMs = tempo aprox. para caer 60 dB; ln(1000) ~ 6.9 constantes de tempo
        const double tau = std::max (0.001, (double) settings.decayMs * 0.001 / 6.908);
        gainReleaseCoef = timeToCoef (tau);
        holdSamples = (int) std::lround (settings.holdMs * 0.001 * sampleRate);
    }

    float computeGain (float detectorAbs)
    {
        // 1) Envolvente de pico
        const double aC = envAttackCoef, rC = envReleaseCoef;
        env = detectorAbs > env ? (float) (aC * env + (1.0 - aC) * detectorAbs)
                                : (float) (rC * env + (1.0 - rC) * detectorAbs);
        if (env < 1.0e-9f)
            env = 0.0f;

        // 2) Hold: mentres o nivel sobe ou acaba de baixar, mantén a envolvente
        if (env >= heldEnv)        { heldEnv = env; holdCounter = holdSamples; }
        else if (holdCounter > 0)  { --holdCounter; }
        else                       { heldEnv = env; }

        // 3) Curva de expansor descendente con joelho suave por debaixo do threshold
        float targetGain = 1.0f;
        if (settings.active)
        {
            const float u = settings.thresholdDb - gainToDb (heldEnv);   // dB por debaixo do threshold
            float attenDb = 0.0f;

            if (u > 0.0f)
                attenDb = u < kneeDb ? ratioMinusOne * u * u / (2.0f * kneeDb)
                                     : ratioMinusOne * (u - 0.5f * kneeDb);

            attenDb    = std::min (attenDb, settings.depthDb);
            targetGain = dbToGain (-attenDb);
        }

        // 4) Suavizado da ganancia: abre rápido, pecha co decay
        const double c = targetGain > gain ? gainAttackCoef : gainReleaseCoef;
        gain = (float) (c * gain + (1.0 - c) * targetGain);
        return gain;
    }

    GateSettings settings;
    double sampleRate = 44100.0;

    double envAttackCoef = 0.0, envReleaseCoef = 0.0;
    double gainAttackCoef = 0.0, gainReleaseCoef = 0.0;

    float env = 0.0f, heldEnv = 0.0f, gain = 1.0f;
    int   holdSamples = 0, holdCounter = 0;

    std::array<std::vector<float>, maxChannels> delay;
    int lookahead = 0, delayLen = 1, writePos = 0;
};

} // namespace calma

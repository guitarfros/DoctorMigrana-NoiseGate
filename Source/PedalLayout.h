// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros
#pragma once

// Posicións dos elementos sobre a imaxe do pedal (assets/pedal.jpg).
// Todo está en coordenadas NORMALIZADAS (0..1) respecto ao ancho/alto da imaxe,
// así que funciona a calquera tamaño. Se cambias a imaxe, retoca só isto.

namespace layout
{
    // Proporción da imaxe orixinal (1280 x 1984)
    constexpr float imageWidth  = 1280.0f;
    constexpr float imageHeight = 1984.0f;

    // Knob de Threshold (na madeira, enriba do rótulo "THRESHOLD")
    constexpr float knobCx   = 0.500f;   // centro X (fracción do ancho)
    constexpr float knobCy   = 0.145f;   // centro Y (fracción do alto)
    constexpr float knobSize = 0.300f;   // lado do slider (fracción do ancho); o knob ocupa ~72%

    // LED vermello (enriba do footswitch)
    constexpr float ledCx = 0.492f;
    constexpr float ledCy = 0.7576f;
    constexpr float ledRadius = 0.0125f; // fracción do ancho

    // Footswitch (zona clicable circular)
    constexpr float footCx = 0.4945f;
    constexpr float footCy = 0.839f;
    constexpr float footRadius = 0.095f; // fracción do ancho

    // Placa de latón (dobre clic para abrir os axustes avanzados)
    constexpr float plateX = 0.168f, plateY = 0.287f, plateW = 0.656f, plateH = 0.454f;
}

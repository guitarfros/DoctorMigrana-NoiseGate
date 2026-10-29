// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros
#include "PluginEditor.h"
#include "PedalLayout.h"

#if CALMA_HAS_PEDAL_IMAGE
 #include <BinaryData.h>
#endif

namespace
{
    const juce::Colour brassLight { 0xffe3c36f };
    const juce::Colour brassMid   { 0xffb98d3a };
    const juce::Colour brassDark  { 0xff6e4f17 };
}

//==============================================================================
// Knob de plástico negro envellecido. Dúas capas en caché (reconstrúense só se
// cambia o tamaño): a base (sombra, saia, cúpula, marcas) e a capa que xira co
// knob (desgaste + marca branca). O brillo píntase por riba en cada repintado.
namespace knobdraw
{
    constexpr float kPi = juce::MathConstants<float>::pi;

    template <typename Fn>
    void conic (juce::Graphics& g, juce::Point<float> c, float rOut, float rIn, int n, Fn colourAt)
    {
        const float d = 2.0f * kPi / (float) n;
        for (int i = 0; i < n; ++i)
        {
            const float a0 = (float) i * d;
            juce::Path p;
            p.addPieSegment (c.x - rOut, c.y - rOut, rOut * 2, rOut * 2, a0 - d * 0.1f, a0 + d * 1.1f, rIn / rOut);
            g.setColour (colourAt (i, a0 + d * 0.5f));
            g.fillPath (p);
        }
    }

    void drawBase (juce::Graphics& g, float size, float startAngle, float endAngle)
    {
        const juce::Point<float> c (size * 0.5f, size * 0.5f);
        const float r = size * 0.36f;

        // marcas arredor
        for (int i = 0; i < 11; ++i)
        {
            const float t = (float) i / 10.0f;
            const float a = startAngle + t * (endAngle - startAngle);
            const float s = std::sin (a), k = -std::cos (a);
            const bool major = (i == 0 || i == 10 || i == 5);
            g.setColour (brassMid.withAlpha (major ? 0.95f : 0.6f));
            g.drawLine (c.x + s * r * 1.16f, c.y + k * r * 1.16f,
                        c.x + s * r * (major ? 1.30f : 1.27f), c.y + k * r * (major ? 1.30f : 1.27f),
                        major ? size * 0.012f : size * 0.008f);
        }

        // sombra
        juce::Path disc; disc.addEllipse (c.x - r, c.y - r, r * 2, r * 2);
        juce::DropShadow (juce::Colours::black.withAlpha (0.75f), (int) (r * 0.28f),
                          { (int) (r * 0.06f), (int) (r * 0.16f) }).drawForPath (g, disc);

        // saia con nervios
        conic (g, c, r, r * 0.80f, 120, [] (int i, float a) {
            const float l = 0.05f + 0.10f * std::pow (std::max (0.0f, std::cos (a - 0.9f)), 2.0f);
            return juce::Colour::fromHSV (0.0f, 0.0f, i % 2 ? l * 0.5f : l, 1.0f);
        });

        // bisel e cúpula
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4d4b49), c.x - r * 0.45f, c.y - r * 0.55f,
                                                 juce::Colour (0xff050505), c.x + r * 0.7f, c.y + r * 0.8f, false));
        g.fillEllipse (c.x - r * 0.80f, c.y - r * 0.80f, r * 1.6f, r * 1.6f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff353331), c.x - r * 0.35f, c.y - r * 0.45f,
                                                 juce::Colour (0xff0b0b0b), c.x + r * 0.6f, c.y + r * 0.7f, true));
        g.fillEllipse (c.x - r * 0.68f, c.y - r * 0.68f, r * 1.36f, r * 1.36f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawEllipse (c.x - r * 0.80f, c.y - r * 0.80f, r * 1.6f, r * 1.6f, r * 0.03f);
    }

    // Desgaste + marca. Debúxase co punteiro cara arriba; despois rótase.
    void drawRotating (juce::Graphics& g, float size)
    {
        const juce::Point<float> c (size * 0.5f, size * 0.5f);
        const float r = size * 0.36f;
        juce::Random rng (20261021);
        const float hair = juce::jmax (0.6f, size * 0.0028f);   // grosor dun rabuñón fino

        auto polar = [&] (float ang, float rad) { return juce::Point<float> (c.x + std::sin (ang) * rad, c.y - std::cos (ang) * rad); };

        // ---- roce nas nervaduras da saia (zonas máis lisas e claras) e mugre
        for (int i = 0; i < 9; ++i)
        {
            const float a0 = rng.nextFloat() * 2 * kPi, len = 0.18f + 0.45f * rng.nextFloat();
            juce::Path p; p.addPieSegment (c.x - r * 0.995f, c.y - r * 0.995f, r * 1.99f, r * 1.99f, a0, a0 + len, 0.82f / 0.995f);
            g.setColour (juce::Colour (0xff8c8984).withAlpha (0.10f + 0.12f * rng.nextFloat()));
            g.fillPath (p);
        }
        for (int i = 0; i < 160; ++i)
        {
            const auto p = polar (rng.nextFloat() * 2 * kPi, r * (0.82f + 0.17f * rng.nextFloat()));
            g.setColour (juce::Colour (0xff6b5a35).withAlpha (0.10f + 0.20f * rng.nextFloat()));
            const float d = hair * (0.8f + 1.6f * rng.nextFloat());
            g.fillEllipse (p.x - d * 0.5f, p.y - d * 0.5f, d, d);
        }

        // ---- desgaste do bisel (pulido polos dedos)
        for (int i = 0; i < 12; ++i)
        {
            const float rr = r * (0.68f + 0.11f * rng.nextFloat()), a0 = rng.nextFloat() * 2 * kPi, len = 0.2f + 0.8f * rng.nextFloat();
            juce::Path p; p.addCentredArc (c.x, c.y, rr, rr, 0.0f, a0, a0 + len, true);
            g.setColour (juce::Colour (0xffa19d96).withAlpha (0.10f + 0.20f * rng.nextFloat()));
            g.strokePath (p, juce::PathStrokeType (hair * (1.5f + 2.0f * rng.nextFloat())));
        }

        // ---- rabuñaduras na cúpula
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path dome; dome.addEllipse (c.x - r * 0.68f, c.y - r * 0.68f, r * 1.36f, r * 1.36f);
            g.reduceClipRegion (dome);

            // remuíños circulares (marcas de uso)
            for (int i = 0; i < 8; ++i)
            {
                const float rr = r * (0.15f + 0.5f * rng.nextFloat()), a0 = rng.nextFloat() * 2 * kPi, len = 0.2f + 0.9f * rng.nextFloat();
                juce::Path p; p.addCentredArc (c.x, c.y, rr, rr, 0.0f, a0, a0 + len, true);
                g.setColour (juce::Colour (0xffb5b0a6).withAlpha (0.04f + 0.08f * rng.nextFloat()));
                g.strokePath (p, juce::PathStrokeType (hair * 0.9f));
            }
            // rabuñaduras finas
            for (int i = 0; i < 26; ++i)
            {
                const auto p0 = polar (rng.nextFloat() * 2 * kPi, r * 0.70f * std::sqrt (rng.nextFloat()));
                const float ang = rng.nextFloat() * 2 * kPi, len = r * (0.05f + 0.30f * std::pow (rng.nextFloat(), 3.0f));
                const juce::Point<float> p1 (p0.x + std::cos (ang) * len, p0.y + std::sin (ang) * len);
                const float curv = (rng.nextFloat() - 0.5f) * len * 0.35f;
                juce::Path p; p.startNewSubPath (p0);
                p.quadraticTo ((p0.x + p1.x) * 0.5f - std::sin (ang) * curv, (p0.y + p1.y) * 0.5f + std::cos (ang) * curv, p1.x, p1.y);
                g.setColour (juce::Colour (0xffc4bfb4).withAlpha (0.05f + 0.20f * rng.nextFloat()));
                g.strokePath (p, juce::PathStrokeType (hair * (0.5f + 0.7f * rng.nextFloat())));
            }
            // rabuñaduras profundas: liña clara con sombra escura ao lado
            for (int i = 0; i < 3; ++i)
            {
                const auto p0 = polar (rng.nextFloat() * 2 * kPi, r * 0.5f * std::sqrt (rng.nextFloat()));
                const float ang = rng.nextFloat() * 2 * kPi, len = r * (0.15f + 0.22f * rng.nextFloat());
                const juce::Point<float> p1 (p0.x + std::cos (ang) * len, p0.y + std::sin (ang) * len);
                g.setColour (juce::Colours::black.withAlpha (0.55f));
                g.drawLine (p0.x + hair, p0.y + hair, p1.x + hair, p1.y + hair, hair);
                g.setColour (juce::Colour (0xffd2cdc2).withAlpha (0.25f + 0.20f * rng.nextFloat()));
                g.drawLine (p0.x, p0.y, p1.x, p1.y, hair * 0.8f);
            }
            // pequenos golpes/descascarillados
            for (int i = 0; i < 9; ++i)
            {
                const auto p = polar (rng.nextFloat() * 2 * kPi, r * (0.30f + 0.38f * rng.nextFloat()));
                const float d = hair * (1.2f + 2.2f * rng.nextFloat());
                g.setColour (juce::Colour (0xff9a968e).withAlpha (0.30f + 0.30f * rng.nextFloat()));
                g.fillEllipse (p.x - d * 0.5f, p.y - d * 0.4f, d, d * 0.8f);
            }
        }

        // ---- marca branca envellecida (amarelada, con falos)
        const auto a = polar (0.0f, r * 0.22f), b = polar (0.0f, r * 0.84f);
        const float w = r * 0.10f;
        juce::Path line; line.startNewSubPath (a); line.lineTo (b);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.strokePath (line, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::translation (hair * 1.2f, hair * 1.2f));
        g.setColour (juce::Colour (0xffe3d8b9));
        g.strokePath (line, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // sombra de envellecemento ao longo da marca
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a6f3a).withAlpha (0.30f), a.x, a.y,
                                                 juce::Colour (0xff8a6f3a).withAlpha (0.0f), b.x, b.y, false));
        g.strokePath (line, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // falos de pintura (onde saltou)
        for (int i = 0; i < 11; ++i)
        {
            const float t = rng.nextFloat();
            const float x = a.x + (b.x - a.x) * t + (rng.nextFloat() - 0.5f) * w * 0.9f;
            const float y = a.y + (b.y - a.y) * t;
            const float d = hair * (0.5f + 1.0f * rng.nextFloat());
            g.setColour (juce::Colour (0xff141211).withAlpha (0.45f + 0.4f * rng.nextFloat()));
            g.fillEllipse (x - d * 0.5f, y - d * 0.5f, d, d * (0.8f + 0.8f * rng.nextFloat()));
        }
        // un rabuñón que cruza a marca
        g.setColour (juce::Colour (0xff141211).withAlpha (0.7f));
        g.drawLine (a.x - w * 0.9f, a.y + (b.y - a.y) * 0.55f - w * 0.4f, a.x + w * 0.9f, a.y + (b.y - a.y) * 0.55f + w * 0.5f, hair);
    }
}

void PedalLookAndFeel::rebuildLayers (int size, float scale)
{
    const int px = juce::jmax (8, juce::roundToInt ((float) size * scale));
    baseLayer = juce::Image (juce::Image::ARGB, px, px, true);
    rotLayer  = juce::Image (juce::Image::ARGB, px, px, true);
    {
        juce::Graphics g (baseLayer);
        g.addTransform (juce::AffineTransform::scale (scale));
        knobdraw::drawBase (g, (float) size, juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f);
    }
    {
        juce::Graphics g (rotLayer);
        g.addTransform (juce::AffineTransform::scale (scale));
        knobdraw::drawRotating (g, (float) size);
    }
    cachedSize = size;
    cachedScale = scale;
}

void PedalLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float startAngle, float endAngle,
                                         juce::Slider&)
{
    const int size = juce::jmin (width, height);
    if (size < 8)
        return;

    const float scale = juce::jlimit (1.0f, 3.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (size != cachedSize || std::abs (scale - cachedScale) > 0.01f)
        rebuildLayers (size, scale);

    const float ox = (float) x + (float) (width - size) * 0.5f;
    const float oy = (float) y + (float) (height - size) * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const auto toLocal = juce::AffineTransform::scale (1.0f / cachedScale).translated (ox, oy);

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (baseLayer, toLocal, false);
    g.drawImageTransformed (rotLayer, toLocal.followedBy (juce::AffineTransform::rotation (angle, ox + (float) size * 0.5f, oy + (float) size * 0.5f)), false);

    // brillo (queda fixo respecto da luz, non xira) e un velo de po
    const juce::Point<float> c (ox + (float) size * 0.5f, oy + (float) size * 0.5f);
    const float rd = (float) size * 0.36f * 0.68f;
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.20f), c.x - rd * 0.35f, c.y - rd * 0.55f,
                                             juce::Colours::white.withAlpha (0.0f), c.x + rd * 0.2f, c.y + rd * 0.2f, true));
    g.fillEllipse (c.x - rd, c.y - rd, rd * 2, rd * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, c.x, c.y,
                                             juce::Colour (0xffb8ad98).withAlpha (0.07f), c.x + rd, c.y, true));
    g.fillEllipse (c.x - rd, c.y - rd, rd * 2, rd * 2);
}

//==============================================================================
FootSwitch::FootSwitch() : juce::Button ("footswitch")
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

bool FootSwitch::hitTest (int x, int y)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    return c.getDistanceFrom ({ (float) x, (float) y }) <= (float) getWidth() * 0.5f;
}

void FootSwitch::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto area = getLocalBounds().toFloat().reduced ((float) getWidth() * 0.12f);

    if (isButtonDown)
    {
        g.setColour (juce::Colours::black.withAlpha (0.38f));
        g.fillEllipse (area);
    }
    else if (isMouseOver)
    {
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillEllipse (area);
    }
}

//==============================================================================
AdvancedPanel::AdvancedPanel (juce::AudioProcessorValueTreeState& apvts)
{
    setupRow (decay, "DECAY", ids::decay, " ms", apvts);
    setupRow (hold,  "HOLD",  ids::hold,  " ms", apvts);
    setupRow (depth, "DEPTH", ids::depth, " dB", apvts);

    // Os clics nas zonas baleiras pasan ao editor (para pechar con dobre clic)
    setInterceptsMouseClicks (false, true);
}

void AdvancedPanel::setupRow (Row& row, const juce::String& name, const juce::ParameterID& id,
                              const juce::String& suffix, juce::AudioProcessorValueTreeState& apvts)
{
    row.label.setText (name, juce::dontSendNotification);
    row.label.setJustificationType (juce::Justification::centredLeft);
    row.label.setColour (juce::Label::textColourId, brassLight);
    addAndMakeVisible (row.label);

    row.slider.setSliderStyle (juce::Slider::LinearHorizontal);
    row.slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 22);
    row.slider.setTextValueSuffix (suffix);
    row.slider.setColour (juce::Slider::trackColourId, brassMid);
    row.slider.setColour (juce::Slider::thumbColourId, brassLight);
    row.slider.setColour (juce::Slider::backgroundColourId, juce::Colours::black.withAlpha (0.5f));
    row.slider.setColour (juce::Slider::textBoxTextColourId, brassLight);
    row.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (row.slider);

    row.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id.getParamID(), row.slider);
}

void AdvancedPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float corner = b.getHeight() * 0.06f;

    g.setColour (juce::Colour (0xf8120e0a));
    g.fillRoundedRectangle (b, corner);
    g.setColour (brassMid);
    g.drawRoundedRectangle (b.reduced (1.5f), corner, 2.0f);

    g.setColour (brassMid.withAlpha (0.8f));
    g.setFont (juce::FontOptions (juce::jmax (9.0f, b.getHeight() * 0.055f)));
    g.drawText ("double-click the plate to close", getLocalBounds().removeFromBottom ((int) (b.getHeight() * 0.16f)),
                juce::Justification::centred);
}

void AdvancedPanel::resized()
{
    auto area = getLocalBounds().reduced (getWidth() / 14, getHeight() / 9);
    area.removeFromBottom (getHeight() / 8);
    const int rowH = area.getHeight() / 3;
    const auto fontH = juce::jmax (9.0f, (float) rowH * 0.28f);

    for (auto* row : { &decay, &hold, &depth })
    {
        auto r = area.removeFromTop (rowH);
        row->label.setFont (juce::FontOptions (fontH, juce::Font::bold));
        row->label.setMinimumHorizontalScale (0.8f);
        row->label.setBounds (r.removeFromLeft (juce::roundToInt ((float) r.getWidth() * 0.27f)));
        row->slider.setBounds (r);
    }
}

//==============================================================================
CalmaEditor::CalmaEditor (CalmaProcessor& p)
    : AudioProcessorEditor (&p), processor (p), advanced (p.apvts)
{
   #if CALMA_HAS_PEDAL_IMAGE
    background = juce::ImageCache::getFromMemory (BinaryData::pedal_jpg, BinaryData::pedal_jpgSize);
   #endif

    setLookAndFeel (&lookAndFeel);

    // Knob de Threshold
    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                              juce::MathConstants<float>::pi * 2.75f, true);
    knob.setMouseDragSensitivity (260);
    knob.setTextValueSuffix (" dB");
    knob.setPopupDisplayEnabled (true, true, this);
    knob.setDoubleClickReturnValue (true, -55.0);
    addAndMakeVisible (knob);
    knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ids::threshold.getParamID(), knob);

    // Footswitch (activa/desactiva o gate)
    addAndMakeVisible (footSwitch);
    footAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, ids::active.getParamID(), footSwitch);

    // Axustes avanzados (agochados ata o dobre clic)
    advanced.setVisible (false);
    addChildComponent (advanced);

    setResizable (true, true);
    setResizeLimits (260, 403, 800, 1240);
    getConstrainer()->setFixedAspectRatio ((double) layout::imageWidth / (double) layout::imageHeight);
    setSize (400, 620);

    startTimerHz (30);
}

CalmaEditor::~CalmaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::Rectangle<float> CalmaEditor::getLedBounds() const
{
    const float w = (float) getWidth(), h = (float) getHeight();
    const float r = layout::ledRadius * w;
    return { layout::ledCx * w - r, layout::ledCy * h - r, r * 2.0f, r * 2.0f };
}

void CalmaEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (background, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll (juce::Colour (0xff1c1612));   // sen imaxe: fondo liso
        g.setColour (brassMid);
        g.setFont (juce::FontOptions (20.0f));
        g.drawText ("Put your image in assets/pedal.jpg", getLocalBounds(), juce::Justification::centred);
    }

    // LED: brillo completo co gate aberto, medio co gate pechado, apagado en bypass
    if (ledLevel > 0.01f)
    {
        const auto led = getLedBounds();
        const auto c   = led.getCentre();
        const float r  = led.getWidth() * 0.5f;

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff3a24).withAlpha (0.55f * ledLevel), c.x, c.y,
                                                 juce::Colour (0xffff3a24).withAlpha (0.0f), c.x + r * 3.4f, c.y, true));
        g.fillEllipse (c.x - r * 3.4f, c.y - r * 3.4f, r * 6.8f, r * 6.8f);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffb09c).withAlpha (ledLevel), c.x - r * 0.3f, c.y - r * 0.3f,
                                                 juce::Colour (0xffe01808).withAlpha (ledLevel), c.x + r, c.y + r, true));
        g.fillEllipse (led.reduced (r * 0.15f));
    }
}

void CalmaEditor::resized()
{
    const float w = (float) getWidth(), h = (float) getHeight();

    const int ks = juce::roundToInt (layout::knobSize * w);
    knob.setBounds (juce::roundToInt (layout::knobCx * w) - ks / 2,
                    juce::roundToInt (layout::knobCy * h) - ks / 2, ks, ks);

    const int fs = juce::roundToInt (layout::footRadius * w * 2.0f);
    footSwitch.setBounds (juce::roundToInt (layout::footCx * w) - fs / 2,
                          juce::roundToInt (layout::footCy * h) - fs / 2, fs, fs);

    advanced.setBounds (juce::roundToInt (layout::plateX * w), juce::roundToInt (layout::plateY * h),
                        juce::roundToInt (layout::plateW * w), juce::roundToInt (layout::plateH * h));
}

void CalmaEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto plate = juce::Rectangle<float> (layout::plateX * (float) getWidth(), layout::plateY * (float) getHeight(),
                                               layout::plateW * (float) getWidth(), layout::plateH * (float) getHeight());
    if (plate.contains (e.position))
        advanced.setVisible (! advanced.isVisible());
}

void CalmaEditor::timerCallback()
{
    const bool active = processor.apvts.getRawParameterValue (ids::active.getParamID())->load() > 0.5f;
    const float target = active ? 0.45f + 0.55f * processor.gateLevel.load (std::memory_order_relaxed) : 0.0f;

    const float next = ledLevel + (target - ledLevel) * 0.5f;
    if (std::abs (next - ledLevel) > 0.004f)
    {
        ledLevel = next;
        repaint (getLedBounds().expanded (getLedBounds().getWidth() * 3.0f).getSmallestIntegerContainer());
    }
}

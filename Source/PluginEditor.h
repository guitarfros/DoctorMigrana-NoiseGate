// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ---------------------------------------------------------------------------
// Aspecto do knob: plástico negro envellecido con marca branca, rabuñaduras e marcas arredor.
// ---------------------------------------------------------------------------
class PedalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

private:
    void rebuildLayers (int size, float scale);

    juce::Image baseLayer, rotLayer;   // capas en caché (ver PluginEditor.cpp)
    int   cachedSize  = 0;
    float cachedScale = 0.0f;
};

// ---------------------------------------------------------------------------
// Footswitch: botón invisible circular sobre o pulsador da imaxe.
// ---------------------------------------------------------------------------
class FootSwitch : public juce::Button
{
public:
    FootSwitch();
    bool hitTest (int x, int y) override;
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
};

// ---------------------------------------------------------------------------
// Panel de axustes avanzados (Decay, Hold, Depth). Ábrese con dobre clic na placa.
// ---------------------------------------------------------------------------
class AdvancedPanel : public juce::Component
{
public:
    explicit AdvancedPanel (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Row
    {
        juce::Label  label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupRow (Row&, const juce::String& name, const juce::ParameterID& id,
                   const juce::String& suffix, juce::AudioProcessorValueTreeState&);

    Row decay, hold, depth;
};

// ---------------------------------------------------------------------------
class CalmaEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit CalmaEditor (CalmaProcessor&);
    ~CalmaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> getLedBounds() const;

    CalmaProcessor& processor;

    PedalLookAndFeel lookAndFeel;
    juce::Image background;

    juce::Slider knob;
    FootSwitch footSwitch;
    AdvancedPanel advanced;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footAttachment;

    float ledLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CalmaEditor)
};

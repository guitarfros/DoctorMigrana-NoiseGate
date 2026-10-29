// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "NoiseGate.h"

namespace ids
{
    inline const juce::ParameterID threshold { "threshold", 1 };
    inline const juce::ParameterID decay     { "decay",     1 };
    inline const juce::ParameterID hold      { "hold",      1 };
    inline const juce::ParameterID depth     { "depth",     1 };
    inline const juce::ParameterID active    { "active",    1 };
}

class CalmaProcessor : public juce::AudioProcessor
{
public:
    CalmaProcessor();
    ~CalmaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // 0 = gate pechado, 1 = aberto. Lido polo editor para o LED.
    std::atomic<float> gateLevel { 1.0f };

private:
    calma::NoiseGate gate;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* decayParam     = nullptr;
    std::atomic<float>* holdParam      = nullptr;
    std::atomic<float>* depthParam     = nullptr;
    std::atomic<float>* activeParam    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CalmaProcessor)
};

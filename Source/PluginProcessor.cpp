// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros
#include "PluginProcessor.h"
#include "PluginEditor.h"

CalmaProcessor::CalmaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    thresholdParam = apvts.getRawParameterValue (ids::threshold.getParamID());
    decayParam     = apvts.getRawParameterValue (ids::decay.getParamID());
    holdParam      = apvts.getRawParameterValue (ids::hold.getParamID());
    depthParam     = apvts.getRawParameterValue (ids::depth.getParamID());
    activeParam    = apvts.getRawParameterValue (ids::active.getParamID());
}

juce::AudioProcessorValueTreeState::ParameterLayout CalmaProcessor::createLayout()
{
    using juce::AudioParameterFloat;
    using juce::AudioParameterFloatAttributes;
    using juce::NormalisableRange;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (
        ids::threshold, "Threshold", NormalisableRange<float> (-80.0f, -10.0f, 0.1f), -55.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ids::decay, "Decay", NormalisableRange<float> (10.0f, 1500.0f, 1.0f, 0.5f), 120.0f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ids::hold, "Hold", NormalisableRange<float> (0.0f, 200.0f, 1.0f), 25.0f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ids::depth, "Depth", NormalisableRange<float> (6.0f, 80.0f, 0.5f), 80.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (ids::active, "Active", true));

    return layout;
}

void CalmaProcessor::prepareToPlay (double sampleRate, int)
{
    gate.prepare (sampleRate);
    setLatencySamples (gate.getLatencySamples());
}

bool CalmaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    const auto mono   = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();

    if ((in != mono && in != stereo) || (out != mono && out != stereo))
        return false;

    // mono->mono, mono->stereo e stereo->stereo; non stereo->mono
    return ! (in == stereo && out == mono);
}

void CalmaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int totalIn    = getTotalNumInputChannels();
    const int totalOut   = getTotalNumOutputChannels();

    if (totalIn == 0)
    {
        buffer.clear();
        return;
    }

    // Guitarra mono -> saída estéreo: duplica a canle 0 antes de procesar
    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    calma::GateSettings s;
    s.thresholdDb = thresholdParam->load();
    s.decayMs     = decayParam->load();
    s.holdMs      = holdParam->load();
    s.depthDb     = depthParam->load();
    s.active      = activeParam->load() > 0.5f;
    gate.setSettings (s);

    gate.process (buffer.getArrayOfWritePointers(), juce::jmin (totalOut, buffer.getNumChannels()), numSamples);

    gateLevel.store (gate.getCurrentGain(), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* CalmaProcessor::createEditor()
{
    return new CalmaEditor (*this);
}

void CalmaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CalmaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CalmaProcessor();
}

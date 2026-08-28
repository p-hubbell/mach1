#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace
{
constexpr int kLegacyAbBytes = static_cast<int> (2 * sizeof (float));
constexpr uint32_t kJuceXmlMagic = 0x21324356;
constexpr float kBypassThreshold = 0.5f;
constexpr int kStereoChannels = 2;

float peakOfChannels (const juce::AudioBuffer<float>& buffer, int numChannels, int numSamples) noexcept
{
    float peak = 0.0f;
    const int chLimit = juce::jmin (numChannels, buffer.getNumChannels());

    for (int ch = 0; ch < chLimit; ++ch)
    {
        const float* p = buffer.getReadPointer (ch);

        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax (peak, std::abs (p[i]));
    }

    return peak;
}
} // namespace

Mach1AudioProcessor::Mach1AudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    inTrimParam = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (inTrimId));
    outPadParam = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (outPadId));
    autoGainParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (autoGainId));
    jassert (inTrimParam != nullptr && outPadParam != nullptr && autoGainParam != nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout Mach1AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { inTrimId, 1 },
        "In Trim",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.1f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { outPadId, 1 },
        "Out Pad",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        1.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { autoGainId, 1 },
        "AutoGain",
        true));
    return layout;
}

void Mach1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    const auto cap = juce::jmax (8192, samplesPerBlock);
    monoRight.assign (static_cast<size_t> (cap), 0.0f);
}

void Mach1AudioProcessor::releaseResources()
{
    engine.reset();
}

void Mach1AudioProcessor::reset()
{
    engine.reset();
}

bool Mach1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::stereo()
        || in == juce::AudioChannelSet::mono();
}

void Mach1AudioProcessor::copyHostIo (juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numIn = getTotalNumInputChannels();
    const auto numOut = getTotalNumOutputChannels();

    for (int ch = 0; ch < numOut; ++ch)
    {
        if (ch < numIn)
        {
            auto* dest = buffer.getWritePointer (ch);
            const auto* src = buffer.getReadPointer (ch);

            if (dest != src)
                juce::FloatVectorOperations::copy (dest, src, numSamples);
        }
        else
        {
            buffer.clear (ch, 0, numSamples);
        }
    }
}

void Mach1AudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    copyHostIo (buffer);
    const auto numSamples = buffer.getNumSamples();
    const float peak = peakOfChannels (buffer, buffer.getNumChannels(), numSamples);
    inputPeak.store (peak, std::memory_order_relaxed);
    outputPeak.store (peak, std::memory_order_relaxed);
}

void Mach1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    if (auto* bypass = getBypassParameter(); bypass != nullptr && bypass->getValue() >= kBypassThreshold)
    {
        processBlockBypassed (buffer, midiMessages);
        return;
    }

    const auto numSamples = buffer.getNumSamples();
    const auto bufCh = buffer.getNumChannels();
    const auto numIn = juce::jmin (getTotalNumInputChannels(), bufCh);
    const auto numOut = juce::jmin (getTotalNumOutputChannels(), bufCh);

    for (int ch = kStereoChannels; ch < bufCh; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples <= 0 || numIn <= 0 || numOut <= 0 || bufCh <= 0)
        return;

    const float inPeak = peakOfChannels (buffer, numIn, numSamples);

    const float A = juce::jlimit (0.0f, 1.0f, inTrimParam != nullptr ? inTrimParam->get() : 0.1f);
    const float B = juce::jlimit (0.0f, 1.0f, outPadParam != nullptr ? outPadParam->get() : 1.0f);
    const bool autoGain = autoGainParam != nullptr && autoGainParam->get();

    if (numIn >= kStereoChannels && numOut >= kStereoChannels && bufCh >= kStereoChannels)
    {
        float* inPtrs[2] = { buffer.getWritePointer (0), buffer.getWritePointer (1) };
        float* outPtrs[2] = { inPtrs[0], inPtrs[1] };
        engine.process (inPtrs, outPtrs, numSamples, A, B, autoGain);
    }
    else
    {
        const int cap = static_cast<int> (monoRight.size());
        const int nProc = juce::jmin (numSamples, cap);

        if (nProc > 0)
        {
            std::fill (monoRight.begin(), monoRight.begin() + nProc, 0.0f);
            float* inPtrs[2] = { buffer.getWritePointer (0), monoRight.data() };
            float* outPtrs[2] = { inPtrs[0], monoRight.data() };
            engine.process (inPtrs, outPtrs, nProc, A, B, autoGain);
        }
    }

    const float outPeak = peakOfChannels (buffer, numOut, numSamples);
    inputPeak.store (inPeak, std::memory_order_relaxed);
    outputPeak.store (outPeak, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* Mach1AudioProcessor::createEditor()
{
    return new Mach1AudioProcessorEditor (*this);
}

void Mach1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void Mach1AudioProcessor::applyLegacyAbState (const float* values)
{
    const float a = juce::jlimit (0.0f, 1.0f, values[0]);
    const float b = juce::jlimit (0.0f, 1.0f, values[1]);

    if (inTrimParam != nullptr)
        *inTrimParam = a;

    if (outPadParam != nullptr)
        *outPadParam = b;

    if (autoGainParam != nullptr)
        *autoGainParam = false;
}

void Mach1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    if (sizeInBytes == kLegacyAbBytes)
    {
        if (juce::ByteOrder::littleEndianInt (data) == kJuceXmlMagic)
            return;

        float values[2] {};
        std::memcpy (values, data, sizeof (values));
        applyLegacyAbState (values);
        return;
    }

    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Mach1AudioProcessor();
}

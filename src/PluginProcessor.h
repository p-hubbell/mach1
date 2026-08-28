#pragma once

#include "MackityEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <vector>

class Mach1AudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr const char* inTrimId = "inTrim";
    static constexpr const char* outPadId = "outPad";
    static constexpr const char* autoGainId = "autoGain";

    Mach1AudioProcessor();
    ~Mach1AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "mach1"; }
    bool acceptsMidi() const override { return false; }
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

    juce::AudioProcessorValueTreeState apvts;

    float getInputPeak() const noexcept
    {
        return inputPeak.load (std::memory_order_relaxed);
    }

    float getOutputPeak() const noexcept
    {
        return outputPeak.load (std::memory_order_relaxed);
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void copyHostIo (juce::AudioBuffer<float>& buffer);
    void applyLegacyAbState (const float* values);

    mach1::MackityEngine engine;
    juce::AudioParameterFloat* inTrimParam = nullptr;
    juce::AudioParameterFloat* outPadParam = nullptr;
    juce::AudioParameterBool* autoGainParam = nullptr;
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::vector<float> monoRight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mach1AudioProcessor)
};

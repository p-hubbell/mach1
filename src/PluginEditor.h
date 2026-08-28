#pragma once

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

class Mach1LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    Mach1LookAndFeel();
};

class Mach1LevelMeter final : public juce::Component
{
public:
    void setLevel (float newLevel);
    float getLevel() const noexcept { return level; }

    void paint (juce::Graphics& g) override;

private:
    float level = 0.0f;
};

class Mach1AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit Mach1AudioProcessorEditor (Mach1AudioProcessor&);
    ~Mach1AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::String getAboutText() const { return aboutText.getText(); }
    void syncMetersFromProcessor();

private:
    void timerCallback() override;

    Mach1AudioProcessor& processorRef;
    Mach1LookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label inTrimLabel;
    juce::Label outPadLabel;
    juce::Slider inTrimSlider;
    juce::Slider outPadSlider;
    juce::ToggleButton autoGainButton;
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;
    Mach1LevelMeter inputMeter;
    Mach1LevelMeter outputMeter;
    juce::TextButton aboutButton;
    juce::Label aboutText;

    juce::AudioProcessorValueTreeState::SliderAttachment inTrimAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outPadAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment autoGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mach1AudioProcessorEditor)
};

#include "PluginEditor.h"

namespace
{
constexpr auto kBackground = 0xff121418;
constexpr auto kPanel = 0xff1c1f24;
constexpr auto kLabel = 0xffe8eaed;
constexpr auto kAccent = 0xffc8a44a;
constexpr auto kTrack = 0xff3a3f46;
} // namespace

Mach1LookAndFeel::Mach1LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (kBackground));
    setColour (juce::Label::textColourId, juce::Colour (kLabel));
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::thumbColourId, juce::Colour (kAccent));
    setColour (juce::Slider::trackColourId, juce::Colour (kTrack));
    setColour (juce::Slider::backgroundColourId, juce::Colour (kPanel));
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (kAccent));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (kTrack));
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (kLabel));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (kPanel));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff4a5058));
    setColour (juce::ToggleButton::textColourId, juce::Colour (kLabel));
    setColour (juce::ToggleButton::tickColourId, juce::Colour (kAccent));
    setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff5a5f66));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2e34));
    setColour (juce::TextButton::textColourOffId, juce::Colour (kLabel));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3a3f46));
}

void Mach1LevelMeter::setLevel (float newLevel)
{
    level = juce::jlimit (0.0f, 1.0f, newLevel);
    repaint();
}

void Mach1LevelMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (kPanel));
    g.fillRoundedRectangle (bounds, 2.0f);
    g.setColour (juce::Colour (0xff4a5058));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 2.0f, 1.0f);

    auto fill = bounds.reduced (3.0f);
    const float h = fill.getHeight() * level;
    g.setColour (juce::Colour (kAccent));
    g.fillRect (fill.removeFromBottom (h));
}

Mach1AudioProcessorEditor::Mach1AudioProcessorEditor (Mach1AudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p),
      inTrimAttachment (p.apvts, Mach1AudioProcessor::inTrimId, inTrimSlider),
      outPadAttachment (p.apvts, Mach1AudioProcessor::outPadId, outPadSlider),
      autoGainAttachment (p.apvts, Mach1AudioProcessor::autoGainId, autoGainButton)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("mach1", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    inTrimLabel.setText ("In Trim", juce::dontSendNotification);
    inTrimLabel.setJustificationType (juce::Justification::centredLeft);
    inTrimLabel.setComponentID ("inTrimLabel");
    addAndMakeVisible (inTrimLabel);

    outPadLabel.setText ("Out Pad", juce::dontSendNotification);
    outPadLabel.setJustificationType (juce::Justification::centredLeft);
    outPadLabel.setComponentID ("outPadLabel");
    addAndMakeVisible (outPadLabel);

    inTrimSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    inTrimSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
    inTrimSlider.setComponentID ("inTrim");
    addAndMakeVisible (inTrimSlider);

    outPadSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    outPadSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
    outPadSlider.setComponentID ("outPad");
    addAndMakeVisible (outPadSlider);

    autoGainButton.setButtonText ("AutoGain");
    autoGainButton.setComponentID ("autoGain");
    addAndMakeVisible (autoGainButton);

    inputMeterLabel.setText ("IN", juce::dontSendNotification);
    inputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (inputMeterLabel);

    outputMeterLabel.setText ("OUT", juce::dontSendNotification);
    outputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputMeterLabel);

    inputMeter.setComponentID ("inMeter");
    outputMeter.setComponentID ("outMeter");
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    aboutButton.setButtonText ("About");
    aboutButton.setComponentID ("aboutButton");
    addAndMakeVisible (aboutButton);

    aboutText.setText (
        "mach1. DSP algorithm from Airwindows Mackity (MIT license). "
        "This product is not Airwindows and is not titled Mackity.",
        juce::dontSendNotification);
    aboutText.setJustificationType (juce::Justification::topLeft);
    aboutText.setMinimumHorizontalScale (0.7f);
    aboutText.setComponentID ("about");
    aboutText.setVisible (false);
    addAndMakeVisible (aboutText);

    aboutButton.onClick = [this]
    {
        aboutText.setVisible (! aboutText.isVisible());
        resized();
    };

    setSize (520, 280);
    setName ("mach1");
    startTimerHz (40);
}

Mach1AudioProcessorEditor::~Mach1AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void Mach1AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (kBackground));
}

void Mach1AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);
    titleLabel.setBounds (bounds.removeFromTop (28));
    bounds.removeFromTop (8);

    auto meterCol = bounds.removeFromRight (72);
    inputMeterLabel.setBounds (meterCol.removeFromTop (16));
    inputMeter.setBounds (meterCol.removeFromTop (88));
    meterCol.removeFromTop (8);
    outputMeterLabel.setBounds (meterCol.removeFromTop (16));
    outputMeter.setBounds (meterCol.removeFromTop (88));

    bounds.removeFromRight (16);

    auto trimRow = bounds.removeFromTop (40);
    inTrimLabel.setBounds (trimRow.removeFromLeft (80));
    inTrimSlider.setBounds (trimRow);

    bounds.removeFromTop (8);
    auto padRow = bounds.removeFromTop (40);
    outPadLabel.setBounds (padRow.removeFromLeft (80));
    outPadSlider.setBounds (padRow);

    bounds.removeFromTop (8);
    autoGainButton.setBounds (bounds.removeFromTop (28).removeFromLeft (140));

    bounds.removeFromTop (8);
    aboutButton.setBounds (bounds.removeFromTop (24).removeFromLeft (80));
    bounds.removeFromTop (6);
    aboutText.setBounds (bounds);
}

void Mach1AudioProcessorEditor::syncMetersFromProcessor()
{
    inputMeter.setLevel (processorRef.getInputPeak());
    outputMeter.setLevel (processorRef.getOutputPeak());
}

void Mach1AudioProcessorEditor::timerCallback()
{
    syncMetersFromProcessor();
}

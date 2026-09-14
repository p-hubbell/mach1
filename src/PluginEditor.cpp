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
      colorAttachment (p.apvts, Mach1AudioProcessor::colorId, colorSlider),
      inTrimAttachment (p.apvts, Mach1AudioProcessor::inTrimId, inTrimSlider),
      outPadAttachment (p.apvts, Mach1AudioProcessor::outPadId, outPadSlider),
      autoGainAttachment (p.apvts, Mach1AudioProcessor::autoGainId, autoGainButton)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("mach1", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    colorLabel.setText ("Color", juce::dontSendNotification);
    colorLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (colorLabel);

    classicLabel.setText ("Classic", juce::dontSendNotification);
    classicLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (classicLabel);

    evenLabel.setText ("Even", juce::dontSendNotification);
    evenLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (evenLabel);

    driveLabel.setText ("Drive", juce::dontSendNotification);
    driveLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (driveLabel);

    outputLabel.setText ("Output", juce::dontSendNotification);
    outputLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputLabel);

    colorSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    colorSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    colorSlider.setComponentID ("color");
    addAndMakeVisible (colorSlider);

    inTrimSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    inTrimSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 18);
    inTrimSlider.setComponentID ("inTrim");
    addAndMakeVisible (inTrimSlider);

    outPadSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outPadSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 18);
    outPadSlider.setComponentID ("outPad");
    addAndMakeVisible (outPadSlider);

    autoGainButton.setButtonText ("Auto Gain");
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

    setSize (720, 340);
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

    auto header = bounds.removeFromTop (28);
    aboutButton.setBounds (header.removeFromRight (80));
    header.removeFromRight (8);
    titleLabel.setBounds (header);

    if (aboutText.isVisible())
        aboutText.setBounds (bounds.removeFromBottom (52));
    else
        aboutText.setBounds ({});

    bounds.removeFromTop (10);

    constexpr int meterW = 28;
    constexpr int gap = 10;
    constexpr int colorKnob = 90;
    constexpr int driveKnob = 150;
    constexpr int outputKnob = 90;
    constexpr int sideLabelW = 52;

    auto inCol = bounds.removeFromLeft (meterW);
    bounds.removeFromLeft (gap);
    auto outCol = bounds.removeFromRight (meterW);
    bounds.removeFromRight (gap);

    inputMeterLabel.setBounds (inCol.removeFromTop (16));
    inputMeter.setBounds (inCol);

    outputMeterLabel.setBounds (outCol.removeFromTop (16));
    outputMeter.setBounds (outCol);

    const int remainingW = bounds.getWidth();
    const int colorColW = juce::jmax (colorKnob + sideLabelW * 2, juce::roundToInt (remainingW * 0.30f));
    const int driveColW = juce::jmax (driveKnob + 16, juce::roundToInt (remainingW * 0.40f));

    auto colorCol = bounds.removeFromLeft (colorColW);
    bounds.removeFromLeft (gap);
    auto driveCol = bounds.removeFromLeft (driveColW);
    bounds.removeFromLeft (gap);
    auto outputCol = bounds;

    colorLabel.setBounds (colorCol.removeFromTop (20));
    colorCol.removeFromTop (4);
    auto colorRow = colorCol.removeFromTop (colorKnob);
    classicLabel.setBounds (colorRow.removeFromLeft (sideLabelW));
    evenLabel.setBounds (colorRow.removeFromRight (sideLabelW));
    colorSlider.setBounds (colorRow.withSizeKeepingCentre (colorKnob, colorKnob));

    driveLabel.setBounds (driveCol.removeFromTop (20));
    driveCol.removeFromTop (4);
    inTrimSlider.setBounds (driveCol.removeFromTop (driveKnob).withSizeKeepingCentre (driveKnob, driveKnob));

    outputLabel.setBounds (outputCol.removeFromTop (20));
    outputCol.removeFromTop (4);
    outPadSlider.setBounds (outputCol.removeFromTop (outputKnob).withSizeKeepingCentre (outputKnob, outputKnob));
    outputCol.removeFromTop (8);
    autoGainButton.setBounds (outputCol.removeFromTop (24).withSizeKeepingCentre (110, 24));
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

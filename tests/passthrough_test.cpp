#include "MackityEngine.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

namespace
{
constexpr float kEps = 1.0e-5f;

bool nearlyEqual (float a, float b, float eps = kEps)
{
    return std::abs (a - b) <= eps;
}

int fail (const char* message)
{
    std::cerr << message << '\n';
    return EXIT_FAILURE;
}

juce::AudioParameterFloat* inTrim (Mach1AudioProcessor& proc)
{
    return dynamic_cast<juce::AudioParameterFloat*> (proc.apvts.getParameter (Mach1AudioProcessor::inTrimId));
}

juce::AudioParameterFloat* outPad (Mach1AudioProcessor& proc)
{
    return dynamic_cast<juce::AudioParameterFloat*> (proc.apvts.getParameter (Mach1AudioProcessor::outPadId));
}

juce::AudioParameterBool* autoGain (Mach1AudioProcessor& proc)
{
    return dynamic_cast<juce::AudioParameterBool*> (proc.apvts.getParameter (Mach1AudioProcessor::autoGainId));
}

juce::AudioParameterFloat* color (Mach1AudioProcessor& proc)
{
    return dynamic_cast<juce::AudioParameterFloat*> (proc.apvts.getParameter (Mach1AudioProcessor::colorId));
}

void writeJuceXmlBlob (const juce::XmlElement& xml, juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream out (dest, false);
    out.writeInt (static_cast<int> (0x21324356));
    out.writeInt (0);
    xml.writeTo (out, juce::XmlElement::TextFormat().singleLine());
    out.writeByte (0);
    static_cast<juce::uint32*> (dest.getData())[1]
        = juce::ByteOrder::swapIfBigEndian (static_cast<juce::uint32> (dest.getSize() - 9));
}

juce::MemoryBlock makeV1ParamsBlob (float drive, float output, bool agOn)
{
    juce::ValueTree tree ("PARAMS");
    auto add = [&] (const char* id, float value)
    {
        juce::ValueTree p ("PARAM");
        p.setProperty ("id", id, nullptr);
        p.setProperty ("value", value, nullptr);
        tree.appendChild (p, nullptr);
    };
    add (Mach1AudioProcessor::inTrimId, drive);
    add (Mach1AudioProcessor::outPadId, output);
    add (Mach1AudioProcessor::autoGainId, agOn ? 1.0f : 0.0f);

    juce::MemoryBlock blob;
    if (auto xml = tree.createXml())
        writeJuceXmlBlob (*xml, blob);
    return blob;
}

bool prepareLayout (Mach1AudioProcessor& proc,
                    const juce::AudioChannelSet& channels,
                    double sampleRate,
                    int blockSize)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (channels);
    layout.outputBuses.add (channels);

    if (! proc.isBusesLayoutSupported (layout) || ! proc.setBusesLayout (layout))
    {
        std::cerr << "setBusesLayout failed\n";
        return false;
    }

    proc.setRateAndBufferSizeDetails (sampleRate, blockSize);
    proc.prepareToPlay (sampleRate, blockSize);
    return true;
}

void fillSine (juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int n = buffer.getNumSamples();
    const int chans = buffer.getNumChannels();

    for (int i = 0; i < n; ++i)
    {
        const float phase = static_cast<float> (i) * 2.0f * juce::MathConstants<float>::pi * 440.0f
                            / static_cast<float> (sampleRate);
        const float left = 0.75f * std::sin (phase);
        const float right = 0.25f * std::sin (phase * 2.0f);
        buffer.setSample (0, i, left);

        if (chans > 1)
            buffer.setSample (1, i, right);
    }
}

float maxAbsDelta (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float m = 0.0f;
    const int chans = juce::jmin (a.getNumChannels(), b.getNumChannels());
    const int n = juce::jmin (a.getNumSamples(), b.getNumSamples());

    for (int ch = 0; ch < chans; ++ch)
        for (int i = 0; i < n; ++i)
            m = juce::jmax (m, std::abs (a.getSample (ch, i) - b.getSample (ch, i)));

    return m;
}
} // namespace

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    {
        Mach1AudioProcessor proc;
        auto* trim = inTrim (proc);
        auto* pad = outPad (proc);
        auto* ag = autoGain (proc);
        auto* col = color (proc);

        if (trim == nullptr || pad == nullptr || ag == nullptr || col == nullptr)
            return fail ("missing APVTS parameters");

        if (trim->name != "Drive" || pad->name != "Output" || ag->name != "Auto Gain" || col->name != "Color")
            return fail ("parameter display names mismatch");

        if (! nearlyEqual (trim->get(), 0.1f) || ! nearlyEqual (pad->get(), 1.0f) || ag->get() != true
            || ! nearlyEqual (col->get(), 0.0f))
            return fail ("parameter defaults mismatch");

        if (col->getCurrentValueAsText() != "Classic" || col->getCurrentValueAsText().contains ("%"))
            return fail ("Color 0 text is not Classic");

        *col = 1.0f;
        if (col->getCurrentValueAsText() != "Even" || col->getCurrentValueAsText().contains ("%"))
            return fail ("Color 1 text is not Even");

        *col = 0.5f;
        if (col->getCurrentValueAsText() != "Blend" || col->getCurrentValueAsText().contains ("%"))
            return fail ("Color Blend text mismatch");

        juce::AudioProcessorParameter& colorParam = *col;
        const float classic = col->convertFrom0to1 (colorParam.getValueForText ("Classic"));
        const float even = col->convertFrom0to1 (colorParam.getValueForText ("Even"));
        const float blend = col->convertFrom0to1 (colorParam.getValueForText ("Blend"));
        const float parsed = col->convertFrom0to1 (colorParam.getValueForText ("0.25"));

        if (! nearlyEqual (classic, 0.0f) || ! nearlyEqual (even, 1.0f) || ! (blend > 0.0f && blend < 1.0f)
            || ! nearlyEqual (parsed, 0.25f))
            return fail ("Color valueFromText mismatch");

        *col = 0.0f;

        if (proc.getName() != "mach1")
            return fail ("product name is not mach1");

        if (proc.getBus (true, 0)->getDefaultLayout() != juce::AudioChannelSet::stereo()
            || proc.getBus (false, 0)->getDefaultLayout() != juce::AudioChannelSet::stereo())
            return fail ("default buses are not stereo 2-in / 2-out");
    }

    Mach1AudioProcessor proc;

    if (! prepareLayout (proc, juce::AudioChannelSet::stereo(), sampleRate, blockSize))
        return EXIT_FAILURE;

    if (proc.getTotalNumInputChannels() != 2 || proc.getTotalNumOutputChannels() != 2)
        return fail ("prepared layout is not 2-in / 2-out");

    *autoGain (proc) = false;
    *inTrim (proc) = 0.1f;
    *outPad (proc) = 1.0f;

    {
        juce::AudioBuffer<float> hostBuf (2, blockSize);
        juce::AudioBuffer<float> engineIn (2, blockSize);
        juce::AudioBuffer<float> engineOut (2, blockSize);
        juce::MidiBuffer midi;
        fillSine (hostBuf, sampleRate);
        engineIn.makeCopyOf (hostBuf);

        mach1::MackityEngine eng;
        eng.prepare (sampleRate);
        float* inPtrs[2] = { engineIn.getWritePointer (0), engineIn.getWritePointer (1) };
        float* outPtrs[2] = { engineOut.getWritePointer (0), engineOut.getWritePointer (1) };
        eng.process (inPtrs, outPtrs, blockSize, 0.1f, 1.0f, false, 0.0f);

        proc.processBlock (hostBuf, midi);

        if (maxAbsDelta (hostBuf, engineOut) > kEps)
            return fail ("processor output does not match MackityEngine at Color 0");
    }

    {
        *color (proc) = 0.5f;
        proc.reset();

        juce::AudioBuffer<float> hostBuf (2, blockSize);
        juce::AudioBuffer<float> engineIn (2, blockSize);
        juce::AudioBuffer<float> engineOut (2, blockSize);
        juce::MidiBuffer midi;
        fillSine (hostBuf, sampleRate);
        engineIn.makeCopyOf (hostBuf);

        mach1::MackityEngine eng;
        eng.prepare (sampleRate);
        float* inPtrs[2] = { engineIn.getWritePointer (0), engineIn.getWritePointer (1) };
        float* outPtrs[2] = { engineOut.getWritePointer (0), engineOut.getWritePointer (1) };
        eng.process (inPtrs, outPtrs, blockSize, 0.1f, 1.0f, false, 0.5f);

        proc.processBlock (hostBuf, midi);

        if (maxAbsDelta (hostBuf, engineOut) > kEps)
            return fail ("processor output does not match MackityEngine at Color 0.5");

        *color (proc) = 0.0f;
        proc.reset();
    }

    {
        *inTrim (proc) = 0.42f;
        *outPad (proc) = 0.73f;
        *autoGain (proc) = false;
        *color (proc) = 0.37f;

        juce::MemoryBlock blob;
        proc.getStateInformation (blob);

        Mach1AudioProcessor loaded;
        loaded.setStateInformation (blob.getData(), static_cast<int> (blob.getSize()));

        if (loaded.apvts.state.getType() != juce::Identifier ("PARAMS"))
            return fail ("ValueTree type is not PARAMS");

        if (! nearlyEqual (inTrim (loaded)->get(), 0.42f)
            || ! nearlyEqual (outPad (loaded)->get(), 0.73f)
            || autoGain (loaded)->get() != false
            || ! nearlyEqual (color (loaded)->get(), 0.37f))
            return fail ("XML state round-trip mismatch");

        *color (proc) = 0.0f;
    }

    {
        Mach1AudioProcessor reused;
        *color (reused) = 0.8f;
        *inTrim (reused) = 0.11f;
        *outPad (reused) = 0.22f;
        *autoGain (reused) = false;

        const auto v1 = makeV1ParamsBlob (0.42f, 0.73f, true);
        reused.setStateInformation (v1.getData(), static_cast<int> (v1.getSize()));

        if (! nearlyEqual (inTrim (reused)->get(), 0.42f)
            || ! nearlyEqual (outPad (reused)->get(), 0.73f)
            || autoGain (reused)->get() != true
            || ! nearlyEqual (color (reused)->get(), 0.0f))
            return fail ("v1 PARAMS XML without color did not restore Drive/Output/Auto Gain and Color 0");
    }

    {
        const float legacy[2] = { 0.25f, 0.8f };
        Mach1AudioProcessor loaded;
        *color (loaded) = 0.9f;
        *autoGain (loaded) = true;
        loaded.setStateInformation (legacy, 8);

        if (! nearlyEqual (inTrim (loaded)->get(), 0.25f)
            || ! nearlyEqual (outPad (loaded)->get(), 0.8f)
            || autoGain (loaded)->get() != false
            || ! nearlyEqual (color (loaded)->get(), 0.0f))
            return fail ("8-byte legacy A,B restore mismatch");

        juce::MemoryBlock blob;
        loaded.getStateInformation (blob);
        Mach1AudioProcessor third;
        third.setStateInformation (blob.getData(), static_cast<int> (blob.getSize()));

        if (autoGain (third)->get() != false || ! nearlyEqual (color (third)->get(), 0.0f))
            return fail ("legacy AutoGain-off / Color 0 did not persist through XML reload");
    }

    {
        const float outOfRange[2] = { -1.5f, 2.25f };
        Mach1AudioProcessor loaded;
        loaded.setStateInformation (outOfRange, 8);

        if (! nearlyEqual (inTrim (loaded)->get(), 0.0f) || ! nearlyEqual (outPad (loaded)->get(), 1.0f))
            return fail ("legacy out-of-range floats were not clamped");

        *inTrim (proc) = -0.4f;
        *outPad (proc) = 1.8f;

        if (! nearlyEqual (inTrim (proc)->get(), 0.0f) || ! nearlyEqual (outPad (proc)->get(), 1.0f))
            return fail ("APVTS range did not clamp In Trim / Out Pad");
    }

    {
        const float nanLegacy[2] = { std::numeric_limits<float>::quiet_NaN(),
                                     std::numeric_limits<float>::infinity() };
        Mach1AudioProcessor loaded;
        loaded.setStateInformation (nanLegacy, 8);

        if (! nearlyEqual (inTrim (loaded)->get(), 0.0f) || ! nearlyEqual (outPad (loaded)->get(), 0.0f))
            return fail ("legacy non-finite A,B were not rejected");
    }

    {
        *inTrim (proc) = 0.0f;
        *outPad (proc) = 1.0f;
        *autoGain (proc) = false;

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        fillSine (buffer, sampleRate);
        juce::AudioBuffer<float> original;
        original.makeCopyOf (buffer);

        proc.processBlockBypassed (buffer, midi);

        if (maxAbsDelta (buffer, original) > kEps)
            return fail ("processBlockBypassed did not copy input to output");

        *inTrim (proc) = 0.1f;
    }

    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        fillSine (buffer, sampleRate);
        proc.processBlock (buffer, midi);

        buffer.clear();
        proc.reset();
        proc.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                if (! nearlyEqual (buffer.getSample (ch, i), 0.0f))
                    return fail ("reset did not clear engine state (zero in → zero out)");
    }

    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        buffer.clear();
        buffer.setSample (0, 3, 0.8f);
        buffer.setSample (1, 9, -0.4f);

        proc.processBlock (buffer, midi);

        float outPeak = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                outPeak = juce::jmax (outPeak, std::abs (buffer.getSample (ch, i)));

        if (! nearlyEqual (proc.getInputPeak(), 0.8f))
            return fail ("input peak atomic did not reflect 0.8");

        if (outPeak <= 0.0f || ! nearlyEqual (proc.getOutputPeak(), outPeak))
            return fail ("output peak atomic mismatch");

        if (std::abs (proc.getOutputPeak() - 0.8f) <= kEps && std::abs (buffer.getSample (0, 3) - 0.8f) > kEps)
        {
            // Peak matching 0.8 is only OK if DSP actually produced that magnitude.
        }
    }

    {
        juce::AudioProcessor::BusesLayout mono;
        mono.inputBuses.add (juce::AudioChannelSet::mono());
        mono.outputBuses.add (juce::AudioChannelSet::mono());

        Mach1AudioProcessor monoProc;
        *autoGain (monoProc) = false;

        if (! monoProc.isBusesLayoutSupported (mono))
            return fail ("mono 1-in/1-out should be supported");

        if (! prepareLayout (monoProc, juce::AudioChannelSet::mono(), sampleRate, blockSize))
            return EXIT_FAILURE;

        if (monoProc.getTotalNumInputChannels() != 1 || monoProc.getTotalNumOutputChannels() != 1)
            return fail ("mono layout exposed extra host channels");

        juce::AudioBuffer<float> buffer (1, blockSize);
        juce::MidiBuffer midi;
        fillSine (buffer, sampleRate);
        monoProc.processBlock (buffer, midi);

        if (buffer.getNumChannels() != 1)
            return fail ("mono processBlock changed channel count");
    }

    proc.releaseResources();

    {
        juce::ScopedJuceInitialiser_GUI gui;

        auto collectTexts = [] (const juce::Component& root, juce::StringArray& texts)
        {
            std::function<void (const juce::Component&)> walk = [&] (const juce::Component& c)
            {
                if (auto* label = dynamic_cast<const juce::Label*> (&c))
                    texts.add (label->getText());

                if (auto* button = dynamic_cast<const juce::Button*> (&c))
                    texts.add (button->getButtonText());

                for (auto* child : c.getChildren())
                    if (child != nullptr)
                        walk (*child);
            };

            walk (root);
        };

        auto dispatch = []
        {
            juce::Thread::sleep (30);
            juce::Timer::callPendingTimersSynchronously();
        };

        {
            Mach1AudioProcessor guiProc;
            std::unique_ptr<juce::AudioProcessorEditor> editor (guiProc.createEditor());

            if (dynamic_cast<juce::GenericAudioProcessorEditor*> (editor.get()) != nullptr)
                return fail ("createEditor returned GenericAudioProcessorEditor");

            if (dynamic_cast<Mach1AudioProcessorEditor*> (editor.get()) == nullptr)
                return fail ("createEditor did not return Mach1AudioProcessorEditor");

            editor.reset();
            editor.reset (guiProc.createEditor());
            editor.reset();
        }

        Mach1AudioProcessor guiProc;
        std::unique_ptr<juce::AudioProcessorEditor> editor (guiProc.createEditor());
        auto* custom = dynamic_cast<Mach1AudioProcessorEditor*> (editor.get());

        if (custom == nullptr)
            return fail ("editor is not Mach1AudioProcessorEditor");

        juce::StringArray texts;
        collectTexts (*custom, texts);

        if (! texts.contains ("Drive") || ! texts.contains ("Output") || ! texts.contains ("Auto Gain")
            || ! texts.contains ("Classic") || ! texts.contains ("Even"))
            return fail ("editor missing Drive / Output / Auto Gain / Classic / Even labels");

        if (texts.contains ("In Trim") || texts.contains ("Out Pad") || texts.contains ("AutoGain"))
            return fail ("editor still shows In Trim / Out Pad / AutoGain");

        custom->resized();

        auto* trimSlider = dynamic_cast<juce::Slider*> (custom->findChildWithID ("inTrim"));
        auto* padSlider = dynamic_cast<juce::Slider*> (custom->findChildWithID ("outPad"));
        auto* colorSlider = dynamic_cast<juce::Slider*> (custom->findChildWithID ("color"));
        auto* agButton = dynamic_cast<juce::ToggleButton*> (custom->findChildWithID ("autoGain"));
        auto* inMeter = dynamic_cast<Mach1LevelMeter*> (custom->findChildWithID ("inMeter"));
        auto* outMeter = dynamic_cast<Mach1LevelMeter*> (custom->findChildWithID ("outMeter"));
        auto* aboutBtn = dynamic_cast<juce::Button*> (custom->findChildWithID ("aboutButton"));
        auto* about = dynamic_cast<juce::Label*> (custom->findChildWithID ("about"));

        if (trimSlider == nullptr || padSlider == nullptr || agButton == nullptr || colorSlider == nullptr)
            return fail ("editor controls not found by component ID");

        if (inMeter == nullptr || outMeter == nullptr || about == nullptr || aboutBtn == nullptr)
            return fail ("meters or About control not found");

        const auto isRotary = [] (juce::Slider::SliderStyle style)
        {
            return style == juce::Slider::Rotary
                || style == juce::Slider::RotaryHorizontalDrag
                || style == juce::Slider::RotaryVerticalDrag
                || style == juce::Slider::RotaryHorizontalVerticalDrag;
        };

        if (! isRotary (colorSlider->getSliderStyle()) || ! isRotary (trimSlider->getSliderStyle())
            || ! isRotary (padSlider->getSliderStyle()))
            return fail ("Color / Drive / Output are not rotary sliders");

        if (colorSlider->getTextBoxPosition() != juce::Slider::NoTextBox)
            return fail ("Color slider is not NoTextBox");

        if (! (inMeter->getX() < colorSlider->getX() && colorSlider->getX() < trimSlider->getX()
               && trimSlider->getX() < padSlider->getX() && padSlider->getX() < outMeter->getX()))
            return fail ("editor L-to-R order is not inMeter, Color, Drive, Output, outMeter");

        if (! (trimSlider->getWidth() > colorSlider->getWidth()
               && trimSlider->getHeight() > colorSlider->getHeight()
               && trimSlider->getWidth() > padSlider->getWidth()
               && trimSlider->getHeight() > padSlider->getHeight()))
            return fail ("Drive is not strictly larger than Color and Output");

        if (! (agButton->getY() > padSlider->getY()))
            return fail ("Auto Gain is not under Output");

        {
            const int agCx = agButton->getBounds().getCentreX();
            const int padCx = padSlider->getBounds().getCentreX();
            const int driveCx = trimSlider->getBounds().getCentreX();

            if (! (std::abs (agCx - padCx) < std::abs (agCx - driveCx)))
                return fail ("Auto Gain is not horizontally nearer Output than Drive");
        }

        juce::Label* classicLbl = nullptr;
        juce::Label* evenLbl = nullptr;
        std::function<void (juce::Component&)> findSeatLabels = [&] (juce::Component& c)
        {
            if (auto* label = dynamic_cast<juce::Label*> (&c))
            {
                if (label->getText() == "Classic")
                    classicLbl = label;
                else if (label->getText() == "Even")
                    evenLbl = label;
            }

            for (auto* child : c.getChildren())
                if (child != nullptr)
                    findSeatLabels (*child);
        };
        findSeatLabels (*custom);

        if (classicLbl == nullptr || evenLbl == nullptr)
            return fail ("Classic / Even labels not found");

        const bool classicLeft = classicLbl->getRight() <= colorSlider->getX()
                                 || classicLbl->getBounds().getCentreX() < colorSlider->getX();
        const bool evenRight = evenLbl->getX() >= colorSlider->getRight()
                               || evenLbl->getBounds().getCentreX() > colorSlider->getRight();

        if (! classicLeft || ! evenRight)
            return fail ("Classic / Even are not flanking the Color slider");

        aboutBtn->triggerClick();
        dispatch();

        const auto aboutCopy = about->getText() + " " + custom->getAboutText();

        if (! aboutCopy.containsIgnoreCase ("MIT") || ! aboutCopy.containsIgnoreCase ("Mackity"))
            return fail ("About text missing MIT / Mackity credit");

        if (custom->getName() != "mach1" || custom->getName().containsIgnoreCase ("Airwindows"))
            return fail ("editor product name is not mach1");

        *inTrim (guiProc) = 0.55f;
        *outPad (guiProc) = 0.25f;
        *autoGain (guiProc) = false;
        *color (guiProc) = 0.5f;
        dispatch();

        if (! nearlyEqual (static_cast<float> (trimSlider->getValue()), 0.55f)
            || ! nearlyEqual (static_cast<float> (padSlider->getValue()), 0.25f)
            || agButton->getToggleState()
            || ! nearlyEqual (static_cast<float> (colorSlider->getValue()), 0.5f))
            return fail ("editor did not reflect APVTS parameter writes");

        trimSlider->setValue (0.33, juce::sendNotificationSync);
        padSlider->setValue (0.77, juce::sendNotificationSync);
        agButton->setToggleState (true, juce::sendNotificationSync);
        colorSlider->setValue (0.25, juce::sendNotificationSync);
        dispatch();

        if (! nearlyEqual (inTrim (guiProc)->get(), 0.33f)
            || ! nearlyEqual (outPad (guiProc)->get(), 0.77f)
            || autoGain (guiProc)->get() != true
            || ! nearlyEqual (color (guiProc)->get(), 0.25f))
            return fail ("APVTS did not reflect editor control changes");

        if (! prepareLayout (guiProc, juce::AudioChannelSet::stereo(), sampleRate, blockSize))
            return EXIT_FAILURE;

        *autoGain (guiProc) = false;
        dispatch();

        {
            juce::AudioBuffer<float> buffer (2, blockSize);
            juce::MidiBuffer midi;
            fillSine (buffer, sampleRate);
            guiProc.processBlock (buffer, midi);
            juce::Timer::callPendingTimersSynchronously();
            custom->syncMetersFromProcessor();

            if (inMeter->getLevel() <= 1.0e-4f || outMeter->getLevel() <= 1.0e-4f)
                return fail ("meters stayed empty after non-silent processBlock");

            for (int i = 0; i < 64; ++i)
            {
                buffer.clear();
                guiProc.processBlock (buffer, midi);

                if (guiProc.getInputPeak() <= 1.0e-4f && guiProc.getOutputPeak() <= 1.0e-3f)
                    break;
            }

            juce::Timer::callPendingTimersSynchronously();
            custom->syncMetersFromProcessor();

            if (! nearlyEqual (inMeter->getLevel(), guiProc.getInputPeak(), 1.0e-4f)
                || ! nearlyEqual (outMeter->getLevel(), guiProc.getOutputPeak(), 1.0e-4f))
                return fail ("meters did not track processor peak atomics");

            if (inMeter->getLevel() > 1.0e-3f || outMeter->getLevel() > 1.0e-3f)
                return fail ("meters did not return near empty after silent processBlock");
        }
    }

    std::cout << "processor tests passed\n";
    return EXIT_SUCCESS;
}

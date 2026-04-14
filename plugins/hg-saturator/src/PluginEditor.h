#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "HGLookAndFeel.h"

class Oscilloscope : public juce::Component, private juce::Timer
{
public:
    Oscilloscope() { startTimerHz(30); }

    void pushInputSample(float sample)
    {
        inputBuffer[writePos] = sample;
    }

    void pushOutputSample(float sample)
    {
        outputBuffer[writePos] = sample;
        writePos = (writePos + 1) % bufferSize;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 3.0f);

        // Grid lines
        g.setColour(juce::Colour(0xFF3A3A3A));
        g.drawHorizontalLine(static_cast<int>(bounds.getCentreY()), bounds.getX(), bounds.getRight());
        g.drawVerticalLine(static_cast<int>(bounds.getCentreX()), bounds.getY(), bounds.getBottom());

        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float midY = bounds.getCentreY();
        const int displaySamples = 256;

        // Draw input waveform (dim)
        {
            juce::Path path;
            bool started = false;
            int readPos = (writePos - displaySamples + bufferSize) % bufferSize;
            for (int i = 0; i < displaySamples; ++i)
            {
                float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(displaySamples - 1)) * w;
                float y = midY - inputBuffer[(readPos + i) % bufferSize] * (h * 0.45f);
                if (!started) { path.startNewSubPath(x, y); started = true; }
                else path.lineTo(x, y);
            }
            g.setColour(juce::Colour(0xFF666666));
            g.strokePath(path, juce::PathStrokeType(1.0f));
        }

        // Draw output waveform (bright orange)
        {
            juce::Path path;
            bool started = false;
            int readPos = (writePos - displaySamples + bufferSize) % bufferSize;
            for (int i = 0; i < displaySamples; ++i)
            {
                float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(displaySamples - 1)) * w;
                float y = midY - outputBuffer[(readPos + i) % bufferSize] * (h * 0.45f);
                if (!started) { path.startNewSubPath(x, y); started = true; }
                else path.lineTo(x, y);
            }
            g.setColour(juce::Colour(0xFFFF9800));
            g.strokePath(path, juce::PathStrokeType(1.5f));
        }
    }

private:
    void timerCallback() override { repaint(); }

    static constexpr int bufferSize = 2048;
    float inputBuffer[bufferSize] = {};
    float outputBuffer[bufferSize] = {};
    int writePos = 0;
};

class HGSaturatorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HGSaturatorEditor(HGSaturatorProcessor& p);
    ~HGSaturatorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    HGSaturatorProcessor& processorRef;
    HGLookAndFeel lookAndFeel{juce::Colour(0xFFFF9800)};

    // Knobs
    juce::Slider inputGainKnob, driveKnob, biasKnob, outputGainKnob,
                 mixKnob, preFilterKnob, postFilterKnob;

    std::unique_ptr<SliderAttachment> inputGainAtt, driveAtt, biasAtt,
                                      outputGainAtt, mixAtt, preFilterAtt, postFilterAtt;

    // Mode selector
    juce::ComboBox modeBox;
    std::unique_ptr<ComboBoxAttachment> modeAtt;

    // Oversampling selector
    juce::ComboBox oversampleBox;
    std::unique_ptr<ComboBoxAttachment> oversampleAtt;

    // DC block toggle
    juce::ToggleButton dcBlockBtn{"DC Block"};
    std::unique_ptr<ButtonAttachment> dcBlockAtt;

    // Labels
    juce::Label inputGainLabel, driveLabel, biasLabel, outputGainLabel,
                mixLabel, preFilterLabel, postFilterLabel, modeLabel, oversampleLabel;

    // Oscilloscope
    Oscilloscope oscilloscope;

    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGSaturatorEditor)
};

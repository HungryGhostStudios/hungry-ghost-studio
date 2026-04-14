#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "HGLookAndFeel.h"

class OscilloscopeDisplay : public juce::Component, private juce::Timer
{
public:
    OscilloscopeDisplay(HGSaturatorProcessor& proc) : processor(proc)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 3.0f);

        auto& apvts = processor.getAPVTS();
        float drive = *apvts.getRawParameterValue("drive") / 100.0f;
        float bias = *apvts.getRawParameterValue("bias");
        int modeIndex = static_cast<int>(*apvts.getRawParameterValue("mode"));

        // Draw input waveform (dim sine)
        juce::Path inputPath;
        juce::Path outputPath;

        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();

        for (int i = 0; i <= static_cast<int>(w); ++i)
        {
            float t = static_cast<float>(i) / w;
            float inputSample = std::sin(t * juce::MathConstants<float>::twoPi);

            // Simulate waveshaping for display
            float shaped = inputSample * (1.0f + drive * 4.0f) + bias;
            switch (modeIndex)
            {
                case 0: // Soft clip (tanh)
                    shaped = std::tanh(shaped);
                    break;
                case 1: // Hard clip
                    shaped = juce::jlimit(-1.0f, 1.0f, shaped);
                    break;
                case 2: // Tape
                    shaped = shaped / (1.0f + std::abs(shaped));
                    break;
                case 3: // Wavefold
                    shaped = std::sin(shaped * juce::MathConstants<float>::halfPi);
                    break;
                case 4: // Asymmetric
                    shaped = shaped >= 0.0f ? std::tanh(shaped) : std::tanh(shaped * 0.5f);
                    break;
                default:
                    shaped = std::tanh(shaped);
                    break;
            }

            float inY = cy - inputSample * (h * 0.4f);
            float outY = cy - shaped * (h * 0.4f);

            if (i == 0)
            {
                inputPath.startNewSubPath(bounds.getX() + static_cast<float>(i), inY);
                outputPath.startNewSubPath(bounds.getX() + static_cast<float>(i), outY);
            }
            else
            {
                inputPath.lineTo(bounds.getX() + static_cast<float>(i), inY);
                outputPath.lineTo(bounds.getX() + static_cast<float>(i), outY);
            }
        }

        // Draw grid
        g.setColour(juce::Colour(0xFF3A3A3A));
        g.drawHorizontalLine(static_cast<int>(cy), bounds.getX(), bounds.getRight());

        // Input waveform (dim)
        g.setColour(juce::Colour(0xFF666666));
        g.strokePath(inputPath, juce::PathStrokeType(1.0f));

        // Output waveform (bright orange)
        g.setColour(juce::Colour(HGLookAndFeel::saturatorAccent));
        g.strokePath(outputPath, juce::PathStrokeType(2.0f));
    }

private:
    void timerCallback() override { repaint(); }
    HGSaturatorProcessor& processor;
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
    HGLookAndFeel lookAndFeel{juce::Colour(HGLookAndFeel::saturatorAccent)};

    // Knobs
    juce::Slider inputGainKnob, driveKnob, biasKnob, outputGainKnob,
                 mixKnob, preFilterKnob, postFilterKnob;

    std::unique_ptr<SliderAttachment> inputGainAtt, driveAtt, biasAtt, outputGainAtt,
                                      mixAtt, preFilterAtt, postFilterAtt;

    // Labels
    juce::Label inputGainLabel, driveLabel, biasLabel, outputGainLabel,
                mixLabel, preFilterLabel, postFilterLabel;

    // Mode selector
    juce::ComboBox modeBox;
    std::unique_ptr<ComboBoxAttachment> modeAtt;
    juce::Label modeLabel;

    // Oversample selector
    juce::ComboBox oversampleBox;
    std::unique_ptr<ComboBoxAttachment> oversampleAtt;
    juce::Label oversampleLabel;

    // DC block toggle
    juce::ToggleButton dcBlockBtn{"DC Block"};
    std::unique_ptr<ButtonAttachment> dcBlockAtt;

    // Oscilloscope
    OscilloscopeDisplay oscilloscope;

    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGSaturatorEditor)
};

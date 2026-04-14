#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "HGLookAndFeel.h"

class IRVisualization : public juce::Component, private juce::Timer
{
public:
    IRVisualization(HGReverbProcessor& proc) : processor(proc)
    {
        startTimerHz(30);
        captureIR();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 3.0f);

        if (irEnvelope.empty())
            return;

        // Draw IR envelope waveform
        juce::Path path;
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float midY = bounds.getCentreY();
        const int numPoints = static_cast<int>(irEnvelope.size());

        bool started = false;
        for (int i = 0; i < numPoints; ++i)
        {
            float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * w;
            float y = midY - irEnvelope[static_cast<size_t>(i)] * (h * 0.45f);

            if (!started) { path.startNewSubPath(x, y); started = true; }
            else path.lineTo(x, y);
        }

        // Mirror for bottom half
        for (int i = numPoints - 1; i >= 0; --i)
        {
            float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * w;
            float y = midY + irEnvelope[static_cast<size_t>(i)] * (h * 0.45f);
            path.lineTo(x, y);
        }
        path.closeSubPath();

        g.setColour(juce::Colour(0xFF26A69A).withAlpha(0.3f));
        g.fillPath(path);
        g.setColour(juce::Colour(0xFF26A69A));
        g.strokePath(path, juce::PathStrokeType(1.0f));

        // Section labels
        g.setColour(juce::Colour(0xFF808080));
        g.setFont(10.0f);
        g.drawText("EARLY", bounds.removeFromLeft(bounds.getWidth() * 0.3f).reduced(4, 2),
                   juce::Justification::topLeft);
        g.drawText("LATE", bounds.reduced(4, 2), juce::Justification::topLeft);
    }

    void captureIR()
    {
        // Generate a synthetic IR envelope based on current parameters
        auto& apvts = processor.apvts;
        float predelayMs = *apvts.getRawParameterValue("predelay");
        float decay = *apvts.getRawParameterValue("decay");
        float size = *apvts.getRawParameterValue("size") / 100.0f;
        float damping = *apvts.getRawParameterValue("damping");
        float earlyDb = *apvts.getRawParameterValue("early_level");
        float lateDb = *apvts.getRawParameterValue("late_level");
        bool freeze = *apvts.getRawParameterValue("freeze") > 0.5f;

        const int numPoints = 200;
        irEnvelope.resize(static_cast<size_t>(numPoints));

        float earlyGain = (earlyDb <= -59.9f) ? 0.0f : std::pow(10.0f, earlyDb / 20.0f);
        float lateGain = (lateDb <= -59.9f) ? 0.0f : std::pow(10.0f, lateDb / 20.0f);

        // Total display duration: max(decay * 3, 1) seconds
        float totalSec = freeze ? 5.0f : std::max(decay * 3.0f, 1.0f);
        float predelaySec = predelayMs / 1000.0f;
        float earlyEndSec = predelaySec + 0.05f * (1.0f + size * 2.0f);

        for (int i = 0; i < numPoints; ++i)
        {
            float t = (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * totalSec;
            float env = 0.0f;

            if (t < predelaySec)
            {
                env = 0.0f;
            }
            else if (t < earlyEndSec)
            {
                // Early reflections: quick series of taps decaying
                float earlyT = (t - predelaySec) / (earlyEndSec - predelaySec);
                env = earlyGain * (1.0f - earlyT * 0.5f);
                // Simulate discrete reflections
                float tapPhase = earlyT * 8.0f;
                env *= 0.5f + 0.5f * std::abs(std::sin(tapPhase * 3.14159f));
            }
            else
            {
                // Late reverb: exponential decay based on RT60
                float lateT = t - earlyEndSec;
                if (freeze)
                {
                    env = lateGain;
                }
                else
                {
                    // RT60: -60dB in 'decay' seconds
                    float rt60Decay = std::exp(-6.9078f * lateT / std::max(decay, 0.01f));
                    // Damping reduces HF, approximate as faster decay of higher components
                    float dampFactor = 1.0f - (1.0f - damping / 10000.0f) * 0.3f;
                    env = lateGain * rt60Decay * dampFactor;
                }
            }

            irEnvelope[static_cast<size_t>(i)] = juce::jlimit(0.0f, 1.0f, env);
        }

        repaint();
    }

private:
    void timerCallback() override
    {
        // Recapture IR when parameters change
        captureIR();
    }

    HGReverbProcessor& processor;
    std::vector<float> irEnvelope;
};

class HGReverbEditor : public juce::AudioProcessorEditor
{
public:
    explicit HGReverbEditor(HGReverbProcessor& p);
    ~HGReverbEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    HGReverbProcessor& processorRef;
    HGLookAndFeel lookAndFeel{juce::Colour(0xFF26A69A)};

    // Knobs
    juce::Slider predelayKnob, sizeKnob, decayKnob, dampingKnob, diffusionKnob,
                 modRateKnob, modDepthKnob, earlyLevelKnob, lateLevelKnob,
                 widthKnob, mixKnob;

    std::unique_ptr<SliderAttachment> predelayAtt, sizeAtt, decayAtt, dampingAtt,
                                      diffusionAtt, modRateAtt, modDepthAtt,
                                      earlyLevelAtt, lateLevelAtt, widthAtt, mixAtt;

    // Freeze toggle
    juce::ToggleButton freezeBtn{"FREEZE"};
    std::unique_ptr<ButtonAttachment> freezeAtt;

    // Labels
    juce::Label predelayLabel, sizeLabel, decayLabel, dampingLabel, diffusionLabel,
                modRateLabel, modDepthLabel, earlyLevelLabel, lateLevelLabel,
                widthLabel, mixLabel;

    // IR Visualization
    IRVisualization irVis;

    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGReverbEditor)
};

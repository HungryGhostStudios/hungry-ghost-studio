#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "HGLookAndFeel.h"

class IRVisualization : public juce::Component, private juce::Timer
{
public:
    explicit IRVisualization(HGReverbProcessor& proc) : processor(proc)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 3.0f);

        if (irEnvelope.empty())
            return;

        // Draw IR waveform envelope
        juce::Path path;
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float midY = bounds.getCentreY();
        const int numPoints = static_cast<int>(irEnvelope.size());

        path.startNewSubPath(bounds.getX(), midY);
        for (int i = 0; i < numPoints; ++i)
        {
            float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numPoints)) * w;
            float y = midY - irEnvelope[static_cast<size_t>(i)] * (h * 0.45f);
            path.lineTo(x, y);
        }

        // Mirror for bottom half
        for (int i = numPoints - 1; i >= 0; --i)
        {
            float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numPoints)) * w;
            float y = midY + irEnvelope[static_cast<size_t>(i)] * (h * 0.45f);
            path.lineTo(x, y);
        }
        path.closeSubPath();

        g.setColour(juce::Colour(0xFF26A69A).withAlpha(0.4f));
        g.fillPath(path);
        g.setColour(juce::Colour(0xFF26A69A));
        g.strokePath(path, juce::PathStrokeType(1.0f));

        // Label sections
        g.setColour(juce::Colour(0xAAE0E0E0));
        g.setFont(10.0f);
        g.drawText("Early", bounds.removeFromLeft(bounds.getWidth() * 0.3f).translated(4, 4).withHeight(14), juce::Justification::topLeft);
        g.drawText("Late", bounds.translated(4, 4).withHeight(14), juce::Justification::topLeft);
    }

    void updateIR()
    {
        // Generate a synthetic IR envelope based on current reverb parameters
        auto& apvts = processor.apvts;
        float decay = *apvts.getRawParameterValue("decay");
        float size = *apvts.getRawParameterValue("size") / 100.0f;
        float damping = *apvts.getRawParameterValue("damping");
        float earlyLevel = *apvts.getRawParameterValue("early_level");
        float lateLevel = *apvts.getRawParameterValue("late_level");
        float predelay = *apvts.getRawParameterValue("predelay");

        const int numPoints = 200;
        irEnvelope.resize(static_cast<size_t>(numPoints));

        float earlyGain = (earlyLevel <= -59.9f) ? 0.0f : std::pow(10.0f, earlyLevel / 20.0f);
        float lateGain = (lateLevel <= -59.9f) ? 0.0f : std::pow(10.0f, lateLevel / 20.0f);

        // Predelay portion (silence)
        int predelayPoints = static_cast<int>((predelay / 100.0f) * numPoints * 0.1f);

        // Decay rate in points
        float decayRate = 1.0f / (decay * 30.0f * (0.3f + size * 0.7f));
        float dampFactor = damping / 10000.0f;

        for (int i = 0; i < numPoints; ++i)
        {
            if (i < predelayPoints)
            {
                irEnvelope[static_cast<size_t>(i)] = 0.0f;
                continue;
            }

            float t = static_cast<float>(i - predelayPoints) / static_cast<float>(numPoints);
            float earlyPart = earlyGain * std::exp(-t * 15.0f);
            float latePart = lateGain * std::exp(-t * decayRate * 60.0f) * (1.0f - dampFactor * t);

            irEnvelope[static_cast<size_t>(i)] = juce::jlimit(0.0f, 1.0f, earlyPart + latePart);
        }

        repaint();
    }

private:
    void timerCallback() override { updateIR(); }
    HGReverbProcessor& processor;
    std::vector<float> irEnvelope;
};

class HGReverbEditor : public juce::AudioProcessorEditor
{
public:
    explicit HGReverbEditor(HGReverbProcessor& p);
    ~HGReverbEditor() override = default;

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

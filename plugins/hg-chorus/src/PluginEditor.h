#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "HGLookAndFeel.h"

class LFOPhaseWheel : public juce::Component, private juce::Timer
{
public:
    explicit LFOPhaseWheel(HGChorusProcessor& proc) : processor(proc)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        float diameter = std::min(bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        float radius = diameter * 0.5f;

        // Draw circle background
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillEllipse(centre.x - radius, centre.y - radius, diameter, diameter);

        // Draw circle outline
        g.setColour(juce::Colour(0xFF4CAF50).withAlpha(0.4f));
        g.drawEllipse(centre.x - radius, centre.y - radius, diameter, diameter, 1.5f);

        // Draw voice dots at their LFO phase positions
        auto& apvts = processor.getAPVTS();
        int numVoices = static_cast<int>(*apvts.getRawParameterValue("voices"));
        float rate = *apvts.getRawParameterValue("rate");
        float spread = *apvts.getRawParameterValue("spread");

        float dotRadius = 5.0f;
        float orbitRadius = radius * 0.75f;

        for (int v = 0; v < numVoices; ++v)
        {
            // Each voice has a phase offset based on spread
            float phaseOffset = (numVoices > 1)
                ? spread * static_cast<float>(v) / static_cast<float>(numVoices - 1) * juce::MathConstants<float>::twoPi
                : 0.0f;

            float angle = phaseAccumulator + phaseOffset - juce::MathConstants<float>::halfPi;
            float dotX = centre.x + orbitRadius * std::cos(angle);
            float dotY = centre.y + orbitRadius * std::sin(angle);

            float alpha = 0.5f + 0.5f * (static_cast<float>(v) / std::max(1.0f, static_cast<float>(numVoices - 1)));
            g.setColour(juce::Colour(0xFF4CAF50).withAlpha(alpha));
            g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        }

        // Centre dot
        g.setColour(juce::Colour(0xFF4CAF50));
        g.fillEllipse(centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);

        // Accumulate phase based on rate
        (void)rate; // used in timerCallback
    }

private:
    void timerCallback() override
    {
        float rate = *processor.getAPVTS().getRawParameterValue("rate");
        float dt = 1.0f / 30.0f; // 30 fps
        phaseAccumulator += juce::MathConstants<float>::twoPi * rate * dt;
        if (phaseAccumulator > juce::MathConstants<float>::twoPi)
            phaseAccumulator -= juce::MathConstants<float>::twoPi;
        repaint();
    }

    HGChorusProcessor& processor;
    float phaseAccumulator = 0.0f;
};

class HGChorusEditor : public juce::AudioProcessorEditor,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit HGChorusEditor(HGChorusProcessor& p);
    ~HGChorusEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateBBDVisibility();
    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text);

    HGChorusProcessor& processorRef;
    HGLookAndFeel lookAndFeel{juce::Colour(0xFF4CAF50)};

    // Knobs
    juce::Slider voicesKnob, rateKnob, depthKnob, delayKnob,
                 feedbackKnob, spreadKnob, mixKnob;
    juce::Slider bbdStagesKnob, bbdNoiseKnob;

    std::unique_ptr<SliderAttachment> voicesAtt, rateAtt, depthAtt, delayAtt,
                                      feedbackAtt, spreadAtt, mixAtt,
                                      bbdStagesAtt, bbdNoiseAtt;

    // Mode selector
    juce::ComboBox modeBox;
    std::unique_ptr<ComboBoxAttachment> modeAtt;

    // Labels
    juce::Label voicesLabel, rateLabel, depthLabel, delayLabel,
                feedbackLabel, spreadLabel, mixLabel,
                bbdStagesLabel, bbdNoiseLabel, modeLabel;

    // LFO Phase Wheel
    LFOPhaseWheel phaseWheel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGChorusEditor)
};

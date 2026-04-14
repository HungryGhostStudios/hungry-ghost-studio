#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "HGLookAndFeel.h"

class GainReductionMeter : public juce::Component, private juce::Timer
{
public:
    GainReductionMeter(HGCompressorProcessor& proc) : processor(proc) { startTimerHz(30); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 3.0f);

        // GR is negative dB, map -30..0 dB to full height
        float grDb = processor.getGainReduction();
        float proportion = juce::jlimit(0.0f, 1.0f, -grDb / 30.0f);

        auto meterBounds = bounds.reduced(2.0f);
        auto filled = meterBounds.removeFromBottom(meterBounds.getHeight() * proportion);
        g.setColour(juce::Colour(0xFFFFB300));
        g.fillRoundedRectangle(filled, 2.0f);
    }

private:
    void timerCallback() override { repaint(); }
    HGCompressorProcessor& processor;
};

class TransferCurve : public juce::Component
{
public:
    TransferCurve(juce::AudioProcessorValueTreeState& apvts) : m_apvts(apvts) {}

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRoundedRectangle(bounds, 3.0f);

        float threshold = *m_apvts.getRawParameterValue("threshold");
        float ratio = *m_apvts.getRawParameterValue("ratio");
        float knee = *m_apvts.getRawParameterValue("knee");

        const float minDb = -60.0f;
        const float maxDb = 0.0f;

        auto toX = [&](float db) { return bounds.getX() + (db - minDb) / (maxDb - minDb) * bounds.getWidth(); };
        auto toY = [&](float db) { return bounds.getBottom() - (db - minDb) / (maxDb - minDb) * bounds.getHeight(); };

        // Grid lines
        g.setColour(juce::Colour(0xFF3A3A3A));
        for (float db = -50.0f; db <= 0.0f; db += 10.0f) {
            g.drawHorizontalLine(static_cast<int>(toY(db)), bounds.getX(), bounds.getRight());
            g.drawVerticalLine(static_cast<int>(toX(db)), bounds.getY(), bounds.getBottom());
        }

        // Unity line
        g.setColour(juce::Colour(0xFF555555));
        g.drawLine(bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getY(), 1.0f);

        // Transfer curve
        juce::Path curve;
        bool started = false;
        for (float inDb = minDb; inDb <= maxDb; inDb += 0.5f) {
            float outDb;
            float halfKnee = knee * 0.5f;
            if (inDb < threshold - halfKnee) {
                outDb = inDb;
            } else if (inDb > threshold + halfKnee) {
                outDb = threshold + (inDb - threshold) / ratio;
            } else {
                float x = inDb - threshold + halfKnee;
                outDb = inDb + (1.0f / ratio - 1.0f) * x * x / (2.0f * knee);
            }
            float px = toX(inDb), py = toY(outDb);
            if (!started) { curve.startNewSubPath(px, py); started = true; }
            else curve.lineTo(px, py);
        }
        g.setColour(juce::Colour(0xFFFFB300));
        g.strokePath(curve, juce::PathStrokeType(2.0f));
    }

private:
    juce::AudioProcessorValueTreeState& m_apvts;
};

class HGCompressorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HGCompressorEditor(HGCompressorProcessor& p);
    ~HGCompressorEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    HGCompressorProcessor& processorRef;
    HGLookAndFeel lookAndFeel{juce::Colour(0xFFFFB300)};

    // Knobs
    juce::Slider inputGainKnob, thresholdKnob, ratioKnob, attackKnob, releaseKnob,
                 kneeKnob, makeupKnob, mixKnob, lookaheadKnob;

    std::unique_ptr<SliderAttachment> inputGainAtt, thresholdAtt, ratioAtt, attackAtt,
                                      releaseAtt, kneeAtt, makeupAtt, mixAtt, lookaheadAtt;

    // Combo boxes
    juce::ComboBox modeBox, detectorBox, msModeBox;
    std::unique_ptr<ComboBoxAttachment> modeAtt, detectorAtt, msModeAtt;

    // Stereo link toggle
    juce::ToggleButton stereoLinkBtn{"Link"};
    std::unique_ptr<ButtonAttachment> stereoLinkAtt;

    // Labels
    juce::Label inputGainLabel, thresholdLabel, ratioLabel, attackLabel, releaseLabel,
                kneeLabel, makeupLabel, mixLabel, lookaheadLabel, modeLabel, detectorLabel,
                msModeLabel;

    // Components
    GainReductionMeter grMeter;
    TransferCurve transferCurve;

    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGCompressorEditor)
};

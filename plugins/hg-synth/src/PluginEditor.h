#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "HGLookAndFeel.h"

class HGSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit HGSynthEditor(HGSynthProcessor& p);
    ~HGSynthEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    HGSynthProcessor& processorRef;
    HGLookAndFeel lookAndFeel{juce::Colour(HGLookAndFeel::synthAccent)};

    // ── Osc 1 ──
    juce::ComboBox osc1WaveBox;
    juce::Slider osc1TuneKnob, osc1PwKnob, osc1LevelKnob;
    std::unique_ptr<ComboBoxAttachment> osc1WaveAtt;
    std::unique_ptr<SliderAttachment> osc1TuneAtt, osc1PwAtt, osc1LevelAtt;
    juce::Label osc1TuneLabel, osc1PwLabel, osc1LevelLabel;

    // ── Osc 2 ──
    juce::ComboBox osc2WaveBox;
    juce::Slider osc2TuneKnob, osc2PwKnob, osc2LevelKnob;
    std::unique_ptr<ComboBoxAttachment> osc2WaveAtt;
    std::unique_ptr<SliderAttachment> osc2TuneAtt, osc2PwAtt, osc2LevelAtt;
    juce::Label osc2TuneLabel, osc2PwLabel, osc2LevelLabel;

    // ── Filter ──
    juce::Slider cutoffKnob, resKnob, envAmtKnob, lfoAmtKnob, kbTrackKnob, driveKnob;
    juce::ComboBox filterTypeBox;
    std::unique_ptr<SliderAttachment> cutoffAtt, resAtt, envAmtAtt, lfoAmtAtt, kbTrackAtt, driveAtt;
    std::unique_ptr<ComboBoxAttachment> filterTypeAtt;
    juce::Label cutoffLabel, resLabel, envAmtLabel, lfoAmtLabel, kbTrackLabel, driveLabel;

    // ── Amp ADSR ──
    juce::Slider ampAKnob, ampDKnob, ampSKnob, ampRKnob;
    std::unique_ptr<SliderAttachment> ampAAtt, ampDAtt, ampSAtt, ampRAtt;
    juce::Label ampALabel, ampDLabel, ampSLabel, ampRLabel;

    // ── Filter ADSR ──
    juce::Slider fltAKnob, fltDKnob, fltSKnob, fltRKnob;
    std::unique_ptr<SliderAttachment> fltAAtt, fltDAtt, fltSAtt, fltRAtt;
    juce::Label fltALabel, fltDLabel, fltSLabel, fltRLabel;

    // ── LFO 1 ──
    juce::Slider lfo1RateKnob, lfo1AmtKnob;
    juce::ComboBox lfo1WaveBox, lfo1DestBox;
    std::unique_ptr<SliderAttachment> lfo1RateAtt, lfo1AmtAtt;
    std::unique_ptr<ComboBoxAttachment> lfo1WaveAtt, lfo1DestAtt;
    juce::Label lfo1RateLabel, lfo1AmtLabel;

    // ── LFO 2 ──
    juce::Slider lfo2RateKnob, lfo2AmtKnob;
    juce::ComboBox lfo2WaveBox, lfo2DestBox;
    std::unique_ptr<SliderAttachment> lfo2RateAtt, lfo2AmtAtt;
    std::unique_ptr<ComboBoxAttachment> lfo2WaveAtt, lfo2DestAtt;
    juce::Label lfo2RateLabel, lfo2AmtLabel;

    // ── Master ──
    juce::Slider voicesKnob, glideKnob, pitchBendKnob, unisonKnob;
    std::unique_ptr<SliderAttachment> voicesAtt, glideAtt, pitchBendAtt, unisonAtt;
    juce::Label voicesLabel, glideLabel, pitchBendLabel, unisonLabel;

    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text);
    void setupCombo(juce::ComboBox& box, const juce::StringArray& items);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGSynthEditor)
};

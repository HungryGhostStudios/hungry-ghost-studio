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
    HGLookAndFeel lookAndFeel{juce::Colour(0xFF9C27B0)};

    // ── Osc 1 ──
    juce::ComboBox osc1WaveBox;
    juce::Slider osc1TuneKnob, osc1PwKnob, osc1LevelKnob;
    juce::Label osc1WaveLabel, osc1TuneLabel, osc1PwLabel, osc1LevelLabel;
    std::unique_ptr<ComboBoxAttachment> osc1WaveAtt;
    std::unique_ptr<SliderAttachment> osc1TuneAtt, osc1PwAtt, osc1LevelAtt;

    // ── Osc 2 ──
    juce::ComboBox osc2WaveBox;
    juce::Slider osc2TuneKnob, osc2PwKnob, osc2LevelKnob;
    juce::Label osc2WaveLabel, osc2TuneLabel, osc2PwLabel, osc2LevelLabel;
    std::unique_ptr<ComboBoxAttachment> osc2WaveAtt;
    std::unique_ptr<SliderAttachment> osc2TuneAtt, osc2PwAtt, osc2LevelAtt;

    // ── Filter ──
    juce::Slider filterCutoffKnob, filterResKnob, filterEnvAmtKnob,
                 filterLfoAmtKnob, filterKbTrackKnob, filterDriveKnob;
    juce::ComboBox filterTypeBox;
    juce::Label filterCutoffLabel, filterResLabel, filterEnvAmtLabel,
                filterLfoAmtLabel, filterKbTrackLabel, filterDriveLabel, filterTypeLabel;
    std::unique_ptr<SliderAttachment> filterCutoffAtt, filterResAtt, filterEnvAmtAtt,
                                      filterLfoAmtAtt, filterKbTrackAtt, filterDriveAtt;
    std::unique_ptr<ComboBoxAttachment> filterTypeAtt;

    // ── Amp ADSR ──
    juce::Slider ampAKnob, ampDKnob, ampSKnob, ampRKnob;
    juce::Label ampALabel, ampDLabel, ampSLabel, ampRLabel;
    std::unique_ptr<SliderAttachment> ampAAtt, ampDAtt, ampSAtt, ampRAtt;

    // ── Filter ADSR ──
    juce::Slider fltAKnob, fltDKnob, fltSKnob, fltRKnob;
    juce::Label fltALabel, fltDLabel, fltSLabel, fltRLabel;
    std::unique_ptr<SliderAttachment> fltAAtt, fltDAtt, fltSAtt, fltRAtt;

    // ── LFO 1 ──
    juce::Slider lfo1RateKnob, lfo1AmountKnob;
    juce::ComboBox lfo1WaveBox, lfo1DestBox;
    juce::Label lfo1RateLabel, lfo1AmountLabel, lfo1WaveLabel, lfo1DestLabel;
    std::unique_ptr<SliderAttachment> lfo1RateAtt, lfo1AmountAtt;
    std::unique_ptr<ComboBoxAttachment> lfo1WaveAtt, lfo1DestAtt;

    // ── LFO 2 ──
    juce::Slider lfo2RateKnob, lfo2AmountKnob;
    juce::ComboBox lfo2WaveBox, lfo2DestBox;
    juce::Label lfo2RateLabel, lfo2AmountLabel, lfo2WaveLabel, lfo2DestLabel;
    std::unique_ptr<SliderAttachment> lfo2RateAtt, lfo2AmountAtt;
    std::unique_ptr<ComboBoxAttachment> lfo2WaveAtt, lfo2DestAtt;

    // ── Master ──
    juce::Slider voicesKnob, glideKnob, pitchBendKnob, unisonKnob;
    juce::Label voicesLabel, glideLabel, pitchBendLabel, unisonLabel;
    std::unique_ptr<SliderAttachment> voicesAtt, glideAtt, pitchBendAtt, unisonAtt;

    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text);
    void setupCombo(juce::ComboBox& box, juce::Label& label, const juce::String& text,
                    const juce::StringArray& items);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGSynthEditor)
};

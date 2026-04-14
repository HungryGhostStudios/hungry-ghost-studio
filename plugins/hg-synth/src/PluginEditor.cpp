#include "PluginEditor.h"

HGSynthEditor::HGSynthEditor(HGSynthProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(900, 560);

    auto& apvts = p.getAPVTS();

    // ── Osc 1 ──
    setupCombo(osc1WaveBox, osc1WaveLabel, "Wave", {"Saw", "Square", "Triangle", "Sine"});
    osc1WaveAtt = std::make_unique<ComboBoxAttachment>(apvts, "osc1_wave", osc1WaveBox);
    setupKnob(osc1TuneKnob, osc1TuneLabel, "Tune");
    osc1TuneAtt = std::make_unique<SliderAttachment>(apvts, "osc1_tune", osc1TuneKnob);
    setupKnob(osc1PwKnob, osc1PwLabel, "PW");
    osc1PwAtt = std::make_unique<SliderAttachment>(apvts, "osc1_pw", osc1PwKnob);
    setupKnob(osc1LevelKnob, osc1LevelLabel, "Level");
    osc1LevelAtt = std::make_unique<SliderAttachment>(apvts, "osc1_level", osc1LevelKnob);

    // ── Osc 2 ──
    setupCombo(osc2WaveBox, osc2WaveLabel, "Wave", {"Saw", "Square", "Triangle", "Sine"});
    osc2WaveAtt = std::make_unique<ComboBoxAttachment>(apvts, "osc2_wave", osc2WaveBox);
    setupKnob(osc2TuneKnob, osc2TuneLabel, "Tune");
    osc2TuneAtt = std::make_unique<SliderAttachment>(apvts, "osc2_tune", osc2TuneKnob);
    setupKnob(osc2PwKnob, osc2PwLabel, "PW");
    osc2PwAtt = std::make_unique<SliderAttachment>(apvts, "osc2_pw", osc2PwKnob);
    setupKnob(osc2LevelKnob, osc2LevelLabel, "Level");
    osc2LevelAtt = std::make_unique<SliderAttachment>(apvts, "osc2_level", osc2LevelKnob);

    // ── Filter ──
    setupKnob(filterCutoffKnob, filterCutoffLabel, "Cutoff");
    filterCutoffAtt = std::make_unique<SliderAttachment>(apvts, "filter_cutoff", filterCutoffKnob);
    setupKnob(filterResKnob, filterResLabel, "Res");
    filterResAtt = std::make_unique<SliderAttachment>(apvts, "filter_res", filterResKnob);
    setupKnob(filterEnvAmtKnob, filterEnvAmtLabel, "Env Amt");
    filterEnvAmtAtt = std::make_unique<SliderAttachment>(apvts, "filter_env_amt", filterEnvAmtKnob);
    setupKnob(filterLfoAmtKnob, filterLfoAmtLabel, "LFO Amt");
    filterLfoAmtAtt = std::make_unique<SliderAttachment>(apvts, "filter_lfo_amt", filterLfoAmtKnob);
    setupKnob(filterKbTrackKnob, filterKbTrackLabel, "KB Track");
    filterKbTrackAtt = std::make_unique<SliderAttachment>(apvts, "filter_kb_track", filterKbTrackKnob);
    setupKnob(filterDriveKnob, filterDriveLabel, "Drive");
    filterDriveAtt = std::make_unique<SliderAttachment>(apvts, "filter_drive", filterDriveKnob);
    setupCombo(filterTypeBox, filterTypeLabel, "Type", {"LP24", "HP24"});
    filterTypeAtt = std::make_unique<ComboBoxAttachment>(apvts, "filter_type", filterTypeBox);

    // ── Amp ADSR ──
    setupKnob(ampAKnob, ampALabel, "A");
    ampAAtt = std::make_unique<SliderAttachment>(apvts, "amp_attack", ampAKnob);
    setupKnob(ampDKnob, ampDLabel, "D");
    ampDAtt = std::make_unique<SliderAttachment>(apvts, "amp_decay", ampDKnob);
    setupKnob(ampSKnob, ampSLabel, "S");
    ampSAtt = std::make_unique<SliderAttachment>(apvts, "amp_sustain", ampSKnob);
    setupKnob(ampRKnob, ampRLabel, "R");
    ampRAtt = std::make_unique<SliderAttachment>(apvts, "amp_release", ampRKnob);

    // ── Filter ADSR ──
    setupKnob(fltAKnob, fltALabel, "A");
    fltAAtt = std::make_unique<SliderAttachment>(apvts, "filter_attack", fltAKnob);
    setupKnob(fltDKnob, fltDLabel, "D");
    fltDAtt = std::make_unique<SliderAttachment>(apvts, "filter_decay", fltDKnob);
    setupKnob(fltSKnob, fltSLabel, "S");
    fltSAtt = std::make_unique<SliderAttachment>(apvts, "filter_sustain", fltSKnob);
    setupKnob(fltRKnob, fltRLabel, "R");
    fltRAtt = std::make_unique<SliderAttachment>(apvts, "filter_release", fltRKnob);

    // ── LFO 1 ──
    setupKnob(lfo1RateKnob, lfo1RateLabel, "Rate");
    lfo1RateAtt = std::make_unique<SliderAttachment>(apvts, "lfo1_rate", lfo1RateKnob);
    setupKnob(lfo1AmountKnob, lfo1AmountLabel, "Amount");
    lfo1AmountAtt = std::make_unique<SliderAttachment>(apvts, "lfo1_amount", lfo1AmountKnob);
    setupCombo(lfo1WaveBox, lfo1WaveLabel, "Wave", {"Sine", "Triangle", "Saw", "Square", "S&H"});
    lfo1WaveAtt = std::make_unique<ComboBoxAttachment>(apvts, "lfo1_wave", lfo1WaveBox);
    setupCombo(lfo1DestBox, lfo1DestLabel, "Dest", {"Pitch", "Filter", "Amp"});
    lfo1DestAtt = std::make_unique<ComboBoxAttachment>(apvts, "lfo1_dest", lfo1DestBox);

    // ── LFO 2 ──
    setupKnob(lfo2RateKnob, lfo2RateLabel, "Rate");
    lfo2RateAtt = std::make_unique<SliderAttachment>(apvts, "lfo2_rate", lfo2RateKnob);
    setupKnob(lfo2AmountKnob, lfo2AmountLabel, "Amount");
    lfo2AmountAtt = std::make_unique<SliderAttachment>(apvts, "lfo2_amount", lfo2AmountKnob);
    setupCombo(lfo2WaveBox, lfo2WaveLabel, "Wave", {"Sine", "Triangle", "Saw", "Square", "S&H"});
    lfo2WaveAtt = std::make_unique<ComboBoxAttachment>(apvts, "lfo2_wave", lfo2WaveBox);
    setupCombo(lfo2DestBox, lfo2DestLabel, "Dest", {"Pitch", "Filter", "Amp"});
    lfo2DestAtt = std::make_unique<ComboBoxAttachment>(apvts, "lfo2_dest", lfo2DestBox);

    // ── Master ──
    setupKnob(voicesKnob, voicesLabel, "Voices");
    voicesAtt = std::make_unique<SliderAttachment>(apvts, "master_voices", voicesKnob);
    setupKnob(glideKnob, glideLabel, "Glide");
    glideAtt = std::make_unique<SliderAttachment>(apvts, "master_glide", glideKnob);
    setupKnob(pitchBendKnob, pitchBendLabel, "P.Bend");
    pitchBendAtt = std::make_unique<SliderAttachment>(apvts, "master_pitch_bend", pitchBendKnob);
    setupKnob(unisonKnob, unisonLabel, "Unison");
    unisonAtt = std::make_unique<SliderAttachment>(apvts, "master_unison", unisonKnob);
}

HGSynthEditor::~HGSynthEditor()
{
    setLookAndFeel(nullptr);
}

void HGSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));

    auto accent = juce::Colour(0xFF9C27B0);
    g.setColour(accent);
    g.setFont(13.0f);

    // Section headers
    g.drawText("OSC 1", 10, 5, 200, 18, juce::Justification::centredLeft);
    g.drawText("OSC 2", 230, 5, 200, 18, juce::Justification::centredLeft);
    g.drawText("FILTER", 460, 5, 200, 18, juce::Justification::centredLeft);
    g.drawText("AMP ENV", 10, 200, 200, 18, juce::Justification::centredLeft);
    g.drawText("FILTER ENV", 230, 200, 200, 18, juce::Justification::centredLeft);
    g.drawText("LFO 1", 460, 200, 200, 18, juce::Justification::centredLeft);
    g.drawText("LFO 2", 660, 200, 200, 18, juce::Justification::centredLeft);
    g.drawText("MASTER", 10, 400, 200, 18, juce::Justification::centredLeft);

    // Section dividers
    g.setColour(juce::Colour(0xFF3A3A3A));
    g.drawHorizontalLine(195, 10, 890);
    g.drawHorizontalLine(395, 10, 890);
}

void HGSynthEditor::resized()
{
    const int knobW = 55;
    const int knobH = 55;
    const int labelH = 14;
    const int comboH = 22;
    const int gap = 4;
    const int sectionY = 25;

    // ═══ Row 1: Oscillators + Filter (y=25 to ~190) ═══

    // ── Osc 1 (x=10) ──
    int x = 10, y = sectionY;
    osc1WaveLabel.setBounds(x, y, 80, labelH);
    osc1WaveBox.setBounds(x, y + labelH, 80, comboH);
    y += labelH + comboH + gap;
    osc1TuneLabel.setBounds(x, y, knobW, labelH);
    osc1TuneKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    osc1PwLabel.setBounds(x, y, knobW, labelH);
    osc1PwKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    osc1LevelLabel.setBounds(x, y, knobW, labelH);
    osc1LevelKnob.setBounds(x, y + labelH, knobW, knobH);

    // ── Osc 2 (x=230) ──
    x = 230; y = sectionY;
    osc2WaveLabel.setBounds(x, y, 80, labelH);
    osc2WaveBox.setBounds(x, y + labelH, 80, comboH);
    y += labelH + comboH + gap;
    osc2TuneLabel.setBounds(x, y, knobW, labelH);
    osc2TuneKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    osc2PwLabel.setBounds(x, y, knobW, labelH);
    osc2PwKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    osc2LevelLabel.setBounds(x, y, knobW, labelH);
    osc2LevelKnob.setBounds(x, y + labelH, knobW, knobH);

    // ── Filter (x=460) ──
    x = 460; y = sectionY;
    filterTypeLabel.setBounds(x, y, 80, labelH);
    filterTypeBox.setBounds(x, y + labelH, 80, comboH);
    y += labelH + comboH + gap;

    filterCutoffLabel.setBounds(x, y, knobW, labelH);
    filterCutoffKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    filterResLabel.setBounds(x, y, knobW, labelH);
    filterResKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    filterDriveLabel.setBounds(x, y, knobW, labelH);
    filterDriveKnob.setBounds(x, y + labelH, knobW, knobH);

    x = 460; y += labelH + knobH + gap;
    filterEnvAmtLabel.setBounds(x, y, knobW, labelH);
    filterEnvAmtKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    filterLfoAmtLabel.setBounds(x, y, knobW, labelH);
    filterLfoAmtKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    filterKbTrackLabel.setBounds(x, y, knobW, labelH);
    filterKbTrackKnob.setBounds(x, y + labelH, knobW, knobH);

    // ═══ Row 2: Envelopes + LFOs (y=220 to ~390) ═══
    const int row2Y = 220;

    // ── Amp ADSR (x=10) ──
    x = 10; y = row2Y;
    ampALabel.setBounds(x, y, knobW, labelH);
    ampAKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    ampDLabel.setBounds(x, y, knobW, labelH);
    ampDKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    ampSLabel.setBounds(x, y, knobW, labelH);
    ampSKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    ampRLabel.setBounds(x, y, knobW, labelH);
    ampRKnob.setBounds(x, y + labelH, knobW, knobH);

    // ── Filter ADSR (x=230) ──
    x = 230; y = row2Y;
    fltALabel.setBounds(x, y, knobW, labelH);
    fltAKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    fltDLabel.setBounds(x, y, knobW, labelH);
    fltDKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    fltSLabel.setBounds(x, y, knobW, labelH);
    fltSKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    fltRLabel.setBounds(x, y, knobW, labelH);
    fltRKnob.setBounds(x, y + labelH, knobW, knobH);

    // ── LFO 1 (x=460) ──
    x = 460; y = row2Y;
    lfo1WaveLabel.setBounds(x, y, 80, labelH);
    lfo1WaveBox.setBounds(x, y + labelH, 80, comboH);
    x += 84;
    lfo1DestLabel.setBounds(x, y, 80, labelH);
    lfo1DestBox.setBounds(x, y + labelH, 80, comboH);

    x = 460; y += labelH + comboH + gap;
    lfo1RateLabel.setBounds(x, y, knobW, labelH);
    lfo1RateKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    lfo1AmountLabel.setBounds(x, y, knobW, labelH);
    lfo1AmountKnob.setBounds(x, y + labelH, knobW, knobH);

    // ── LFO 2 (x=660) ──
    x = 660; y = row2Y;
    lfo2WaveLabel.setBounds(x, y, 80, labelH);
    lfo2WaveBox.setBounds(x, y + labelH, 80, comboH);
    x += 84;
    lfo2DestLabel.setBounds(x, y, 80, labelH);
    lfo2DestBox.setBounds(x, y + labelH, 80, comboH);

    x = 660; y += labelH + comboH + gap;
    lfo2RateLabel.setBounds(x, y, knobW, labelH);
    lfo2RateKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    lfo2AmountLabel.setBounds(x, y, knobW, labelH);
    lfo2AmountKnob.setBounds(x, y + labelH, knobW, knobH);

    // ═══ Row 3: Master (y=420) ═══
    const int row3Y = 420;
    x = 10; y = row3Y;
    voicesLabel.setBounds(x, y, knobW, labelH);
    voicesKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    glideLabel.setBounds(x, y, knobW, labelH);
    glideKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    pitchBendLabel.setBounds(x, y, knobW, labelH);
    pitchBendKnob.setBounds(x, y + labelH, knobW, knobH);
    x += knobW + gap;
    unisonLabel.setBounds(x, y, knobW, labelH);
    unisonKnob.setBounds(x, y + labelH, knobW, knobH);
}

void HGSynthEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
{
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 14);
    addAndMakeVisible(knob);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    label.setFont(juce::Font(11.0f));
    addAndMakeVisible(label);
}

void HGSynthEditor::setupCombo(juce::ComboBox& box, juce::Label& label,
                                const juce::String& text, const juce::StringArray& items)
{
    box.addItemList(items, 1);
    addAndMakeVisible(box);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    label.setFont(juce::Font(11.0f));
    addAndMakeVisible(label);
}

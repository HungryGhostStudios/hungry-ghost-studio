#include "PluginEditor.h"

HGSynthEditor::HGSynthEditor(HGSynthProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(800, 500);

    auto& apvts = p.getAPVTS();

    // ── Osc 1 ──
    setupCombo(osc1WaveBox, {"Saw", "Square", "Triangle", "Sine"});
    osc1WaveAtt = std::make_unique<ComboBoxAttachment>(apvts, "osc1_wave", osc1WaveBox);
    setupKnob(osc1TuneKnob, osc1TuneLabel, "Tune");
    osc1TuneAtt = std::make_unique<SliderAttachment>(apvts, "osc1_tune", osc1TuneKnob);
    setupKnob(osc1PwKnob, osc1PwLabel, "PW");
    osc1PwAtt = std::make_unique<SliderAttachment>(apvts, "osc1_pw", osc1PwKnob);
    setupKnob(osc1LevelKnob, osc1LevelLabel, "Level");
    osc1LevelAtt = std::make_unique<SliderAttachment>(apvts, "osc1_level", osc1LevelKnob);

    // ── Osc 2 ──
    setupCombo(osc2WaveBox, {"Saw", "Square", "Triangle", "Sine"});
    osc2WaveAtt = std::make_unique<ComboBoxAttachment>(apvts, "osc2_wave", osc2WaveBox);
    setupKnob(osc2TuneKnob, osc2TuneLabel, "Tune");
    osc2TuneAtt = std::make_unique<SliderAttachment>(apvts, "osc2_tune", osc2TuneKnob);
    setupKnob(osc2PwKnob, osc2PwLabel, "PW");
    osc2PwAtt = std::make_unique<SliderAttachment>(apvts, "osc2_pw", osc2PwKnob);
    setupKnob(osc2LevelKnob, osc2LevelLabel, "Level");
    osc2LevelAtt = std::make_unique<SliderAttachment>(apvts, "osc2_level", osc2LevelKnob);

    // ── Filter ──
    setupKnob(cutoffKnob, cutoffLabel, "Cutoff");
    cutoffAtt = std::make_unique<SliderAttachment>(apvts, "filter_cutoff", cutoffKnob);
    setupKnob(resKnob, resLabel, "Res");
    resAtt = std::make_unique<SliderAttachment>(apvts, "filter_res", resKnob);
    setupKnob(envAmtKnob, envAmtLabel, "Env");
    envAmtAtt = std::make_unique<SliderAttachment>(apvts, "filter_env_amt", envAmtKnob);
    setupKnob(lfoAmtKnob, lfoAmtLabel, "LFO");
    lfoAmtAtt = std::make_unique<SliderAttachment>(apvts, "filter_lfo_amt", lfoAmtKnob);
    setupKnob(kbTrackKnob, kbTrackLabel, "KB");
    kbTrackAtt = std::make_unique<SliderAttachment>(apvts, "filter_kb_track", kbTrackKnob);
    setupKnob(driveKnob, driveLabel, "Drive");
    driveAtt = std::make_unique<SliderAttachment>(apvts, "filter_drive", driveKnob);
    setupCombo(filterTypeBox, {"LP24", "HP24"});
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
    setupKnob(lfo1AmtKnob, lfo1AmtLabel, "Amt");
    lfo1AmtAtt = std::make_unique<SliderAttachment>(apvts, "lfo1_amount", lfo1AmtKnob);
    setupCombo(lfo1WaveBox, {"Sine", "Triangle", "Saw", "Square", "S&H"});
    lfo1WaveAtt = std::make_unique<ComboBoxAttachment>(apvts, "lfo1_wave", lfo1WaveBox);
    setupCombo(lfo1DestBox, {"Pitch", "Filter", "Amplitude"});
    lfo1DestAtt = std::make_unique<ComboBoxAttachment>(apvts, "lfo1_dest", lfo1DestBox);

    // ── LFO 2 ──
    setupKnob(lfo2RateKnob, lfo2RateLabel, "Rate");
    lfo2RateAtt = std::make_unique<SliderAttachment>(apvts, "lfo2_rate", lfo2RateKnob);
    setupKnob(lfo2AmtKnob, lfo2AmtLabel, "Amt");
    lfo2AmtAtt = std::make_unique<SliderAttachment>(apvts, "lfo2_amount", lfo2AmtKnob);
    setupCombo(lfo2WaveBox, {"Sine", "Triangle", "Saw", "Square", "S&H"});
    lfo2WaveAtt = std::make_unique<ComboBoxAttachment>(apvts, "lfo2_wave", lfo2WaveBox);
    setupCombo(lfo2DestBox, {"Pitch", "Filter", "Amplitude"});
    lfo2DestAtt = std::make_unique<ComboBoxAttachment>(apvts, "lfo2_dest", lfo2DestBox);

    // ── Master ──
    setupKnob(voicesKnob, voicesLabel, "Voices");
    voicesAtt = std::make_unique<SliderAttachment>(apvts, "master_voices", voicesKnob);
    setupKnob(glideKnob, glideLabel, "Glide");
    glideAtt = std::make_unique<SliderAttachment>(apvts, "master_glide", glideKnob);
    setupKnob(pitchBendKnob, pitchBendLabel, "PB");
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

    const auto accent = juce::Colour(HGLookAndFeel::synthAccent);
    g.setColour(accent);
    g.setFont(14.0f);

    // Section headers
    g.drawText("OSC 1", 10, 5, 180, 20, juce::Justification::centred);
    g.drawText("OSC 2", 200, 5, 180, 20, juce::Justification::centred);
    g.drawText("FILTER", 390, 5, 200, 20, juce::Justification::centred);
    g.drawText("AMP ENV", 600, 5, 90, 20, juce::Justification::centred);
    g.drawText("FLT ENV", 700, 5, 90, 20, juce::Justification::centred);
    g.drawText("LFO 1", 10, 235, 180, 20, juce::Justification::centred);
    g.drawText("LFO 2", 200, 235, 180, 20, juce::Justification::centred);
    g.drawText("MASTER", 600, 235, 190, 20, juce::Justification::centred);

    // Section dividers
    g.setColour(juce::Colour(0xFF3A3A3A));
    g.drawHorizontalLine(230, 0.0f, static_cast<float>(getWidth()));
    g.drawVerticalLine(190, 5.0f, 225.0f);
    g.drawVerticalLine(385, 5.0f, 225.0f);
    g.drawVerticalLine(595, 5.0f, 225.0f);
    g.drawVerticalLine(695, 5.0f, 225.0f);
    g.drawVerticalLine(190, 235.0f, static_cast<float>(getHeight()));
    g.drawVerticalLine(385, 235.0f, static_cast<float>(getHeight()));
    g.drawVerticalLine(595, 235.0f, static_cast<float>(getHeight()));
}

void HGSynthEditor::resized()
{
    const int knobW = 55;
    const int knobH = 55;
    const int labelH = 14;
    const int comboH = 22;
    const int pad = 4;

    // ── Top row: Osc1 | Osc2 | Filter | Amp ADSR | Flt ADSR ──
    // Osc 1 (x=10..190)
    osc1WaveBox.setBounds(20, 28, 160, comboH);
    int oscY = 55;
    osc1TuneLabel.setBounds(15, oscY, knobW, labelH);
    osc1TuneKnob.setBounds(15, oscY + labelH, knobW, knobH);
    osc1PwLabel.setBounds(75, oscY, knobW, labelH);
    osc1PwKnob.setBounds(75, oscY + labelH, knobW, knobH);
    osc1LevelLabel.setBounds(135, oscY, knobW, labelH);
    osc1LevelKnob.setBounds(135, oscY + labelH, knobW, knobH);

    // Osc 2 (x=200..385)
    osc2WaveBox.setBounds(210, 28, 160, comboH);
    osc2TuneLabel.setBounds(205, oscY, knobW, labelH);
    osc2TuneKnob.setBounds(205, oscY + labelH, knobW, knobH);
    osc2PwLabel.setBounds(265, oscY, knobW, labelH);
    osc2PwKnob.setBounds(265, oscY + labelH, knobW, knobH);
    osc2LevelLabel.setBounds(325, oscY, knobW, labelH);
    osc2LevelKnob.setBounds(325, oscY + labelH, knobW, knobH);

    // Filter (x=390..595)
    int fltX = 392;
    int fltY1 = 28;
    filterTypeBox.setBounds(fltX, fltY1, 80, comboH);
    cutoffLabel.setBounds(fltX, oscY, knobW, labelH);
    cutoffKnob.setBounds(fltX, oscY + labelH, knobW, knobH);
    resLabel.setBounds(fltX + 55, oscY, knobW, labelH);
    resKnob.setBounds(fltX + 55, oscY + labelH, knobW, knobH);
    envAmtLabel.setBounds(fltX + 110, oscY, knobW, labelH);
    envAmtKnob.setBounds(fltX + 110, oscY + labelH, knobW, knobH);

    int fltY2 = oscY + labelH + knobH + pad;
    lfoAmtLabel.setBounds(fltX, fltY2, knobW, labelH);
    lfoAmtKnob.setBounds(fltX, fltY2 + labelH, knobW, knobH);
    kbTrackLabel.setBounds(fltX + 55, fltY2, knobW, labelH);
    kbTrackKnob.setBounds(fltX + 55, fltY2 + labelH, knobW, knobH);
    driveLabel.setBounds(fltX + 110, fltY2, knobW, labelH);
    driveKnob.setBounds(fltX + 110, fltY2 + labelH, knobW, knobH);

    // Amp ADSR (x=600..695)
    int envX = 602;
    ampALabel.setBounds(envX, oscY, 40, labelH);
    ampAKnob.setBounds(envX, oscY + labelH, 40, 40);
    ampDLabel.setBounds(envX + 44, oscY, 40, labelH);
    ampDKnob.setBounds(envX + 44, oscY + labelH, 40, 40);
    int envY2 = oscY + labelH + 44;
    ampSLabel.setBounds(envX, envY2, 40, labelH);
    ampSKnob.setBounds(envX, envY2 + labelH, 40, 40);
    ampRLabel.setBounds(envX + 44, envY2, 40, labelH);
    ampRKnob.setBounds(envX + 44, envY2 + labelH, 40, 40);

    // Filter ADSR (x=700..790)
    int fenvX = 702;
    fltALabel.setBounds(fenvX, oscY, 40, labelH);
    fltAKnob.setBounds(fenvX, oscY + labelH, 40, 40);
    fltDLabel.setBounds(fenvX + 44, oscY, 40, labelH);
    fltDKnob.setBounds(fenvX + 44, oscY + labelH, 40, 40);
    fltSLabel.setBounds(fenvX, envY2, 40, labelH);
    fltSKnob.setBounds(fenvX, envY2 + labelH, 40, 40);
    fltRLabel.setBounds(fenvX + 44, envY2, 40, labelH);
    fltRKnob.setBounds(fenvX + 44, envY2 + labelH, 40, 40);

    // ── Bottom row: LFO 1 | LFO 2 | (space) | Master ──
    int botY = 258;

    // LFO 1 (x=10..190)
    lfo1WaveBox.setBounds(20, botY, 75, comboH);
    lfo1DestBox.setBounds(100, botY, 80, comboH);
    int lfoKnobY = botY + comboH + pad;
    lfo1RateLabel.setBounds(30, lfoKnobY, knobW, labelH);
    lfo1RateKnob.setBounds(30, lfoKnobY + labelH, knobW, knobH);
    lfo1AmtLabel.setBounds(100, lfoKnobY, knobW, labelH);
    lfo1AmtKnob.setBounds(100, lfoKnobY + labelH, knobW, knobH);

    // LFO 2 (x=200..385)
    lfo2WaveBox.setBounds(210, botY, 75, comboH);
    lfo2DestBox.setBounds(290, botY, 80, comboH);
    lfo2RateLabel.setBounds(220, lfoKnobY, knobW, labelH);
    lfo2RateKnob.setBounds(220, lfoKnobY + labelH, knobW, knobH);
    lfo2AmtLabel.setBounds(290, lfoKnobY, knobW, labelH);
    lfo2AmtKnob.setBounds(290, lfoKnobY + labelH, knobW, knobH);

    // Master (x=600..790)
    int mastX = 605;
    voicesLabel.setBounds(mastX, botY, 40, labelH);
    voicesKnob.setBounds(mastX, botY + labelH, 40, 40);
    glideLabel.setBounds(mastX + 48, botY, 40, labelH);
    glideKnob.setBounds(mastX + 48, botY + labelH, 40, 40);
    pitchBendLabel.setBounds(mastX + 96, botY, 40, labelH);
    pitchBendKnob.setBounds(mastX + 96, botY + labelH, 40, 40);
    unisonLabel.setBounds(mastX + 144, botY, 40, labelH);
    unisonKnob.setBounds(mastX + 144, botY + labelH, 40, 40);
}

void HGSynthEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
{
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
    addAndMakeVisible(knob);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    label.setFont(juce::Font(11.0f));
    addAndMakeVisible(label);
}

void HGSynthEditor::setupCombo(juce::ComboBox& box, const juce::StringArray& items)
{
    box.addItemList(items, 1);
    addAndMakeVisible(box);
}

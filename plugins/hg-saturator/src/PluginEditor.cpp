#include "PluginEditor.h"

HGSaturatorEditor::HGSaturatorEditor(HGSaturatorProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(650, 380);

    // Setup knobs
    setupKnob(inputGainKnob, inputGainLabel, "Input");
    setupKnob(driveKnob, driveLabel, "Drive");
    setupKnob(biasKnob, biasLabel, "Bias");
    setupKnob(outputGainKnob, outputGainLabel, "Output");
    setupKnob(mixKnob, mixLabel, "Mix");
    setupKnob(preFilterKnob, preFilterLabel, "Pre Filter");
    setupKnob(postFilterKnob, postFilterLabel, "Post Filter");

    // APVTS attachments
    auto& apvts = p.getAPVTS();
    inputGainAtt  = std::make_unique<SliderAttachment>(apvts, "input_gain", inputGainKnob);
    driveAtt      = std::make_unique<SliderAttachment>(apvts, "drive", driveKnob);
    biasAtt       = std::make_unique<SliderAttachment>(apvts, "bias", biasKnob);
    outputGainAtt = std::make_unique<SliderAttachment>(apvts, "output_gain", outputGainKnob);
    mixAtt        = std::make_unique<SliderAttachment>(apvts, "mix", mixKnob);
    preFilterAtt  = std::make_unique<SliderAttachment>(apvts, "pre_filter_freq", preFilterKnob);
    postFilterAtt = std::make_unique<SliderAttachment>(apvts, "post_filter_freq", postFilterKnob);

    // Mode selector
    modeBox.addItemList({"Soft", "Hard", "Tape", "Fold", "Asym"}, 1);
    addAndMakeVisible(modeBox);
    modeAtt = std::make_unique<ComboBoxAttachment>(apvts, "mode", modeBox);
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    addAndMakeVisible(modeLabel);

    // Oversampling selector
    oversampleBox.addItemList({"1x", "2x", "4x", "8x"}, 1);
    addAndMakeVisible(oversampleBox);
    oversampleAtt = std::make_unique<ComboBoxAttachment>(apvts, "oversample", oversampleBox);
    oversampleLabel.setText("Oversample", juce::dontSendNotification);
    oversampleLabel.setJustificationType(juce::Justification::centred);
    oversampleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    addAndMakeVisible(oversampleLabel);

    // DC block toggle
    dcBlockBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFE0E0E0));
    dcBlockBtn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFFF9800));
    addAndMakeVisible(dcBlockBtn);
    dcBlockAtt = std::make_unique<ButtonAttachment>(apvts, "dc_block", dcBlockBtn);

    // Oscilloscope
    addAndMakeVisible(oscilloscope);
}

HGSaturatorEditor::~HGSaturatorEditor()
{
    setLookAndFeel(nullptr);
}

void HGSaturatorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));

    // Section labels
    g.setColour(juce::Colour(0xFFFF9800));
    g.setFont(14.0f);
    g.drawText("INPUT", 10, 5, 80, 20, juce::Justification::centred);
    g.drawText("SATURATION", 200, 5, 150, 20, juce::Justification::centred);
    g.drawText("OUTPUT", 530, 5, 110, 20, juce::Justification::centred);
}

void HGSaturatorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20); // section labels

    const int knobSize = 56;
    const int labelH = 16;

    // Oscilloscope at top
    auto scopeArea = area.removeFromTop(110);
    oscilloscope.setBounds(scopeArea);

    area.removeFromTop(8);

    // Row 1: Input Gain, Drive, Bias, Output Gain, Mix
    auto row1 = area.removeFromTop(knobSize + labelH);
    int knobW = row1.getWidth() / 5;

    auto col = row1.removeFromLeft(knobW);
    inputGainLabel.setBounds(col.removeFromTop(labelH));
    inputGainKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    driveLabel.setBounds(col.removeFromTop(labelH));
    driveKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    biasLabel.setBounds(col.removeFromTop(labelH));
    biasKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    outputGainLabel.setBounds(col.removeFromTop(labelH));
    outputGainKnob.setBounds(col.removeFromTop(knobSize));

    col = row1;
    mixLabel.setBounds(col.removeFromTop(labelH));
    mixKnob.setBounds(col.removeFromTop(knobSize));

    area.removeFromTop(8);

    // Row 2: Pre Filter, Post Filter, Mode, Oversample, DC Block
    auto row2 = area.removeFromTop(knobSize + labelH);
    int row2W = row2.getWidth() / 5;

    col = row2.removeFromLeft(row2W);
    preFilterLabel.setBounds(col.removeFromTop(labelH));
    preFilterKnob.setBounds(col.removeFromTop(knobSize));

    col = row2.removeFromLeft(row2W);
    postFilterLabel.setBounds(col.removeFromTop(labelH));
    postFilterKnob.setBounds(col.removeFromTop(knobSize));

    col = row2.removeFromLeft(row2W);
    modeLabel.setBounds(col.removeFromTop(labelH));
    modeBox.setBounds(col.removeFromTop(24).reduced(4, 0));

    col = row2.removeFromLeft(row2W);
    oversampleLabel.setBounds(col.removeFromTop(labelH));
    oversampleBox.setBounds(col.removeFromTop(24).reduced(4, 0));

    col = row2;
    dcBlockBtn.setBounds(col.reduced(4, 12));
}

void HGSaturatorEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
{
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
    addAndMakeVisible(knob);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    label.setFont(juce::Font(11.0f));
    addAndMakeVisible(label);
}

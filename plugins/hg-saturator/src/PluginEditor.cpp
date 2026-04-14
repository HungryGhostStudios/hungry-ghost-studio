#include "PluginEditor.h"

HGSaturatorEditor::HGSaturatorEditor(HGSaturatorProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p),
      oscilloscope(p)
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

    // Mode selector (5 modes)
    modeBox.addItemList({"Soft Clip", "Hard Clip", "Tape", "Wavefold", "Asymmetric"}, 1);
    addAndMakeVisible(modeBox);
    modeAtt = std::make_unique<ComboBoxAttachment>(apvts, "mode", modeBox);
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    addAndMakeVisible(modeLabel);

    // Oversample selector (1x/2x/4x/8x)
    oversampleBox.addItemList({"1x", "2x", "4x", "8x"}, 1);
    addAndMakeVisible(oversampleBox);
    oversampleAtt = std::make_unique<ComboBoxAttachment>(apvts, "oversample", oversampleBox);
    oversampleLabel.setText("Oversample", juce::dontSendNotification);
    oversampleLabel.setJustificationType(juce::Justification::centred);
    oversampleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    addAndMakeVisible(oversampleLabel);

    // DC block toggle
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
    g.setColour(juce::Colour(HGLookAndFeel::saturatorAccent));
    g.setFont(14.0f);
    g.drawText("INPUT", 10, 5, 80, 20, juce::Justification::centred);
    g.drawText("SHAPING", 180, 5, 120, 20, juce::Justification::centred);
    g.drawText("OUTPUT", 540, 5, 100, 20, juce::Justification::centred);
}

void HGSaturatorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20); // section labels

    const int knobSize = 60;
    const int labelH = 16;

    // Left column: Input gain + pre-filter
    auto leftCol = area.removeFromLeft(80);
    inputGainLabel.setBounds(leftCol.removeFromTop(labelH));
    inputGainKnob.setBounds(leftCol.removeFromTop(knobSize));
    leftCol.removeFromTop(10);
    preFilterLabel.setBounds(leftCol.removeFromTop(labelH));
    preFilterKnob.setBounds(leftCol.removeFromTop(knobSize));

    // Right column: Output gain + mix + post-filter
    auto rightCol = area.removeFromRight(80);
    outputGainLabel.setBounds(rightCol.removeFromTop(labelH));
    outputGainKnob.setBounds(rightCol.removeFromTop(knobSize));
    rightCol.removeFromTop(10);
    mixLabel.setBounds(rightCol.removeFromTop(labelH));
    mixKnob.setBounds(rightCol.removeFromTop(knobSize));
    rightCol.removeFromTop(10);
    postFilterLabel.setBounds(rightCol.removeFromTop(labelH));
    postFilterKnob.setBounds(rightCol.removeFromTop(knobSize));

    area.removeFromLeft(10);
    area.removeFromRight(10);

    // Center area: oscilloscope on top, controls below
    auto scopeArea = area.removeFromTop(160);
    oscilloscope.setBounds(scopeArea);

    area.removeFromTop(10);

    // Drive and Bias knobs row
    auto knobRow = area.removeFromTop(knobSize + labelH);
    int knobW = knobRow.getWidth() / 2;

    auto driveCol = knobRow.removeFromLeft(knobW);
    driveLabel.setBounds(driveCol.removeFromTop(labelH));
    driveKnob.setBounds(driveCol.removeFromTop(knobSize));

    auto biasCol = knobRow;
    biasLabel.setBounds(biasCol.removeFromTop(labelH));
    biasKnob.setBounds(biasCol.removeFromTop(knobSize));

    area.removeFromTop(5);

    // Bottom row: mode selector, oversample selector, DC block toggle
    auto bottomRow = area.removeFromTop(50);
    int bottomW = bottomRow.getWidth() / 3;

    auto modeCol = bottomRow.removeFromLeft(bottomW);
    modeLabel.setBounds(modeCol.removeFromTop(labelH));
    modeBox.setBounds(modeCol.removeFromTop(24).reduced(2, 0));

    auto osCol = bottomRow.removeFromLeft(bottomW);
    oversampleLabel.setBounds(osCol.removeFromTop(labelH));
    oversampleBox.setBounds(osCol.removeFromTop(24).reduced(2, 0));

    auto dcCol = bottomRow;
    dcBlockBtn.setBounds(dcCol.removeFromTop(40).reduced(4, 8));
}

void HGSaturatorEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
{
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    addAndMakeVisible(knob);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    label.setFont(juce::Font(12.0f));
    addAndMakeVisible(label);
}

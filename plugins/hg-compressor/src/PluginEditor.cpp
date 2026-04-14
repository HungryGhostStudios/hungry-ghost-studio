#include "PluginEditor.h"

HGCompressorEditor::HGCompressorEditor(HGCompressorProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p),
      grMeter(p),
      transferCurve(p.getAPVTS())
{
    setLookAndFeel(&lookAndFeel);
    setSize(600, 400);

    // Setup all knobs
    setupKnob(inputGainKnob, inputGainLabel, "Input");
    setupKnob(thresholdKnob, thresholdLabel, "Threshold");
    setupKnob(ratioKnob, ratioLabel, "Ratio");
    setupKnob(attackKnob, attackLabel, "Attack");
    setupKnob(releaseKnob, releaseLabel, "Release");
    setupKnob(kneeKnob, kneeLabel, "Knee");
    setupKnob(makeupKnob, makeupLabel, "Makeup");
    setupKnob(mixKnob, mixLabel, "Mix");
    setupKnob(lookaheadKnob, lookaheadLabel, "Lookahead");

    // APVTS attachments
    auto& apvts = p.getAPVTS();
    inputGainAtt = std::make_unique<SliderAttachment>(apvts, "input_gain", inputGainKnob);
    thresholdAtt = std::make_unique<SliderAttachment>(apvts, "threshold", thresholdKnob);
    ratioAtt     = std::make_unique<SliderAttachment>(apvts, "ratio", ratioKnob);
    attackAtt    = std::make_unique<SliderAttachment>(apvts, "attack", attackKnob);
    releaseAtt   = std::make_unique<SliderAttachment>(apvts, "release", releaseKnob);
    kneeAtt      = std::make_unique<SliderAttachment>(apvts, "knee", kneeKnob);
    makeupAtt    = std::make_unique<SliderAttachment>(apvts, "makeup", makeupKnob);
    mixAtt       = std::make_unique<SliderAttachment>(apvts, "mix", mixKnob);
    lookaheadAtt = std::make_unique<SliderAttachment>(apvts, "lookahead", lookaheadKnob);

    // Mode selector
    modeBox.addItemList({"Feed-Forward", "Feed-Back", "Opto"}, 1);
    addAndMakeVisible(modeBox);
    modeAtt = std::make_unique<ComboBoxAttachment>(apvts, "mode", modeBox);
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    addAndMakeVisible(modeLabel);

    // Detector selector
    detectorBox.addItemList({"Peak", "RMS"}, 1);
    addAndMakeVisible(detectorBox);
    detectorAtt = std::make_unique<ComboBoxAttachment>(apvts, "detector", detectorBox);
    detectorLabel.setText("Detector", juce::dontSendNotification);
    detectorLabel.setJustificationType(juce::Justification::centred);
    detectorLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    addAndMakeVisible(detectorLabel);

    // M/S mode selector
    msModeBox.addItemList({"Stereo", "Mid", "Side"}, 1);
    addAndMakeVisible(msModeBox);
    msModeAtt = std::make_unique<ComboBoxAttachment>(apvts, "ms_mode", msModeBox);
    msModeLabel.setText("M/S", juce::dontSendNotification);
    msModeLabel.setJustificationType(juce::Justification::centred);
    msModeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    addAndMakeVisible(msModeLabel);

    // Stereo link toggle
    addAndMakeVisible(stereoLinkBtn);
    stereoLinkAtt = std::make_unique<ButtonAttachment>(apvts, "stereo_link", stereoLinkBtn);

    // GR meter and transfer curve
    addAndMakeVisible(grMeter);
    addAndMakeVisible(transferCurve);
}

void HGCompressorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));

    // Section labels
    g.setColour(juce::Colour(0xFFFFB300));
    g.setFont(14.0f);
    g.drawText("INPUT", 10, 5, 80, 20, juce::Justification::centred);
    g.drawText("COMPRESSION", 130, 5, 200, 20, juce::Justification::centred);
    g.drawText("OUTPUT", 480, 5, 110, 20, juce::Justification::centred);
}

void HGCompressorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20); // section labels

    // Layout: [Input col] [Compression section] [GR meter] [Output col]
    const int knobSize = 60;
    const int labelH = 16;

    // Input column
    auto inputCol = area.removeFromLeft(80);
    inputGainLabel.setBounds(inputCol.removeFromTop(labelH));
    inputGainKnob.setBounds(inputCol.removeFromTop(knobSize));

    // Output column (right side)
    auto outputCol = area.removeFromRight(80);
    makeupLabel.setBounds(outputCol.removeFromTop(labelH));
    makeupKnob.setBounds(outputCol.removeFromTop(knobSize));
    mixLabel.setBounds(outputCol.removeFromTop(labelH));
    mixKnob.setBounds(outputCol.removeFromTop(knobSize));

    // GR meter (next to output)
    auto meterArea = area.removeFromRight(30);
    grMeter.setBounds(meterArea.reduced(2));

    area.removeFromLeft(10);
    area.removeFromRight(10);

    // Transfer curve (top of compression area)
    auto curveArea = area.removeFromTop(130);
    transferCurve.setBounds(curveArea);

    area.removeFromTop(5);

    // Compression knobs row
    auto knobRow = area.removeFromTop(knobSize + labelH);
    int compKnobW = knobRow.getWidth() / 5;

    auto threshCol = knobRow.removeFromLeft(compKnobW);
    thresholdLabel.setBounds(threshCol.removeFromTop(labelH));
    thresholdKnob.setBounds(threshCol.removeFromTop(knobSize));

    auto ratioCol = knobRow.removeFromLeft(compKnobW);
    ratioLabel.setBounds(ratioCol.removeFromTop(labelH));
    ratioKnob.setBounds(ratioCol.removeFromTop(knobSize));

    auto attackCol = knobRow.removeFromLeft(compKnobW);
    attackLabel.setBounds(attackCol.removeFromTop(labelH));
    attackKnob.setBounds(attackCol.removeFromTop(knobSize));

    auto releaseCol = knobRow.removeFromLeft(compKnobW);
    releaseLabel.setBounds(releaseCol.removeFromTop(labelH));
    releaseKnob.setBounds(releaseCol.removeFromTop(knobSize));

    auto kneeCol = knobRow;
    kneeLabel.setBounds(kneeCol.removeFromTop(labelH));
    kneeKnob.setBounds(kneeCol.removeFromTop(knobSize));

    area.removeFromTop(5);

    // Bottom row: mode/detector selectors + lookahead
    auto bottomRow = area.removeFromTop(50);
    int bottomW = bottomRow.getWidth() / 5;

    auto modeCol = bottomRow.removeFromLeft(bottomW);
    modeLabel.setBounds(modeCol.removeFromTop(labelH));
    modeBox.setBounds(modeCol.removeFromTop(24).reduced(2, 0));

    auto detCol = bottomRow.removeFromLeft(bottomW);
    detectorLabel.setBounds(detCol.removeFromTop(labelH));
    detectorBox.setBounds(detCol.removeFromTop(24).reduced(2, 0));

    auto msCol = bottomRow.removeFromLeft(bottomW);
    msModeLabel.setBounds(msCol.removeFromTop(labelH));
    msModeBox.setBounds(msCol.removeFromTop(24).reduced(2, 0));

    auto linkCol = bottomRow.removeFromLeft(bottomW);
    stereoLinkBtn.setBounds(linkCol.removeFromTop(40).reduced(4, 8));

    auto laCol = bottomRow;
    lookaheadLabel.setBounds(laCol.removeFromTop(labelH));
    lookaheadKnob.setBounds(laCol.removeFromTop(knobSize).reduced(2));
}

void HGCompressorEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
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

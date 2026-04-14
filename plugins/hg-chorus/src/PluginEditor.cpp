#include "PluginEditor.h"

HGChorusEditor::HGChorusEditor(HGChorusProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p),
      phaseWheel(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(580, 360);

    // Setup knobs
    setupKnob(voicesKnob, voicesLabel, "Voices");
    setupKnob(rateKnob, rateLabel, "Rate");
    setupKnob(depthKnob, depthLabel, "Depth");
    setupKnob(delayKnob, delayLabel, "Delay");
    setupKnob(feedbackKnob, feedbackLabel, "Feedback");
    setupKnob(spreadKnob, spreadLabel, "Spread");
    setupKnob(mixKnob, mixLabel, "Mix");
    setupKnob(bbdStagesKnob, bbdStagesLabel, "Stages");
    setupKnob(bbdNoiseKnob, bbdNoiseLabel, "Noise");

    // APVTS attachments
    auto& apvts = p.getAPVTS();
    voicesAtt    = std::make_unique<SliderAttachment>(apvts, "voices", voicesKnob);
    rateAtt      = std::make_unique<SliderAttachment>(apvts, "rate", rateKnob);
    depthAtt     = std::make_unique<SliderAttachment>(apvts, "depth", depthKnob);
    delayAtt     = std::make_unique<SliderAttachment>(apvts, "delay", delayKnob);
    feedbackAtt  = std::make_unique<SliderAttachment>(apvts, "feedback", feedbackKnob);
    spreadAtt    = std::make_unique<SliderAttachment>(apvts, "spread", spreadKnob);
    mixAtt       = std::make_unique<SliderAttachment>(apvts, "mix", mixKnob);
    bbdStagesAtt = std::make_unique<SliderAttachment>(apvts, "bbd_stages", bbdStagesKnob);
    bbdNoiseAtt  = std::make_unique<SliderAttachment>(apvts, "bbd_noise", bbdNoiseKnob);

    // Mode ComboBox
    modeBox.addItem("Clean", 1);
    modeBox.addItem("BBD", 2);
    modeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A2A));
    modeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFE0E0E0));
    modeBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF4CAF50));
    addAndMakeVisible(modeBox);
    modeAtt = std::make_unique<ComboBoxAttachment>(apvts, "mode", modeBox);

    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    modeLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(modeLabel);

    // LFO Phase Wheel
    addAndMakeVisible(phaseWheel);

    // Listen for mode changes to show/hide BBD controls
    apvts.addParameterListener("mode", this);
    updateBBDVisibility();
}

HGChorusEditor::~HGChorusEditor()
{
    processorRef.getAPVTS().removeParameterListener("mode", this);
    setLookAndFeel(nullptr);
}

void HGChorusEditor::parameterChanged(const juce::String& /*parameterID*/, float /*newValue*/)
{
    juce::MessageManager::callAsync([this] { updateBBDVisibility(); });
}

void HGChorusEditor::updateBBDVisibility()
{
    bool isBBD = static_cast<int>(*processorRef.getAPVTS().getRawParameterValue("mode")) == 1;
    bbdStagesKnob.setVisible(isBBD);
    bbdStagesLabel.setVisible(isBBD);
    bbdNoiseKnob.setVisible(isBBD);
    bbdNoiseLabel.setVisible(isBBD);
}

void HGChorusEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));

    // Section labels
    g.setColour(juce::Colour(0xFF4CAF50));
    g.setFont(14.0f);
    g.drawText("CHORUS", 10, 5, 100, 20, juce::Justification::centred);
    g.drawText("LFO PHASE", 420, 5, 150, 20, juce::Justification::centred);
}

void HGChorusEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20); // section labels

    const int knobSize = 56;
    const int labelH = 16;

    // Split: left side for knobs, right side for phase wheel
    auto rightSide = area.removeFromRight(160);
    phaseWheel.setBounds(rightSide.reduced(10));

    // Row 1: Voices, Rate, Depth, Delay, Feedback
    auto row1 = area.removeFromTop(knobSize + labelH);
    int knobW = area.getWidth() / 5;

    auto col = row1.removeFromLeft(knobW);
    voicesLabel.setBounds(col.removeFromTop(labelH));
    voicesKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    rateLabel.setBounds(col.removeFromTop(labelH));
    rateKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    depthLabel.setBounds(col.removeFromTop(labelH));
    depthKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    delayLabel.setBounds(col.removeFromTop(labelH));
    delayKnob.setBounds(col.removeFromTop(knobSize));

    col = row1;
    feedbackLabel.setBounds(col.removeFromTop(labelH));
    feedbackKnob.setBounds(col.removeFromTop(knobSize));

    area.removeFromTop(12);

    // Row 2: Spread, Mix, Mode
    auto row2 = area.removeFromTop(knobSize + labelH);
    int row2W = area.getWidth() / 5;

    col = row2.removeFromLeft(row2W);
    spreadLabel.setBounds(col.removeFromTop(labelH));
    spreadKnob.setBounds(col.removeFromTop(knobSize));

    col = row2.removeFromLeft(row2W);
    mixLabel.setBounds(col.removeFromTop(labelH));
    mixKnob.setBounds(col.removeFromTop(knobSize));

    col = row2.removeFromLeft(row2W);
    modeLabel.setBounds(col.removeFromTop(labelH));
    modeBox.setBounds(col.removeFromTop(24).reduced(4, 0));

    area.removeFromTop(12);

    // Row 3: BBD-specific (Stages, Noise) — only visible in BBD mode
    auto row3 = area.removeFromTop(knobSize + labelH);
    int row3W = area.getWidth() / 5;

    col = row3.removeFromLeft(row3W);
    bbdStagesLabel.setBounds(col.removeFromTop(labelH));
    bbdStagesKnob.setBounds(col.removeFromTop(knobSize));

    col = row3.removeFromLeft(row3W);
    bbdNoiseLabel.setBounds(col.removeFromTop(labelH));
    bbdNoiseKnob.setBounds(col.removeFromTop(knobSize));
}

void HGChorusEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
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

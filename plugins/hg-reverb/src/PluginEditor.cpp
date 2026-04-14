#include "PluginEditor.h"

HGReverbEditor::HGReverbEditor(HGReverbProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p),
      irVis(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(700, 420);

    // Setup all knobs
    setupKnob(predelayKnob, predelayLabel, "Predelay");
    setupKnob(sizeKnob, sizeLabel, "Size");
    setupKnob(decayKnob, decayLabel, "Decay");
    setupKnob(dampingKnob, dampingLabel, "Damping");
    setupKnob(diffusionKnob, diffusionLabel, "Diffusion");
    setupKnob(modRateKnob, modRateLabel, "Mod Rate");
    setupKnob(modDepthKnob, modDepthLabel, "Mod Depth");
    setupKnob(earlyLevelKnob, earlyLevelLabel, "Early");
    setupKnob(lateLevelKnob, lateLevelLabel, "Late");
    setupKnob(widthKnob, widthLabel, "Width");
    setupKnob(mixKnob, mixLabel, "Mix");

    // APVTS attachments
    auto& apvts = p.apvts;
    predelayAtt    = std::make_unique<SliderAttachment>(apvts, "predelay", predelayKnob);
    sizeAtt        = std::make_unique<SliderAttachment>(apvts, "size", sizeKnob);
    decayAtt       = std::make_unique<SliderAttachment>(apvts, "decay", decayKnob);
    dampingAtt     = std::make_unique<SliderAttachment>(apvts, "damping", dampingKnob);
    diffusionAtt   = std::make_unique<SliderAttachment>(apvts, "diffusion", diffusionKnob);
    modRateAtt     = std::make_unique<SliderAttachment>(apvts, "mod_rate", modRateKnob);
    modDepthAtt    = std::make_unique<SliderAttachment>(apvts, "mod_depth", modDepthKnob);
    earlyLevelAtt  = std::make_unique<SliderAttachment>(apvts, "early_level", earlyLevelKnob);
    lateLevelAtt   = std::make_unique<SliderAttachment>(apvts, "late_level", lateLevelKnob);
    widthAtt       = std::make_unique<SliderAttachment>(apvts, "width", widthKnob);
    mixAtt         = std::make_unique<SliderAttachment>(apvts, "mix", mixKnob);

    // Freeze toggle
    freezeBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFE0E0E0));
    freezeBtn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFF26A69A));
    addAndMakeVisible(freezeBtn);
    freezeAtt = std::make_unique<ButtonAttachment>(apvts, "freeze", freezeBtn);

    // IR Visualization
    addAndMakeVisible(irVis);
}

HGReverbEditor::~HGReverbEditor()
{
    setLookAndFeel(nullptr);
}

void HGReverbEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));

    // Section labels
    g.setColour(juce::Colour(0xFF26A69A));
    g.setFont(14.0f);
    g.drawText("EARLY", 10, 5, 100, 20, juce::Justification::centred);
    g.drawText("LATE", 230, 5, 100, 20, juce::Justification::centred);
    g.drawText("MODULATION", 430, 5, 120, 20, juce::Justification::centred);
    g.drawText("OUTPUT", 580, 5, 110, 20, juce::Justification::centred);
}

void HGReverbEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20); // section labels

    const int knobSize = 56;
    const int labelH = 16;

    // IR Visualization at top
    auto irArea = area.removeFromTop(120);
    irVis.setBounds(irArea);

    area.removeFromTop(8);

    // Knob rows
    // Row 1: Predelay, Size, Decay, Damping, Diffusion, Width, Mix
    auto row1 = area.removeFromTop(knobSize + labelH);
    int knobW = row1.getWidth() / 7;

    auto col = row1.removeFromLeft(knobW);
    predelayLabel.setBounds(col.removeFromTop(labelH));
    predelayKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    sizeLabel.setBounds(col.removeFromTop(labelH));
    sizeKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    decayLabel.setBounds(col.removeFromTop(labelH));
    decayKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    dampingLabel.setBounds(col.removeFromTop(labelH));
    dampingKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    diffusionLabel.setBounds(col.removeFromTop(labelH));
    diffusionKnob.setBounds(col.removeFromTop(knobSize));

    col = row1.removeFromLeft(knobW);
    widthLabel.setBounds(col.removeFromTop(labelH));
    widthKnob.setBounds(col.removeFromTop(knobSize));

    col = row1;
    mixLabel.setBounds(col.removeFromTop(labelH));
    mixKnob.setBounds(col.removeFromTop(knobSize));

    area.removeFromTop(8);

    // Row 2: Early Level, Late Level, Mod Rate, Mod Depth, Freeze
    auto row2 = area.removeFromTop(knobSize + labelH);
    int row2W = row2.getWidth() / 5;

    col = row2.removeFromLeft(row2W);
    earlyLevelLabel.setBounds(col.removeFromTop(labelH));
    earlyLevelKnob.setBounds(col.removeFromTop(knobSize));

    col = row2.removeFromLeft(row2W);
    lateLevelLabel.setBounds(col.removeFromTop(labelH));
    lateLevelKnob.setBounds(col.removeFromTop(knobSize));

    col = row2.removeFromLeft(row2W);
    modRateLabel.setBounds(col.removeFromTop(labelH));
    modRateKnob.setBounds(col.removeFromTop(knobSize));

    col = row2.removeFromLeft(row2W);
    modDepthLabel.setBounds(col.removeFromTop(labelH));
    modDepthKnob.setBounds(col.removeFromTop(knobSize));

    // Freeze button — prominent, centered in remaining space
    col = row2;
    auto freezeArea = col.reduced(8, 10);
    freezeBtn.setBounds(freezeArea);
}

void HGReverbEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
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

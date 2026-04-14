#include "PluginEditor.h"
#include <cmath>

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
    predelayAtt   = std::make_unique<SliderAttachment>(apvts, "predelay", predelayKnob);
    sizeAtt       = std::make_unique<SliderAttachment>(apvts, "size", sizeKnob);
    decayAtt      = std::make_unique<SliderAttachment>(apvts, "decay", decayKnob);
    dampingAtt    = std::make_unique<SliderAttachment>(apvts, "damping", dampingKnob);
    diffusionAtt  = std::make_unique<SliderAttachment>(apvts, "diffusion", diffusionKnob);
    modRateAtt    = std::make_unique<SliderAttachment>(apvts, "mod_rate", modRateKnob);
    modDepthAtt   = std::make_unique<SliderAttachment>(apvts, "mod_depth", modDepthKnob);
    earlyLevelAtt = std::make_unique<SliderAttachment>(apvts, "early_level", earlyLevelKnob);
    lateLevelAtt  = std::make_unique<SliderAttachment>(apvts, "late_level", lateLevelKnob);
    widthAtt      = std::make_unique<SliderAttachment>(apvts, "width", widthKnob);
    mixAtt        = std::make_unique<SliderAttachment>(apvts, "mix", mixKnob);

    // Freeze toggle button
    freezeBtn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFF26A69A));
    addAndMakeVisible(freezeBtn);
    freezeAtt = std::make_unique<ButtonAttachment>(apvts, "freeze", freezeBtn);

    // IR visualization
    addAndMakeVisible(irVis);
}

void HGReverbEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));

    // Section labels
    g.setColour(juce::Colour(0xFF26A69A));
    g.setFont(14.0f);
    g.drawText("EARLY", 10, 5, 80, 20, juce::Justification::centred);
    g.drawText("REVERB", 200, 5, 120, 20, juce::Justification::centred);
    g.drawText("MODULATION", 400, 5, 120, 20, juce::Justification::centred);
    g.drawText("OUTPUT", 580, 5, 110, 20, juce::Justification::centred);
}

void HGReverbEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20); // section labels

    const int knobSize = 60;
    const int labelH = 16;

    // IR visualization at top
    auto irArea = area.removeFromTop(130);
    irVis.setBounds(irArea);

    area.removeFromTop(8);

    // Row 1: Main reverb controls (predelay, size, decay, damping, diffusion)
    auto row1 = area.removeFromTop(knobSize + labelH);
    int knobW = row1.getWidth() / 6;

    auto predelayCol = row1.removeFromLeft(knobW);
    predelayLabel.setBounds(predelayCol.removeFromTop(labelH));
    predelayKnob.setBounds(predelayCol.removeFromTop(knobSize));

    auto sizeCol = row1.removeFromLeft(knobW);
    sizeLabel.setBounds(sizeCol.removeFromTop(labelH));
    sizeKnob.setBounds(sizeCol.removeFromTop(knobSize));

    auto decayCol = row1.removeFromLeft(knobW);
    decayLabel.setBounds(decayCol.removeFromTop(labelH));
    decayKnob.setBounds(decayCol.removeFromTop(knobSize));

    auto dampingCol = row1.removeFromLeft(knobW);
    dampingLabel.setBounds(dampingCol.removeFromTop(labelH));
    dampingKnob.setBounds(dampingCol.removeFromTop(knobSize));

    auto diffusionCol = row1.removeFromLeft(knobW);
    diffusionLabel.setBounds(diffusionCol.removeFromTop(labelH));
    diffusionKnob.setBounds(diffusionCol.removeFromTop(knobSize));

    // Freeze button in remaining space of row 1
    auto freezeCol = row1;
    freezeBtn.setBounds(freezeCol.reduced(4, 10));

    area.removeFromTop(8);

    // Row 2: Modulation, levels, width, mix
    auto row2 = area.removeFromTop(knobSize + labelH);
    knobW = row2.getWidth() / 6;

    auto modRateCol = row2.removeFromLeft(knobW);
    modRateLabel.setBounds(modRateCol.removeFromTop(labelH));
    modRateKnob.setBounds(modRateCol.removeFromTop(knobSize));

    auto modDepthCol = row2.removeFromLeft(knobW);
    modDepthLabel.setBounds(modDepthCol.removeFromTop(labelH));
    modDepthKnob.setBounds(modDepthCol.removeFromTop(knobSize));

    auto earlyCol = row2.removeFromLeft(knobW);
    earlyLevelLabel.setBounds(earlyCol.removeFromTop(labelH));
    earlyLevelKnob.setBounds(earlyCol.removeFromTop(knobSize));

    auto lateCol = row2.removeFromLeft(knobW);
    lateLevelLabel.setBounds(lateCol.removeFromTop(labelH));
    lateLevelKnob.setBounds(lateCol.removeFromTop(knobSize));

    auto widthCol = row2.removeFromLeft(knobW);
    widthLabel.setBounds(widthCol.removeFromTop(labelH));
    widthKnob.setBounds(widthCol.removeFromTop(knobSize));

    auto mixCol = row2;
    mixLabel.setBounds(mixCol.removeFromTop(labelH));
    mixKnob.setBounds(mixCol.removeFromTop(knobSize));
}

void HGReverbEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
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

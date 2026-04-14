#include "HGLookAndFeel.h"

HGLookAndFeel::HGLookAndFeel(juce::Colour accent)
    : accentColour(accent)
{
    // Dark theme base colours
    auto bg   = juce::Colour(backgroundColourHex);
    auto text = juce::Colour(textColourHex);

    setColour(juce::ResizableWindow::backgroundColourId, bg);
    setColour(juce::Label::textColourId, text);
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, bg);
    setColour(juce::Slider::rotarySliderFillColourId, accentColour);
    setColour(juce::Slider::thumbColourId, accentColour);
    setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
    setColour(juce::TextButton::textColourOnId, text);
    setColour(juce::TextButton::textColourOffId, text);

    // Force rotary knobs to vertical-drag style
    setDefaultSansSerifTypefaceName("Arial");
}

void HGLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                      juce::Slider& /*slider*/)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto diameter = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Flat circular knob background
    g.setColour(juce::Colour(backgroundColourHex).brighter(0.15f));
    g.fillEllipse(rx, ry, diameter, diameter);

    // Accent arc showing value
    juce::Path arcPath;
    arcPath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                          0.0f, rotaryStartAngle, angle, true);
    g.setColour(accentColour);
    g.strokePath(arcPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // Notch indicator
    juce::Path notch;
    auto notchLength = radius * 0.33f;
    notch.addRectangle(-1.5f, -radius + 4.0f, 3.0f, notchLength);
    notch.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(juce::Colour(textColourHex));
    g.fillPath(notch);
}

void HGLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                      juce::Slider::SliderStyle style, juce::Slider& /*slider*/)
{
    auto isVertical = (style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical);
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

    // Track background
    g.setColour(juce::Colour(backgroundColourHex).brighter(0.1f));
    if (isVertical)
    {
        auto trackWidth = juce::jmin(6.0f, bounds.getWidth() * 0.25f);
        auto trackX = bounds.getCentreX() - trackWidth * 0.5f;
        g.fillRoundedRectangle(trackX, bounds.getY(), trackWidth, bounds.getHeight(), 2.0f);

        // Accent fill from bottom to slider position
        g.setColour(accentColour);
        auto fillHeight = bounds.getBottom() - sliderPos;
        g.fillRoundedRectangle(trackX, sliderPos, trackWidth, fillHeight, 2.0f);
    }
    else
    {
        auto trackHeight = juce::jmin(6.0f, bounds.getHeight() * 0.25f);
        auto trackY = bounds.getCentreY() - trackHeight * 0.5f;
        g.fillRoundedRectangle(bounds.getX(), trackY, bounds.getWidth(), trackHeight, 2.0f);

        // Accent fill from left to slider position
        g.setColour(accentColour);
        auto fillWidth = sliderPos - bounds.getX();
        g.fillRoundedRectangle(bounds.getX(), trackY, fillWidth, trackHeight, 2.0f);
    }

    // Flat rectangular fader thumb
    g.setColour(juce::Colour(textColourHex));
    if (isVertical)
    {
        auto thumbWidth = juce::jmin(20.0f, bounds.getWidth() * 0.6f);
        g.fillRoundedRectangle(bounds.getCentreX() - thumbWidth * 0.5f, sliderPos - 4.0f,
                               thumbWidth, 8.0f, 2.0f);
    }
    else
    {
        auto thumbHeight = juce::jmin(20.0f, bounds.getHeight() * 0.6f);
        g.fillRoundedRectangle(sliderPos - 4.0f, bounds.getCentreY() - thumbHeight * 0.5f,
                               8.0f, thumbHeight, 2.0f);
    }
}

void HGLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& /*backgroundColour*/,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto isOn = button.getToggleState();

    // Flat toggle: accent when on, dark when off
    if (isOn)
        g.setColour(accentColour);
    else
        g.setColour(juce::Colour(backgroundColourHex).brighter(0.15f));

    g.fillRoundedRectangle(bounds, 4.0f);

    // Highlight on hover
    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRoundedRectangle(bounds, 4.0f);
    }
}

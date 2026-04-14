#include "HGLookAndFeel.h"

HGLookAndFeel::HGLookAndFeel(juce::Colour accent)
    : accentColour(accent)
{
    // Dark theme base colours
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(backgroundColourHex));
    setColour(juce::Label::textColourId, juce::Colour(textColourHex));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(textColourHex));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::textColourOnId, juce::Colour(textColourHex));
    setColour(juce::TextButton::textColourOffId, juce::Colour(textColourHex));

    // All rotary sliders use vertical drag
    setDefaultLookAndFeel(nullptr); // don't override global
}

void HGLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float rotaryStartAngle,
                                      float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused(slider);

    const float radius = static_cast<float>(juce::jmin(width, height)) * 0.5f - 4.0f;
    const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Flat circular knob background
    g.setColour(juce::Colour(0xFF2A2A2A));
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Accent arc showing value
    juce::Path arcPath;
    arcPath.addCentredArc(centreX, centreY, radius, radius,
                          0.0f, rotaryStartAngle, angle, true);
    g.setColour(accentColour);
    g.strokePath(arcPath, juce::PathStrokeType(3.0f));

    // Notch indicator
    const float notchLength = radius * 0.33f;
    const float notchX = centreX + (radius - notchLength) * std::cos(angle - juce::MathConstants<float>::halfPi);
    const float notchY = centreY + (radius - notchLength) * std::sin(angle - juce::MathConstants<float>::halfPi);
    const float notchEndX = centreX + radius * std::cos(angle - juce::MathConstants<float>::halfPi);
    const float notchEndY = centreY + radius * std::sin(angle - juce::MathConstants<float>::halfPi);

    g.setColour(accentColour);
    g.drawLine(notchX, notchY, notchEndX, notchEndY, 2.5f);
}

void HGLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float minSliderPos, float maxSliderPos,
                                      juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos, style, slider);

    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                static_cast<float>(width), static_cast<float>(height));

    // Flat rectangular fader track
    g.setColour(juce::Colour(0xFF2A2A2A));
    g.fillRect(bounds);

    // Accent fill up to current position
    g.setColour(accentColour);
    if (slider.isHorizontal())
    {
        g.fillRect(bounds.withWidth(sliderPos - static_cast<float>(x)));
    }
    else
    {
        const float fillHeight = bounds.getBottom() - sliderPos;
        g.fillRect(bounds.withTop(sliderPos).withHeight(fillHeight));
    }
}

void HGLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour, shouldDrawButtonAsHighlighted);

    const auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool isOn = button.getToggleState() || shouldDrawButtonAsDown;

    // Flat toggle: accent when on, dark when off
    g.setColour(isOn ? accentColour : juce::Colour(0xFF2A2A2A));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Subtle border
    g.setColour(accentColour.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HGLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit HGLookAndFeel(juce::Colour accent);

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    // Per-plugin accent colours
    static constexpr juce::uint32 compressorAccent = 0xFFFFC107; // amber
    static constexpr juce::uint32 eqAccent         = 0xFF2196F3; // blue
    static constexpr juce::uint32 saturatorAccent   = 0xFFFF9800; // orange
    static constexpr juce::uint32 reverbAccent      = 0xFF009688; // teal
    static constexpr juce::uint32 chorusAccent      = 0xFF4CAF50; // green
    static constexpr juce::uint32 synthAccent       = 0xFF9C27B0; // purple

private:
    juce::Colour accentColour;

    static constexpr juce::uint32 backgroundColourHex = 0xFF1E1E1E;
    static constexpr juce::uint32 textColourHex       = 0xFFE0E0E0;
};

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "HGLookAndFeel.h"

#include <array>

// Forward declarations
class FrequencyResponseDisplay;
class BandControlStrip;

//==============================================================================
class SpectrumAnalyzer : public juce::Component, private juce::Timer
{
public:
    SpectrumAnalyzer();

    void pushSamples(const float* data, int numSamples);
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder; // 2048
    juce::dsp::FFT fft{fftOrder};
    juce::dsp::WindowingFunction<float> window{static_cast<size_t>(fftSize),
                                                juce::dsp::WindowingFunction<float>::hann};
    std::array<float, fftSize * 2> fftData = {};
    std::array<float, fftSize / 2> smoothedMagnitudes = {};
    int fifoIndex = 0;
    std::array<float, fftSize> fifo = {};
    bool nextBlockReady = false;
};

//==============================================================================
class FrequencyResponseDisplay : public juce::Component, private juce::Timer
{
public:
    FrequencyResponseDisplay(HGEqProcessor& proc);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    int getSelectedBand() const { return selectedBand; }
    SpectrumAnalyzer& getAnalyzer() { return analyzer; }

private:
    void timerCallback() override { repaint(); }

    float freqToX(float freq, float width) const;
    float xToFreq(float x, float width) const;
    float dbToY(float db, float height) const;
    float yToDb(float y, float height) const;

    HGEqProcessor& processor;
    SpectrumAnalyzer analyzer;
    int selectedBand = -1;

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb = -24.0f;
    static constexpr float maxDb = 24.0f;
};

//==============================================================================
class HGEqEditor : public juce::AudioProcessorEditor
{
public:
    explicit HGEqEditor(HGEqProcessor& p);
    ~HGEqEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    HGEqProcessor& processorRef;
    HGLookAndFeel lookAndFeel{juce::Colour(0xFF42A5F5)};

    FrequencyResponseDisplay responseDisplay;

    // Per-band controls
    struct BandUI {
        juce::ComboBox typeBox;
        juce::Slider freqKnob, gainKnob, qKnob;
        juce::ToggleButton enableBtn{"On"};
        juce::Label freqLabel, gainLabel, qLabel;
        std::unique_ptr<ComboBoxAttachment> typeAtt;
        std::unique_ptr<SliderAttachment> freqAtt, gainAtt, qAtt;
        std::unique_ptr<ButtonAttachment> enableAtt;
    };
    std::array<BandUI, HGEqProcessor::numBands> bands;

    // Global controls
    juce::ComboBox msModeBox;
    juce::ToggleButton linearPhaseBtn{"Linear Phase"};
    juce::Slider outputGainKnob;
    juce::Label outputGainLabel, msModeLabel;
    std::unique_ptr<ComboBoxAttachment> msModeAtt;
    std::unique_ptr<ButtonAttachment> linearPhaseAtt;
    std::unique_ptr<SliderAttachment> outputGainAtt;

    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGEqEditor)
};

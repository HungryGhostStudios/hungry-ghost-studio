#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <libdsp/filters/BiquadCascade.h>
#include <libdsp/filters/Coefficients.h>
#include <libdsp/util/MidSide.h>

class HGEqProcessor : public juce::AudioProcessor
{
public:
    HGEqProcessor();
    ~HGEqProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "HG EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int numBands = 8;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void updateCoefficients(int band, float sampleRate);

    // Per-channel EQ: 8 bands, 2 channels
    std::array<libdsp::BiquadCascade, numBands> eqBandsL;
    std::array<libdsp::BiquadCascade, numBands> eqBandsR;

    // Cached atomic parameter pointers
    struct BandParams {
        std::atomic<float>* type    = nullptr;
        std::atomic<float>* freq    = nullptr;
        std::atomic<float>* gain    = nullptr;
        std::atomic<float>* q       = nullptr;
        std::atomic<float>* enabled = nullptr;
    };

    std::array<BandParams, numBands> bandParams;
    std::atomic<float>* msModeParam    = nullptr;
    std::atomic<float>* linearPhaseParam = nullptr;
    std::atomic<float>* outputGainParam  = nullptr;

    // Previous parameter values for dirty detection
    struct BandPrevValues {
        float type = -1.0f;
        float freq = -1.0f;
        float gain = -1.0f;
        float q    = -1.0f;
    };
    std::array<BandPrevValues, numBands> prevValues;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGEqProcessor)
};

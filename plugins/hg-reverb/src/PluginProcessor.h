#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <libdsp/reverb/DelayLine.h>
#include <libdsp/reverb/AllpassChain.h>
#include <libdsp/reverb/FDN.h>
#include <libdsp/util/MidSide.h>

class HGReverbProcessor : public juce::AudioProcessor
{
public:
    HGReverbProcessor();
    ~HGReverbProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HG Reverb"; }
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Predelay lines (stereo)
    libdsp::DelayLine predelayL;
    libdsp::DelayLine predelayR;

    // Early reflections (stereo allpass chains)
    libdsp::AllpassChain earlyL;
    libdsp::AllpassChain earlyR;

    // Late reverb (stereo FDN)
    libdsp::FDN fdnL;
    libdsp::FDN fdnR;

    // Cached atomic parameter pointers
    std::atomic<float>* predelayParam   = nullptr;
    std::atomic<float>* sizeParam       = nullptr;
    std::atomic<float>* decayParam      = nullptr;
    std::atomic<float>* dampingParam    = nullptr;
    std::atomic<float>* diffusionParam  = nullptr;
    std::atomic<float>* modRateParam    = nullptr;
    std::atomic<float>* modDepthParam   = nullptr;
    std::atomic<float>* earlyLevelParam = nullptr;
    std::atomic<float>* lateLevelParam  = nullptr;
    std::atomic<float>* widthParam      = nullptr;
    std::atomic<float>* mixParam        = nullptr;
    std::atomic<float>* freezeParam     = nullptr;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGReverbProcessor)
};

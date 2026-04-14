#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <libdsp/nonlinear/WaveShaper.h>
#include <libdsp/util/Oversampler.h>
#include <libdsp/filters/SVFFilter.h>

class HGSaturatorProcessor : public juce::AudioProcessor
{
public:
    HGSaturatorProcessor();
    ~HGSaturatorProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HG Saturator"; }

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

    juce::AudioProcessorValueTreeState& getAPVTS() { return m_apvts; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState m_apvts;

    // DSP
    libdsp::WaveShaper m_waveShaperL;
    libdsp::WaveShaper m_waveShaperR;
    libdsp::Oversampler m_oversamplerL;
    libdsp::Oversampler m_oversamplerR;
    libdsp::SVFFilter m_preFilterL;
    libdsp::SVFFilter m_preFilterR;
    libdsp::SVFFilter m_postFilterL;
    libdsp::SVFFilter m_postFilterR;
    libdsp::SVFFilter m_dcBlockL;
    libdsp::SVFFilter m_dcBlockR;

    int m_currentOversampleFactor = 1;

    // Cached parameter pointers
    std::atomic<float>* m_inputGain = nullptr;
    std::atomic<float>* m_mode = nullptr;
    std::atomic<float>* m_drive = nullptr;
    std::atomic<float>* m_bias = nullptr;
    std::atomic<float>* m_outputGain = nullptr;
    std::atomic<float>* m_mix = nullptr;
    std::atomic<float>* m_oversample = nullptr;
    std::atomic<float>* m_dcBlock = nullptr;
    std::atomic<float>* m_preFilterFreq = nullptr;
    std::atomic<float>* m_postFilterFreq = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGSaturatorProcessor)
};

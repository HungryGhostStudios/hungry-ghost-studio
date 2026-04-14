#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <libdsp/dynamics/Compressor.h>
#include <libdsp/util/MidSide.h>

class HGCompressorProcessor : public juce::AudioProcessor
{
public:
    HGCompressorProcessor();
    ~HGCompressorProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HG Compressor"; }

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
    float getGainReduction() const { return m_compressor.getGainReduction(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState m_apvts;
    libdsp::dynamics::Compressor m_compressor;

    // Cached parameter pointers
    std::atomic<float>* m_threshold = nullptr;
    std::atomic<float>* m_ratio = nullptr;
    std::atomic<float>* m_attack = nullptr;
    std::atomic<float>* m_release = nullptr;
    std::atomic<float>* m_knee = nullptr;
    std::atomic<float>* m_makeup = nullptr;
    std::atomic<float>* m_inputGain = nullptr;
    std::atomic<float>* m_mix = nullptr;
    std::atomic<float>* m_mode = nullptr;
    std::atomic<float>* m_detector = nullptr;
    std::atomic<float>* m_msMode = nullptr;
    std::atomic<float>* m_stereoLink = nullptr;
    std::atomic<float>* m_lookahead = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGCompressorProcessor)
};

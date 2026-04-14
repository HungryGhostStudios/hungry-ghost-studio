#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <libdsp/modulation/Chorus.h>

class HGChorusProcessor : public juce::AudioProcessor
{
public:
    HGChorusProcessor();
    ~HGChorusProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HG Chorus"; }

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
    libdsp::Chorus m_chorus;

    // Cached parameter pointers
    std::atomic<float>* m_voices = nullptr;
    std::atomic<float>* m_rate = nullptr;
    std::atomic<float>* m_depth = nullptr;
    std::atomic<float>* m_delay = nullptr;
    std::atomic<float>* m_feedback = nullptr;
    std::atomic<float>* m_mode = nullptr;
    std::atomic<float>* m_spread = nullptr;
    std::atomic<float>* m_mix = nullptr;
    std::atomic<float>* m_bbdStages = nullptr;
    std::atomic<float>* m_bbdNoise = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGChorusProcessor)
};

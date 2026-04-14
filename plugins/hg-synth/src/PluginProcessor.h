#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <libdsp/synthesis/PolyBlepOscillator.h>
#include <libdsp/filters/LadderFilter.h>
#include <libdsp/modulation/LFO.h>

// ── Synth Voice ──────────────────────────────────────────────
class HGSynthVoice : public juce::SynthesiserVoice
{
public:
    HGSynthVoice() = default;

    void setParams(juce::AudioProcessorValueTreeState& apvts);
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newValue) override;
    void controllerMoved(int controllerNumber, int newValue) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    float midiNoteToHz(int noteNumber) const;

    libdsp::synthesis::PolyBlepOscillator m_osc1;
    libdsp::synthesis::PolyBlepOscillator m_osc2;
    libdsp::LadderFilter m_filter;
    libdsp::LFO m_lfo1;
    libdsp::LFO m_lfo2;

    juce::ADSR m_ampEnv;
    juce::ADSR m_filterEnv;

    // Cached params (set per block via setParams)
    float m_osc1Tune = 0.0f, m_osc1Level = 1.0f;
    float m_osc2Tune = 0.0f, m_osc2Level = 0.0f;
    float m_filterCutoff = 5000.0f, m_filterEnvAmt = 0.0f;
    float m_filterLfoAmt = 0.0f, m_filterKbTrack = 0.0f;
    int m_lfo1Dest = 1, m_lfo2Dest = 1;
    float m_lfo1Amount = 0.0f, m_lfo2Amount = 0.0f;
    float m_pitchBendRange = 2.0f;

    float m_velocity = 0.0f;
    float m_currentFreq = 440.0f;
    float m_targetFreq = 440.0f;
    float m_glideRate = 0.0f;
    float m_glideSmooth = 0.0f;
    double m_sampleRate = 44100.0;
    int m_pitchWheelValue = 8192;
    bool m_isPrepared = false;
};

// ── Synth Sound ──────────────────────────────────────────────
class HGSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

// ── Plugin Processor ─────────────────────────────────────────
class HGSynthProcessor : public juce::AudioProcessor
{
public:
    HGSynthProcessor();
    ~HGSynthProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HG Synth"; }

    bool acceptsMidi() const override { return true; }
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
    juce::Synthesiser m_synth;

    static constexpr int kMaxVoices = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HGSynthProcessor)
};

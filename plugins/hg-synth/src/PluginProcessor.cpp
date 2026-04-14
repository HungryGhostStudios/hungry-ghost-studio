#include "PluginProcessor.h"
#include "PluginEditor.h"

// ═══════════════════════════════════════════════════════════════
//  HGSynthVoice
// ═══════════════════════════════════════════════════════════════

bool HGSynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<HGSynthSound*>(sound) != nullptr;
}

void HGSynthVoice::prepareToPlay(double sampleRate, int /*samplesPerBlock*/, int /*outputChannels*/)
{
    m_sampleRate = sampleRate;
    m_osc1.prepare(sampleRate);
    m_osc2.prepare(sampleRate);
    m_filter.prepare(sampleRate);
    m_lfo1.prepare(sampleRate);
    m_lfo2.prepare(sampleRate);
    m_ampEnv.setSampleRate(sampleRate);
    m_filterEnv.setSampleRate(sampleRate);
    m_isPrepared = true;
}

void HGSynthVoice::setParams(juce::AudioProcessorValueTreeState& apvts)
{
    // Osc 1
    m_osc1.setWaveform(static_cast<libdsp::synthesis::OscWaveform>(
        static_cast<int>(apvts.getRawParameterValue("osc1_wave")->load())));
    m_osc1.setPulseWidth(apvts.getRawParameterValue("osc1_pw")->load());
    m_osc1Tune  = apvts.getRawParameterValue("osc1_tune")->load();
    m_osc1Level = apvts.getRawParameterValue("osc1_level")->load();

    // Osc 2
    m_osc2.setWaveform(static_cast<libdsp::synthesis::OscWaveform>(
        static_cast<int>(apvts.getRawParameterValue("osc2_wave")->load())));
    m_osc2.setPulseWidth(apvts.getRawParameterValue("osc2_pw")->load());
    m_osc2Tune  = apvts.getRawParameterValue("osc2_tune")->load();
    m_osc2Level = apvts.getRawParameterValue("osc2_level")->load();

    // Filter
    m_filterCutoff = apvts.getRawParameterValue("filter_cutoff")->load();
    m_filter.setResonance(apvts.getRawParameterValue("filter_res")->load());
    m_filter.setDrive(apvts.getRawParameterValue("filter_drive")->load());
    m_filter.setType(static_cast<libdsp::LadderFilter::Type>(
        static_cast<int>(apvts.getRawParameterValue("filter_type")->load())));
    m_filterEnvAmt  = apvts.getRawParameterValue("filter_env_amt")->load();
    m_filterLfoAmt  = apvts.getRawParameterValue("filter_lfo_amt")->load();
    m_filterKbTrack = apvts.getRawParameterValue("filter_kb_track")->load();

    // Amp ADSR
    juce::ADSR::Parameters ampP;
    ampP.attack  = apvts.getRawParameterValue("amp_attack")->load();
    ampP.decay   = apvts.getRawParameterValue("amp_decay")->load();
    ampP.sustain = apvts.getRawParameterValue("amp_sustain")->load();
    ampP.release = apvts.getRawParameterValue("amp_release")->load();
    m_ampEnv.setParameters(ampP);

    // Filter ADSR
    juce::ADSR::Parameters fltP;
    fltP.attack  = apvts.getRawParameterValue("filter_attack")->load();
    fltP.decay   = apvts.getRawParameterValue("filter_decay")->load();
    fltP.sustain = apvts.getRawParameterValue("filter_sustain")->load();
    fltP.release = apvts.getRawParameterValue("filter_release")->load();
    m_filterEnv.setParameters(fltP);

    // LFO 1
    m_lfo1.setFrequency(apvts.getRawParameterValue("lfo1_rate")->load());
    m_lfo1.setWaveform(static_cast<libdsp::LFO::Waveform>(
        static_cast<int>(apvts.getRawParameterValue("lfo1_wave")->load())));
    m_lfo1Dest   = static_cast<int>(apvts.getRawParameterValue("lfo1_dest")->load());
    m_lfo1Amount = apvts.getRawParameterValue("lfo1_amount")->load();

    // LFO 2
    m_lfo2.setFrequency(apvts.getRawParameterValue("lfo2_rate")->load());
    m_lfo2.setWaveform(static_cast<libdsp::LFO::Waveform>(
        static_cast<int>(apvts.getRawParameterValue("lfo2_wave")->load())));
    m_lfo2Dest   = static_cast<int>(apvts.getRawParameterValue("lfo2_dest")->load());
    m_lfo2Amount = apvts.getRawParameterValue("lfo2_amount")->load();

    // Master
    m_pitchBendRange = apvts.getRawParameterValue("master_pitch_bend")->load();

    const float glideTime = apvts.getRawParameterValue("master_glide")->load();
    m_glideRate = glideTime;
    if (glideTime > 0.0f)
        m_glideSmooth = std::exp(-1.0f / (glideTime * static_cast<float>(m_sampleRate)));
    else
        m_glideSmooth = 0.0f;
}

void HGSynthVoice::startNote(int midiNoteNumber, float velocity,
                              juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    m_velocity = velocity;
    m_pitchWheelValue = currentPitchWheelPosition;

    const float newFreq = midiNoteToHz(midiNoteNumber);

    if (m_glideRate > 0.0f && m_currentFreq > 0.0f)
    {
        m_targetFreq = newFreq;
    }
    else
    {
        m_currentFreq = newFreq;
        m_targetFreq = newFreq;
    }

    m_filter.reset();
    m_lfo1.reset();
    m_lfo2.reset();
    m_ampEnv.noteOn();
    m_filterEnv.noteOn();
}

void HGSynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        m_ampEnv.noteOff();
        m_filterEnv.noteOff();
    }
    else
    {
        m_ampEnv.reset();
        m_filterEnv.reset();
        clearCurrentNote();
    }
}

void HGSynthVoice::pitchWheelMoved(int newValue)
{
    m_pitchWheelValue = newValue;
}

void HGSynthVoice::controllerMoved(int, int) {}

void HGSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                    int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    // Pitch wheel: center = 8192, range ±pitchBendRange semitones
    const float pitchBendSemitones = m_pitchBendRange *
        (static_cast<float>(m_pitchWheelValue) - 8192.0f) / 8192.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Glide (portamento)
        if (m_glideSmooth > 0.0f && std::abs(m_currentFreq - m_targetFreq) > 0.01f)
            m_currentFreq = m_targetFreq + m_glideSmooth * (m_currentFreq - m_targetFreq);
        else
            m_currentFreq = m_targetFreq;

        // LFO processing
        const float lfo1Val = m_lfo1.process();
        const float lfo2Val = m_lfo2.process();

        // Compute pitch modulation from LFOs
        float pitchMod = pitchBendSemitones;
        if (m_lfo1Dest == 0) pitchMod += lfo1Val * m_lfo1Amount; // 0 = Pitch
        if (m_lfo2Dest == 0) pitchMod += lfo2Val * m_lfo2Amount;

        const float freqMultiplier = std::pow(2.0f, pitchMod / 12.0f);

        // Osc frequencies with tune offsets
        m_osc1.setFrequencyHz(m_currentFreq * std::pow(2.0f, m_osc1Tune / 12.0f) * freqMultiplier);
        m_osc2.setFrequencyHz(m_currentFreq * std::pow(2.0f, m_osc2Tune / 12.0f) * freqMultiplier);

        // Generate oscillator outputs and mix
        const float mixed = m_osc1.processSample() * m_osc1Level
                          + m_osc2.processSample() * m_osc2Level;

        // Filter envelope modulation
        const float filterEnvVal = m_filterEnv.getNextSample();
        float cutoffMod = m_filterCutoff;

        // Keyboard tracking: scale cutoff relative to middle C (261.63 Hz)
        if (m_filterKbTrack > 0.0f)
        {
            const float kbSemitones = 12.0f * std::log2(m_currentFreq / 261.63f);
            cutoffMod *= std::pow(2.0f, m_filterKbTrack * kbSemitones / 12.0f);
        }

        // Envelope modulation (bipolar, in Hz)
        cutoffMod += m_filterEnvAmt * filterEnvVal;

        // LFO filter modulation
        float filterLfoMod = 0.0f;
        if (m_lfo1Dest == 1) filterLfoMod += lfo1Val * m_lfo1Amount; // 1 = Filter
        if (m_lfo2Dest == 1) filterLfoMod += lfo2Val * m_lfo2Amount;
        cutoffMod += m_filterLfoAmt * filterLfoMod;

        m_filter.setCutoff(std::clamp(cutoffMod, 20.0f, 20000.0f));
        float filtered = m_filter.process(mixed);

        // Amplitude envelope
        const float ampEnvVal = m_ampEnv.getNextSample();

        // LFO amplitude modulation
        float ampMod = 1.0f;
        if (m_lfo1Dest == 2) ampMod += lfo1Val * m_lfo1Amount * 0.5f; // 2 = Amplitude
        if (m_lfo2Dest == 2) ampMod += lfo2Val * m_lfo2Amount * 0.5f;
        ampMod = std::max(ampMod, 0.0f);

        const float sample = filtered * ampEnvVal * m_velocity * ampMod;

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample(ch, startSample + i, sample);

        if (!m_ampEnv.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

float HGSynthVoice::midiNoteToHz(int noteNumber) const
{
    return 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}

// ═══════════════════════════════════════════════════════════════
//  HGSynthProcessor
// ═══════════════════════════════════════════════════════════════

HGSynthProcessor::HGSynthProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    m_synth.addSound(new HGSynthSound());

    for (int i = 0; i < kMaxVoices; ++i)
        m_synth.addVoice(new HGSynthVoice());
}

void HGSynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < m_synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<HGSynthVoice*>(m_synth.getVoice(i)))
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    }
}

void HGSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Update voice parameters from APVTS
    for (int i = 0; i < m_synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<HGSynthVoice*>(m_synth.getVoice(i)))
            voice->setParams(m_apvts);
    }

    m_synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* HGSynthProcessor::createEditor()
{
    return new HGSynthEditor(*this);
}

void HGSynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HGSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(m_apvts.state.getType()))
        m_apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout
HGSynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray waveforms{"Saw", "Square", "Triangle", "Sine"};
    juce::StringArray lfoWaveforms{"Sine", "Triangle", "Saw", "Square", "S&H"};
    juce::StringArray lfoDests{"Pitch", "Filter", "Amplitude"};
    juce::StringArray filterTypes{"LP24", "HP24"};

    // ── Osc 1 ──
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"osc1_wave", 1}, "Osc 1 Wave", waveforms, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_tune", 1}, "Osc 1 Tune",
        juce::NormalisableRange<float>(-24.f, 24.f, 1.f), 0.f, "st"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_pw", 1}, "Osc 1 PW",
        juce::NormalisableRange<float>(0.01f, 0.99f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc1_level", 1}, "Osc 1 Level",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));

    // ── Osc 2 ──
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"osc2_wave", 1}, "Osc 2 Wave", waveforms, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_tune", 1}, "Osc 2 Tune",
        juce::NormalisableRange<float>(-24.f, 24.f, 1.f), 0.f, "st"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_pw", 1}, "Osc 2 PW",
        juce::NormalisableRange<float>(0.01f, 0.99f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc2_level", 1}, "Osc 2 Level",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));

    // ── Filter ──
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_cutoff", 1}, "Filter Cutoff",
        juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.3f), 5000.f, "Hz"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_res", 1}, "Filter Resonance",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_env_amt", 1}, "Filter Env Amount",
        juce::NormalisableRange<float>(-10000.f, 10000.f, 1.f), 0.f, "Hz"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_lfo_amt", 1}, "Filter LFO Amount",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_kb_track", 1}, "Filter KB Track",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_drive", 1}, "Filter Drive",
        juce::NormalisableRange<float>(0.f, 5.f, 0.01f), 1.f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"filter_type", 1}, "Filter Type", filterTypes, 0));

    // ── Amp ADSR ──
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_attack", 1}, "Amp Attack",
        juce::NormalisableRange<float>(0.001f, 5.f, 0.001f, 0.4f), 0.01f, "s"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_decay", 1}, "Amp Decay",
        juce::NormalisableRange<float>(0.001f, 5.f, 0.001f, 0.4f), 0.3f, "s"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_sustain", 1}, "Amp Sustain",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_release", 1}, "Amp Release",
        juce::NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.5f, "s"));

    // ── Filter ADSR ──
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_attack", 1}, "Filter Attack",
        juce::NormalisableRange<float>(0.001f, 5.f, 0.001f, 0.4f), 0.01f, "s"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_decay", 1}, "Filter Decay",
        juce::NormalisableRange<float>(0.001f, 5.f, 0.001f, 0.4f), 0.5f, "s"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_sustain", 1}, "Filter Sustain",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_release", 1}, "Filter Release",
        juce::NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.5f, "s"));

    // ── LFO 1 ──
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1_rate", 1}, "LFO 1 Rate",
        juce::NormalisableRange<float>(0.01f, 20.f, 0.01f, 0.4f), 1.f, "Hz"));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo1_wave", 1}, "LFO 1 Wave", lfoWaveforms, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo1_dest", 1}, "LFO 1 Dest", lfoDests, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1_amount", 1}, "LFO 1 Amount",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));

    // ── LFO 2 ──
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2_rate", 1}, "LFO 2 Rate",
        juce::NormalisableRange<float>(0.01f, 20.f, 0.01f, 0.4f), 1.f, "Hz"));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo2_wave", 1}, "LFO 2 Wave", lfoWaveforms, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo2_dest", 1}, "LFO 2 Dest", lfoDests, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2_amount", 1}, "LFO 2 Amount",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));

    // ── Master ──
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"master_voices", 1}, "Voices",
        juce::NormalisableRange<float>(1.f, 16.f, 1.f), 16.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"master_glide", 1}, "Glide",
        juce::NormalisableRange<float>(0.f, 2.f, 0.001f, 0.4f), 0.f, "s"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"master_pitch_bend", 1}, "Pitch Bend Range",
        juce::NormalisableRange<float>(0.f, 24.f, 1.f), 2.f, "st"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"master_unison", 1}, "Unison Detune",
        juce::NormalisableRange<float>(0.f, 50.f, 0.1f), 0.f, "cents"));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HGSynthProcessor();
}

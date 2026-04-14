#include "PluginProcessor.h"
#include "PluginEditor.h"

HGChorusProcessor::HGChorusProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    m_voices    = m_apvts.getRawParameterValue("voices");
    m_rate      = m_apvts.getRawParameterValue("rate");
    m_depth     = m_apvts.getRawParameterValue("depth");
    m_delay     = m_apvts.getRawParameterValue("delay");
    m_feedback  = m_apvts.getRawParameterValue("feedback");
    m_mode      = m_apvts.getRawParameterValue("mode");
    m_spread    = m_apvts.getRawParameterValue("spread");
    m_mix       = m_apvts.getRawParameterValue("mix");
    m_bbdStages = m_apvts.getRawParameterValue("bbd_stages");
    m_bbdNoise  = m_apvts.getRawParameterValue("bbd_noise");
}

void HGChorusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_chorus.prepare(sampleRate, samplesPerBlock);
}

void HGChorusProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // Update chorus parameters from APVTS
    m_chorus.setVoices(static_cast<int>(m_voices->load()));
    m_chorus.setRate(m_rate->load());
    m_chorus.setDepth(m_depth->load());
    m_chorus.setDelay(m_delay->load());
    m_chorus.setFeedback(m_feedback->load());
    m_chorus.setSpread(m_spread->load());

    // Set mode: 0 = Clean, 1 = BBD
    const int modeVal = static_cast<int>(m_mode->load());
    m_chorus.setMode(modeVal == 0 ? libdsp::Chorus::Mode::Clean
                                  : libdsp::Chorus::Mode::BBD);

    // Dry/wet mix
    const float mixAmount = m_mix->load() / 100.0f;

    // Process sample by sample with dry/wet mix
    for (int i = 0; i < numSamples; ++i)
    {
        const float dryL = left[i];
        const float dryR = right[i];

        auto wet = m_chorus.process(dryL, dryR);

        left[i]  = dryL * (1.0f - mixAmount) + wet.left * mixAmount;
        right[i] = dryR * (1.0f - mixAmount) + wet.right * mixAmount;
    }
}

juce::AudioProcessorEditor* HGChorusProcessor::createEditor()
{
    return new HGChorusEditor(*this);
}

void HGChorusProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HGChorusProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(m_apvts.state.getType()))
        m_apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout
HGChorusProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"voices", 1}, "Voices", 2, 6, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rate", 1}, "Rate",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.4f), 0.5f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"depth", 1}, "Depth",
        juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f), 3.0f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay", 1}, "Delay",
        juce::NormalisableRange<float>(1.0f, 30.0f, 0.1f), 7.0f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"feedback", 1}, "Feedback",
        juce::NormalisableRange<float>(-0.95f, 0.95f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Mode",
        juce::StringArray{"Clean", "BBD"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"spread", 1}, "Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"bbd_stages", 1}, "BBD Stages", 256, 4096, 512));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"bbd_noise", 1}, "BBD Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HGChorusProcessor();
}

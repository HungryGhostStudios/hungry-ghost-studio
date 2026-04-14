#include "PluginProcessor.h"
#include "PluginEditor.h"

HGCompressorProcessor::HGCompressorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    m_threshold   = m_apvts.getRawParameterValue("threshold");
    m_ratio       = m_apvts.getRawParameterValue("ratio");
    m_attack      = m_apvts.getRawParameterValue("attack");
    m_release     = m_apvts.getRawParameterValue("release");
    m_knee        = m_apvts.getRawParameterValue("knee");
    m_makeup      = m_apvts.getRawParameterValue("makeup");
    m_inputGain   = m_apvts.getRawParameterValue("input_gain");
    m_mix         = m_apvts.getRawParameterValue("mix");
    m_mode        = m_apvts.getRawParameterValue("mode");
    m_detector    = m_apvts.getRawParameterValue("detector");
    m_msMode      = m_apvts.getRawParameterValue("ms_mode");
    m_stereoLink  = m_apvts.getRawParameterValue("stereo_link");
    m_lookahead   = m_apvts.getRawParameterValue("lookahead");
}

void HGCompressorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_compressor.prepare(sampleRate, samplesPerBlock);
}

void HGCompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // Apply input gain
    const float inputGainLinear = std::pow(10.f, m_inputGain->load() / 20.f);
    if (std::abs(inputGainLinear - 1.f) > 1e-6f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  *= inputGainLinear;
            right[i] *= inputGainLinear;
        }
    }

    // M/S encoding if needed
    const int msMode = static_cast<int>(m_msMode->load());
    // 0 = Stereo, 1 = Mid, 2 = Side
    if (msMode > 0)
        libdsp::util::MidSide::encode(left, right, numSamples);

    // Update compressor parameters
    m_compressor.setThreshold(m_threshold->load());
    m_compressor.setRatio(m_ratio->load());
    m_compressor.setAttack(m_attack->load());
    m_compressor.setRelease(m_release->load());
    m_compressor.setKnee(m_knee->load());
    m_compressor.setMakeup(m_makeup->load());
    m_compressor.setMix(m_mix->load());
    m_compressor.setLookahead(m_lookahead->load());
    m_compressor.setStereoLink(m_stereoLink->load() >= 0.5f);

    // Set compressor mode
    const int modeVal = static_cast<int>(m_mode->load());
    switch (modeVal)
    {
        case 0: m_compressor.setMode(libdsp::dynamics::CompressorMode::FeedForward); break;
        case 1: m_compressor.setMode(libdsp::dynamics::CompressorMode::FeedBack); break;
        case 2: m_compressor.setMode(libdsp::dynamics::CompressorMode::Opto); break;
        default: break;
    }

    // Process compression
    m_compressor.process(left, right, numSamples);

    // M/S decoding if needed
    if (msMode > 0)
        libdsp::util::MidSide::decode(left, right, numSamples);
}

juce::AudioProcessorEditor* HGCompressorProcessor::createEditor()
{
    return new HGCompressorEditor(*this);
}

void HGCompressorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HGCompressorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(m_apvts.state.getType()))
        m_apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout
HGCompressorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"threshold", 1}, "Threshold",
        juce::NormalisableRange<float>(-60.f, 0.f, 0.1f), -20.f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"ratio", 1}, "Ratio",
        juce::NormalisableRange<float>(1.f, 20.f, 0.1f, 0.5f), 4.f, ":1"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"attack", 1}, "Attack",
        juce::NormalisableRange<float>(0.1f, 200.f, 0.1f, 0.4f), 10.f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"release", 1}, "Release",
        juce::NormalisableRange<float>(10.f, 2000.f, 1.f, 0.4f), 100.f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"knee", 1}, "Knee",
        juce::NormalisableRange<float>(0.f, 12.f, 0.1f), 6.f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"makeup", 1}, "Makeup",
        juce::NormalisableRange<float>(-12.f, 24.f, 0.1f), 0.f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"input_gain", 1}, "Input Gain",
        juce::NormalisableRange<float>(-12.f, 12.f, 0.1f), 0.f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix",
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 100.f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Mode",
        juce::StringArray{"Feed-Forward", "Feed-Back", "Opto"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"detector", 1}, "Detector",
        juce::StringArray{"Peak", "RMS"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"ms_mode", 1}, "M/S Mode",
        juce::StringArray{"Stereo", "Mid", "Side"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"stereo_link", 1}, "Stereo Link", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lookahead", 1}, "Lookahead",
        juce::NormalisableRange<float>(0.f, 10.f, 0.1f), 0.f, "ms"));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HGCompressorProcessor();
}

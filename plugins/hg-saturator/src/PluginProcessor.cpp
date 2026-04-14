#include "PluginProcessor.h"
#include "PluginEditor.h"

static int oversampleIndexToFactor(int index)
{
    switch (index)
    {
        case 0: return 1;
        case 1: return 2;
        case 2: return 4;
        case 3: return 8;
        default: return 1;
    }
}

HGSaturatorProcessor::HGSaturatorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      m_apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    m_inputGain     = m_apvts.getRawParameterValue("input_gain");
    m_mode          = m_apvts.getRawParameterValue("mode");
    m_drive         = m_apvts.getRawParameterValue("drive");
    m_bias          = m_apvts.getRawParameterValue("bias");
    m_outputGain    = m_apvts.getRawParameterValue("output_gain");
    m_mix           = m_apvts.getRawParameterValue("mix");
    m_oversample    = m_apvts.getRawParameterValue("oversample");
    m_dcBlock       = m_apvts.getRawParameterValue("dc_block");
    m_preFilterFreq = m_apvts.getRawParameterValue("pre_filter_freq");
    m_postFilterFreq = m_apvts.getRawParameterValue("post_filter_freq");
}

void HGSaturatorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Determine oversample factor
    m_currentOversampleFactor = oversampleIndexToFactor(static_cast<int>(m_oversample->load()));

    // Prepare oversamplers
    m_oversamplerL.prepare(sampleRate, samplesPerBlock, m_currentOversampleFactor);
    m_oversamplerR.prepare(sampleRate, samplesPerBlock, m_currentOversampleFactor);

    // Prepare waveshapers at oversampled rate
    const double osRate = sampleRate * m_currentOversampleFactor;
    m_waveShaperL.prepare(osRate);
    m_waveShaperR.prepare(osRate);

    // Prepare pre/post filters at base rate
    m_preFilterL.prepare(sampleRate);
    m_preFilterR.prepare(sampleRate);
    m_postFilterL.prepare(sampleRate);
    m_postFilterR.prepare(sampleRate);

    m_preFilterL.setType(libdsp::SVFFilter::Type::LowPass);
    m_preFilterR.setType(libdsp::SVFFilter::Type::LowPass);
    m_postFilterL.setType(libdsp::SVFFilter::Type::LowPass);
    m_postFilterR.setType(libdsp::SVFFilter::Type::LowPass);

    // DC blocker: HPF at ~5Hz
    m_dcBlockL.prepare(sampleRate);
    m_dcBlockR.prepare(sampleRate);
    m_dcBlockL.setType(libdsp::SVFFilter::Type::HighPass);
    m_dcBlockR.setType(libdsp::SVFFilter::Type::HighPass);
    m_dcBlockL.setFrequency(5.0f);
    m_dcBlockR.setFrequency(5.0f);
}

void HGSaturatorProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // Read parameters
    const float inputGainDb  = m_inputGain->load();
    const float outputGainDb = m_outputGain->load();
    const float drivePercent = m_drive->load();
    const float bias         = m_bias->load();
    const float mixPercent   = m_mix->load();
    const bool  dcBlockOn    = m_dcBlock->load() >= 0.5f;
    const float preFreq      = m_preFilterFreq->load();
    const float postFreq     = m_postFilterFreq->load();
    const int   modeVal      = static_cast<int>(m_mode->load());

    const float inputGainLin  = std::pow(10.f, inputGainDb / 20.f);
    const float outputGainLin = std::pow(10.f, outputGainDb / 20.f);
    const float mixNorm       = mixPercent / 100.f;

    // Map drive percent to waveshaper drive (0-100% → 0-10 drive range)
    const float driveAmount = drivePercent / 10.f;

    // Set waveshaper mode
    libdsp::WaveShaper::Mode wsMode;
    switch (modeVal)
    {
        case 0: wsMode = libdsp::WaveShaper::Mode::Soft; break;
        case 1: wsMode = libdsp::WaveShaper::Mode::Hard; break;
        case 2: wsMode = libdsp::WaveShaper::Mode::Tape; break;
        case 3: wsMode = libdsp::WaveShaper::Mode::Fold; break;
        case 4: wsMode = libdsp::WaveShaper::Mode::Asym; break;
        default: wsMode = libdsp::WaveShaper::Mode::Soft; break;
    }

    m_waveShaperL.setMode(wsMode);
    m_waveShaperR.setMode(wsMode);
    m_waveShaperL.setDrive(driveAmount);
    m_waveShaperR.setDrive(driveAmount);
    m_waveShaperL.setBias(bias);
    m_waveShaperR.setBias(bias);
    m_waveShaperL.setADAA(true);
    m_waveShaperR.setADAA(true);
    // Disable WaveShaper's internal DC block — we use our own SVF-based one
    m_waveShaperL.setDCBlock(false);
    m_waveShaperR.setDCBlock(false);

    // Update pre/post filter frequencies
    m_preFilterL.setFrequency(preFreq);
    m_preFilterR.setFrequency(preFreq);
    m_postFilterL.setFrequency(postFreq);
    m_postFilterR.setFrequency(postFreq);

    // Check if oversample factor changed
    const int newFactor = oversampleIndexToFactor(static_cast<int>(m_oversample->load()));
    if (newFactor != m_currentOversampleFactor)
    {
        m_currentOversampleFactor = newFactor;
        m_oversamplerL.prepare(getSampleRate(), numSamples, m_currentOversampleFactor);
        m_oversamplerR.prepare(getSampleRate(), numSamples, m_currentOversampleFactor);
        m_waveShaperL.prepare(getSampleRate() * m_currentOversampleFactor);
        m_waveShaperR.prepare(getSampleRate() * m_currentOversampleFactor);
    }

    // Store dry signal for mix
    std::vector<float> dryL(left, left + numSamples);
    std::vector<float> dryR(right, right + numSamples);

    // 1. Input gain
    for (int i = 0; i < numSamples; ++i)
    {
        left[i]  *= inputGainLin;
        right[i] *= inputGainLin;
    }

    // 2. Pre-filter (LP)
    for (int i = 0; i < numSamples; ++i)
    {
        left[i]  = m_preFilterL.process(left[i]);
        right[i] = m_preFilterR.process(right[i]);
    }

    // 3. Upsample
    float* osL = m_oversamplerL.upsample(left, numSamples);
    float* osR = m_oversamplerR.upsample(right, numSamples);
    const int osSamples = m_oversamplerL.getOversampledBlockSize(numSamples);

    // 4. WaveShaper + ADAA processing at oversampled rate
    for (int i = 0; i < osSamples; ++i)
    {
        osL[i] = m_waveShaperL.process(osL[i]);
        osR[i] = m_waveShaperR.process(osR[i]);
    }

    // 5. Downsample
    m_oversamplerL.downsample(left, numSamples);
    m_oversamplerR.downsample(right, numSamples);

    // 6. Post-filter (LP)
    for (int i = 0; i < numSamples; ++i)
    {
        left[i]  = m_postFilterL.process(left[i]);
        right[i] = m_postFilterR.process(right[i]);
    }

    // 7. DC block (HPF ~5Hz)
    if (dcBlockOn)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  = m_dcBlockL.process(left[i]);
            right[i] = m_dcBlockR.process(right[i]);
        }
    }

    // 8. Output gain
    for (int i = 0; i < numSamples; ++i)
    {
        left[i]  *= outputGainLin;
        right[i] *= outputGainLin;
    }

    // 9. Dry/wet mix
    for (int i = 0; i < numSamples; ++i)
    {
        left[i]  = dryL[static_cast<size_t>(i)] * (1.f - mixNorm) + left[i] * mixNorm;
        right[i] = dryR[static_cast<size_t>(i)] * (1.f - mixNorm) + right[i] * mixNorm;
    }
}

juce::AudioProcessorEditor* HGSaturatorProcessor::createEditor()
{
    return new HGSaturatorEditor(*this);
}

void HGSaturatorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = m_apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HGSaturatorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(m_apvts.state.getType()))
        m_apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout
HGSaturatorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"input_gain", 1}, "Input Gain",
        juce::NormalisableRange<float>(-12.f, 24.f, 0.1f), 0.f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Mode",
        juce::StringArray{"Soft", "Hard", "Tape", "Fold", "Asym"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1}, "Drive",
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 50.f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"bias", 1}, "Bias",
        juce::NormalisableRange<float>(-0.5f, 0.5f, 0.01f), 0.f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"output_gain", 1}, "Output Gain",
        juce::NormalisableRange<float>(-24.f, 12.f, 0.1f), 0.f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix",
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 100.f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"oversample", 1}, "Oversample",
        juce::StringArray{"1x", "2x", "4x", "8x"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"dc_block", 1}, "DC Block", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pre_filter_freq", 1}, "Pre Filter",
        juce::NormalisableRange<float>(1000.f, 20000.f, 1.f, 0.3f), 20000.f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"post_filter_freq", 1}, "Post Filter",
        juce::NormalisableRange<float>(1000.f, 20000.f, 1.f, 0.3f), 20000.f, "Hz"));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HGSaturatorProcessor();
}

#include "PluginProcessor.h"

HGEqProcessor::HGEqProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int b = 0; b < numBands; ++b)
    {
        auto id = juce::String(b + 1);
        bandParams[b].type    = apvts.getRawParameterValue("band" + id + "_type");
        bandParams[b].freq    = apvts.getRawParameterValue("band" + id + "_freq");
        bandParams[b].gain    = apvts.getRawParameterValue("band" + id + "_gain");
        bandParams[b].q       = apvts.getRawParameterValue("band" + id + "_q");
        bandParams[b].enabled = apvts.getRawParameterValue("band" + id + "_enabled");
    }
    msModeParam      = apvts.getRawParameterValue("ms_mode");
    linearPhaseParam = apvts.getRawParameterValue("linear_phase");
    outputGainParam  = apvts.getRawParameterValue("output_gain");
}

juce::AudioProcessorValueTreeState::ParameterLayout HGEqProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int b = 1; b <= numBands; ++b)
    {
        auto id = juce::String(b);
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "band" + id + "_type", "Band " + id + " Type", 0, 6, 4)); // default Peak
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "band" + id + "_freq", "Band " + id + " Freq",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), // log skew
            1000.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "band" + id + "_gain", "Band " + id + " Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
            0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "band" + id + "_q", "Band " + id + " Q",
            juce::NormalisableRange<float>(0.1f, 18.0f, 0.01f, 0.5f),
            0.707f));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "band" + id + "_enabled", "Band " + id + " Enabled", true));
    }

    layout.add(std::make_unique<juce::AudioParameterInt>(
        "ms_mode", "M/S Mode", 0, 2, 0)); // 0=Stereo, 1=Mid, 2=Side
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "linear_phase", "Linear Phase", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "output_gain", "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    return layout;
}

void HGEqProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    for (int b = 0; b < numBands; ++b)
    {
        eqBandsL[b].prepare(sampleRate);
        eqBandsL[b].setNumStages(1);
        eqBandsR[b].prepare(sampleRate);
        eqBandsR[b].setNumStages(1);

        // Force coefficient recalculation
        prevValues[b] = {-1.0f, -1.0f, -1.0f, -1.0f};
    }
}

void HGEqProcessor::updateCoefficients(int band, float sampleRate)
{
    const int filterType = static_cast<int>(bandParams[band].type->load());
    const float freq     = bandParams[band].freq->load();
    const float gain     = bandParams[band].gain->load();
    const float q        = bandParams[band].q->load();

    libdsp::EQCoeffs::CoeffArray coeffs;
    switch (filterType)
    {
        case 0: coeffs = libdsp::EQCoeffs::makeLP(freq, q, sampleRate); break;
        case 1: coeffs = libdsp::EQCoeffs::makeHP(freq, q, sampleRate); break;
        case 2: coeffs = libdsp::EQCoeffs::makeLowShelf(freq, gain, q, sampleRate); break;
        case 3: coeffs = libdsp::EQCoeffs::makeHighShelf(freq, gain, q, sampleRate); break;
        case 4: coeffs = libdsp::EQCoeffs::makePeaking(freq, gain, q, sampleRate); break;
        case 5: coeffs = libdsp::EQCoeffs::makeNotch(freq, q, sampleRate); break;
        case 6: coeffs = libdsp::EQCoeffs::makeAllpass(freq, q, sampleRate); break;
        default: coeffs = libdsp::EQCoeffs::makePeaking(freq, gain, q, sampleRate); break;
    }

    eqBandsL[band].setCoefficients(0, coeffs);
    eqBandsR[band].setCoefficients(0, coeffs);
}

void HGEqProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numChannels < 2 || numSamples == 0)
        return;

    const int msMode = static_cast<int>(msModeParam->load());
    const float outputGainDb = outputGainParam->load();
    const float outputGainLinear = std::pow(10.0f, outputGainDb / 20.0f);

    // Check for coefficient changes and update
    for (int b = 0; b < numBands; ++b)
    {
        const float type = bandParams[b].type->load();
        const float freq = bandParams[b].freq->load();
        const float gain = bandParams[b].gain->load();
        const float q    = bandParams[b].q->load();

        if (type != prevValues[b].type || freq != prevValues[b].freq ||
            gain != prevValues[b].gain || q != prevValues[b].q)
        {
            updateCoefficients(b, static_cast<float>(currentSampleRate));
            prevValues[b] = {type, freq, gain, q};
        }
    }

    auto* leftChannel  = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    for (int n = 0; n < numSamples; ++n)
    {
        float left  = leftChannel[n];
        float right = rightChannel[n];

        // M/S encode if needed
        float procL = left;
        float procR = right;
        if (msMode != 0)
        {
            auto [mid, side] = libdsp::MidSide::encode(left, right);
            procL = mid;
            procR = side;
        }

        // Apply 8 EQ bands in series
        for (int b = 0; b < numBands; ++b)
        {
            if (bandParams[b].enabled->load() < 0.5f)
                continue;

            if (msMode == 2) // Side-only: only process procR (side)
            {
                procR = eqBandsR[b].process(procR);
            }
            else if (msMode == 1) // Mid-only: only process procL (mid)
            {
                procL = eqBandsL[b].process(procL);
            }
            else // Stereo: process both
            {
                procL = eqBandsL[b].process(procL);
                procR = eqBandsR[b].process(procR);
            }
        }

        // M/S decode if needed
        if (msMode != 0)
        {
            auto [l, r] = libdsp::MidSide::decode(procL, procR);
            procL = l;
            procR = r;
        }

        // Output gain
        leftChannel[n]  = procL * outputGainLinear;
        rightChannel[n] = procR * outputGainLinear;
    }
}

void HGEqProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HGEqProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HGEqProcessor();
}

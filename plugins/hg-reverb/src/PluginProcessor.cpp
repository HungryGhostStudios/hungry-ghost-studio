#include "PluginProcessor.h"
#include <cmath>

HGReverbProcessor::HGReverbProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    predelayParam   = apvts.getRawParameterValue("predelay");
    sizeParam       = apvts.getRawParameterValue("size");
    decayParam      = apvts.getRawParameterValue("decay");
    dampingParam    = apvts.getRawParameterValue("damping");
    diffusionParam  = apvts.getRawParameterValue("diffusion");
    modRateParam    = apvts.getRawParameterValue("mod_rate");
    modDepthParam   = apvts.getRawParameterValue("mod_depth");
    earlyLevelParam = apvts.getRawParameterValue("early_level");
    lateLevelParam  = apvts.getRawParameterValue("late_level");
    widthParam      = apvts.getRawParameterValue("width");
    mixParam        = apvts.getRawParameterValue("mix");
    freezeParam     = apvts.getRawParameterValue("freeze");
}

juce::AudioProcessorValueTreeState::ParameterLayout HGReverbProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "predelay", "Predelay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "size", "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "decay", "Decay",
        juce::NormalisableRange<float>(0.1f, 30.0f, 0.01f, 0.4f), 2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "damping", "Damping",
        juce::NormalisableRange<float>(200.0f, 10000.0f, 1.0f, 0.4f), 5000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "diffusion", "Diffusion",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 70.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mod_rate", "Mod Rate",
        juce::NormalisableRange<float>(0.0f, 5.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mod_depth", "Mod Depth",
        juce::NormalisableRange<float>(0.0f, 5.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "early_level", "Early Level",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -6.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "late_level", "Late Level",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -3.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "width", "Width",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "freeze", "Freeze", false));

    return layout;
}

void HGReverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Predelay: max 100ms
    const int maxPredelaySamples = static_cast<int>(sampleRate * 0.1) + 1;
    predelayL.prepare(sampleRate, maxPredelaySamples);
    predelayR.prepare(sampleRate, maxPredelaySamples);

    // Early reflections
    earlyL.prepare(sampleRate, 4096);
    earlyR.prepare(sampleRate, 4096);
    earlyL.setNumStages(4);
    earlyR.setNumStages(4);

    // Set prime-number delays for early reflection stages (slightly different L/R for stereo)
    const int earlyDelaysL[] = {142, 379, 607, 883};
    const int earlyDelaysR[] = {157, 397, 631, 911};
    for (int i = 0; i < 4; ++i)
    {
        earlyL.setStageParams(i, earlyDelaysL[i], 0.5f);
        earlyR.setStageParams(i, earlyDelaysR[i], 0.5f);
    }

    // Late reverb FDN
    fdnL.prepare(sampleRate, samplesPerBlock);
    fdnR.prepare(sampleRate, samplesPerBlock);
}

void HGReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numChannels < 2 || numSamples == 0)
        return;

    // Read parameters
    const float predelayMs = predelayParam->load();
    const float size       = sizeParam->load() / 100.0f;       // 0..1
    const float decay      = decayParam->load();
    const float damping    = dampingParam->load();
    const float diffusion  = diffusionParam->load() / 100.0f;  // 0..1
    const float modRate    = modRateParam->load();
    const float modDepth   = modDepthParam->load();
    const float earlyDb    = earlyLevelParam->load();
    const float lateDb     = lateLevelParam->load();
    const float width      = widthParam->load() / 100.0f;      // 0..1
    const float mixPct     = mixParam->load() / 100.0f;         // 0..1
    const bool  freeze     = freezeParam->load() > 0.5f;

    // Convert dB levels to linear
    const float earlyGain = (earlyDb <= -59.9f) ? 0.0f : std::pow(10.0f, earlyDb / 20.0f);
    const float lateGain  = (lateDb <= -59.9f) ? 0.0f : std::pow(10.0f, lateDb / 20.0f);

    // Update predelay
    predelayL.setDelayMs(predelayMs);
    predelayR.setDelayMs(predelayMs);

    // Update early reflection diffusion (re-set delays + scaled gain)
    {
        const int earlyDelaysL[] = {142, 379, 607, 883};
        const int earlyDelaysR[] = {157, 397, 631, 911};
        const float apGain = diffusion * 0.7f;
        for (int i = 0; i < 4; ++i)
        {
            earlyL.setStageParams(i, earlyDelaysL[i], apGain);
            earlyR.setStageParams(i, earlyDelaysR[i], apGain);
        }
    }

    // Update FDN parameters
    fdnL.setDecay(decay);
    fdnR.setDecay(decay);
    fdnL.setDamping(damping);
    fdnR.setDamping(damping);
    fdnL.setSize(size);
    fdnR.setSize(size);
    fdnL.setModRate(modRate);
    fdnR.setModRate(modRate);
    fdnL.setModDepth(modDepth);
    fdnR.setModDepth(modDepth);
    fdnL.setFreeze(freeze);
    fdnR.setFreeze(freeze);
    fdnL.setDiffusion(diffusion);
    fdnR.setDiffusion(diffusion);

    auto* leftChannel  = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    for (int n = 0; n < numSamples; ++n)
    {
        const float dryL = leftChannel[n];
        const float dryR = rightChannel[n];

        // Predelay
        predelayL.push(dryL);
        predelayR.push(dryR);
        const float preL = predelayL.read();
        const float preR = predelayR.read();

        // Early reflections via allpass chains
        const float earlyOutL = earlyL.process(preL);
        const float earlyOutR = earlyR.process(preR);

        // Late reverb via FDN (fed from early reflections)
        const float lateOutL = fdnL.process(earlyOutL);
        const float lateOutR = fdnR.process(earlyOutR);

        // Mix early and late
        float wetL = earlyOutL * earlyGain + lateOutL * lateGain;
        float wetR = earlyOutR * earlyGain + lateOutR * lateGain;

        // Stereo width via M/S processing
        auto [mid, side] = libdsp::MidSide::encode(wetL, wetR);
        side *= width;
        auto [wideL, wideR] = libdsp::MidSide::decode(mid, side);
        wetL = wideL;
        wetR = wideR;

        // Dry/wet mix
        leftChannel[n]  = dryL * (1.0f - mixPct) + wetL * mixPct;
        rightChannel[n] = dryR * (1.0f - mixPct) + wetR * mixPct;
    }
}

void HGReverbProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HGReverbProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HGReverbProcessor();
}

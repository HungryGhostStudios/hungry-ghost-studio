#include <libdsp/dynamics/Compressor.h>

#include <cmath>
#include <algorithm>

namespace libdsp {
namespace dynamics {

void Compressor::prepare(double sampleRate, int blockSize)
{
    m_sampleRate = sampleRate;

    m_detectorL.prepare(sampleRate);
    m_detectorR.prepare(sampleRate);
    m_detectorL.setAttack(m_attackMs);
    m_detectorL.setRelease(m_releaseMs);
    m_detectorR.setAttack(m_attackMs);
    m_detectorR.setRelease(m_releaseMs);

    // Lookahead delay: up to 10ms
    int maxLookaheadSamples = static_cast<int>(0.010 * sampleRate) + 1;
    m_lookaheadL.prepare(sampleRate, maxLookaheadSamples);
    m_lookaheadR.prepare(sampleRate, maxLookaheadSamples);

    m_currentGR = 0.f;
    m_feedbackStateL = 0.f;
    m_feedbackStateR = 0.f;
    m_optoGR = 0.f;
}

void Compressor::process(float* leftChannel, float* rightChannel, int numSamples)
{
    const bool stereo = (rightChannel != nullptr);
    const float inGainLin = dbToLinear(m_inputGainDb);
    const float makeupLin = dbToLinear(m_makeupDb);

    for (int i = 0; i < numSamples; ++i)
    {
        // Input gain
        leftChannel[i] *= inGainLin;
        float inL = leftChannel[i];
        float inR = inL;
        if (stereo)
        {
            rightChannel[i] *= inGainLin;
            inR = rightChannel[i];
        }

        // Determine detection signal based on mode
        float detectL, detectR;
        if (m_mode == CompressorMode::FeedBack)
        {
            detectL = m_feedbackStateL + DENORMAL_DC;
            detectR = m_feedbackStateR + DENORMAL_DC;
        }
        else
        {
            detectL = inL;
            detectR = inR;
        }

        // Opto mode: program-dependent ballistics
        if (m_mode == CompressorMode::Opto)
        {
            m_detectorL.setAttack(1.f);
            m_detectorR.setAttack(1.f);

            float maxDetect = std::max(std::fabs(detectL), std::fabs(detectR));
            float detectDb = linearToDb(maxDetect);
            float releaseScale = std::clamp((detectDb + 60.f) / 60.f, 0.f, 1.f);
            float optoRelease = 50.f + releaseScale * 450.f;
            m_detectorL.setRelease(optoRelease);
            m_detectorR.setRelease(optoRelease);
        }
        else
        {
            m_detectorL.setAttack(m_attackMs);
            m_detectorL.setRelease(m_releaseMs);
            m_detectorR.setAttack(m_attackMs);
            m_detectorR.setRelease(m_releaseMs);
        }

        // Level detection
        float envL = m_detectorL.process(detectL);
        float envR = m_detectorR.process(detectR);

        // Stereo linking
        float env;
        if (m_stereoLink && stereo)
            env = (envL + envR) * 0.5f;
        else
            env = envL;

        // Convert to dB for gain computer
        float envDb = linearToDb(env);
        float grDb = m_gainComputer.process(envDb);

        // Store current gain reduction for metering
        m_currentGR = grDb;

        // Convert GR to linear
        float grLin = dbToLinear(grDb);

        // Lookahead: push dry signal, read delayed
        m_lookaheadL.push(inL);
        float delayedL, delayedR;
        if (m_lookaheadMs > 0.f)
        {
            float delaySamples = m_lookaheadMs * 0.001f * static_cast<float>(m_sampleRate);
            m_lookaheadL.setDelay(delaySamples);
            delayedL = m_lookaheadL.read();
        }
        else
        {
            delayedL = inL;
        }

        if (stereo)
        {
            m_lookaheadR.push(inR);
            if (m_lookaheadMs > 0.f)
            {
                float delaySamples = m_lookaheadMs * 0.001f * static_cast<float>(m_sampleRate);
                m_lookaheadR.setDelay(delaySamples);
                delayedR = m_lookaheadR.read();
            }
            else
            {
                delayedR = inR;
            }
        }
        else
        {
            delayedR = delayedL;
        }

        // Apply gain reduction + makeup
        float wetL = delayedL * grLin * makeupLin;
        float wetR = delayedR * grLin * makeupLin;

        // Store for feedback mode (with denormal protection)
        m_feedbackStateL = wetL + DENORMAL_DC;
        m_feedbackStateR = wetR + DENORMAL_DC;

        // Dry/wet mix
        leftChannel[i] = delayedL * (1.f - m_mix) + wetL * m_mix;
        if (stereo)
            rightChannel[i] = delayedR * (1.f - m_mix) + wetR * m_mix;
    }
}

float Compressor::processDetector(float sample, LevelDetector& detector)
{
    return detector.process(sample);
}

float Compressor::dbToLinear(float dB) const
{
    return std::pow(10.f, dB * 0.05f);
}

float Compressor::linearToDb(float linear) const
{
    return 20.f * std::log10(std::max(std::fabs(linear), 1e-10f));
}

} // namespace dynamics
} // namespace libdsp

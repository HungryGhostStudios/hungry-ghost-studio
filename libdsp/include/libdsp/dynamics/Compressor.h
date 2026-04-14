#pragma once

#include <libdsp/dynamics/LevelDetector.h>
#include <libdsp/dynamics/GainComputer.h>
#include <libdsp/reverb/DelayLine.h>

#include <cmath>
#include <algorithm>
#include <vector>

namespace libdsp {
namespace dynamics {

enum class CompressorMode { FeedForward, FeedBack, Opto };

class Compressor {
public:
    Compressor() = default;

    void prepare(double sampleRate, int blockSize);
    void process(float* leftChannel, float* rightChannel, int numSamples);

    void setMode(CompressorMode mode) { m_mode = mode; }
    void setThreshold(float dB) { m_gainComputer.setThreshold(dB); }
    void setRatio(float ratio) { m_gainComputer.setRatio(ratio); }
    void setKnee(float dB) { m_gainComputer.setKnee(dB); }
    void setAttack(float ms) { m_attackMs = ms; }
    void setRelease(float ms) { m_releaseMs = ms; }
    void setMakeup(float dB) { m_makeupDb = dB; }
    void setMix(float pct) { m_mix = std::clamp(pct, 0.f, 100.f) / 100.f; }
    void setStereoLink(bool linked) { m_stereoLink = linked; }
    void setLookahead(float ms) { m_lookaheadMs = ms; }
    void setInputGain(float dB) { m_inputGainDb = dB; }

    float getGainReduction() const { return m_currentGR; }

private:
    float processDetector(float sample, LevelDetector& detector);
    float dbToLinear(float dB) const;
    float linearToDb(float linear) const;

    CompressorMode m_mode = CompressorMode::FeedForward;
    GainComputer m_gainComputer;
    LevelDetector m_detectorL;
    LevelDetector m_detectorR;

    // Lookahead delay lines
    DelayLine m_lookaheadL;
    DelayLine m_lookaheadR;

    double m_sampleRate = 44100.0;
    float m_attackMs = 10.f;
    float m_releaseMs = 100.f;
    float m_makeupDb = 0.f;
    float m_inputGainDb = 0.f;
    float m_mix = 1.f;
    float m_lookaheadMs = 0.f;
    bool m_stereoLink = false;

    float m_currentGR = 0.f;

    // Feedback state (previous frame output level)
    float m_feedbackStateL = 0.f;
    float m_feedbackStateR = 0.f;

    // Opto state
    float m_optoGR = 0.f;

    // Denormal protection constant
    static constexpr float DENORMAL_DC = 1e-25f;
};

} // namespace dynamics
} // namespace libdsp

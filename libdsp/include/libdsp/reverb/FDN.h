#pragma once

#include <array>
#include <cmath>
#include <vector>
#include "DelayLine.h"

namespace libdsp {

/**
 * 8-line Feedback Delay Network with Hadamard mixing matrix.
 * Features: per-line damping, modulated delays, decay control, freeze mode.
 * No JUCE dependency.
 */
class FDN {
public:
    static constexpr int kNumLines = 8;

    FDN() = default;

    void prepare(double sampleRate, int blockSize);
    void setDecay(float seconds);
    void setDamping(float freqHz);
    void setDiffusion(float amount);
    void setModRate(float rateHz);
    void setModDepth(float depthMs);
    void setSize(float size);
    void setFreeze(bool freeze);

    /** Process one sample and return the mixed output. */
    float process(float input);

    void reset();

private:
    // Hadamard-like mixing: in-place on array of kNumLines values
    void hadamardMix(std::array<float, kNumLines>& data);

    // One-pole lowpass for damping
    struct DampingFilter {
        float state = 0.0f;
        float coeff = 0.5f;

        float process(float input) {
            state += coeff * (input - state);
            return state;
        }

        void reset() { state = 0.0f; }
    };

    double m_sampleRate = 44100.0;
    float m_decay = 2.0f;
    float m_damping = 5000.0f;
    float m_diffusion = 1.0f;
    float m_size = 1.0f;
    bool m_freeze = false;

    // LFO state for modulation
    float m_modRate = 0.5f;
    float m_modDepth = 0.0f;     // in samples
    float m_modPhase = 0.0f;
    float m_modPhaseInc = 0.0f;

    // Per-line state
    std::array<DelayLine, kNumLines> m_delayLines;
    std::array<DampingFilter, kNumLines> m_dampingFilters;
    std::array<float, kNumLines> m_feedbackGains{};

    // Base delay times in samples (mutually prime)
    std::array<int, kNumLines> m_baseDelays{};

    void updateFeedbackGains();
    void updateDampingCoeffs();
    void updateModPhaseInc();
};

} // namespace libdsp

#pragma once

#include <cmath>
#include <algorithm>

namespace libdsp {

/**
 * 4-pole (24dB/oct) ladder filter modeled after the classic Moog topology.
 * Uses the TPT (Topology-Preserving Transform) approach for zero-delay feedback.
 * Supports LP24 and HP24 modes with resonance up to self-oscillation.
 * No JUCE dependency.
 */
class LadderFilter {
public:
    enum class Type {
        LP24,
        HP24
    };

    LadderFilter() = default;

    void prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        reset();
    }

    void setCutoff(float frequencyHz) {
        m_cutoff = std::clamp(frequencyHz, 20.0f, static_cast<float>(m_sampleRate * 0.49));
    }

    /** Resonance 0..1 — values near 1.0 approach self-oscillation. */
    void setResonance(float resonance) {
        m_resonance = std::clamp(resonance, 0.0f, 1.0f);
    }

    /** Drive adds saturation in the feedback path. 1.0 = no drive, higher = more saturation. */
    void setDrive(float drive) {
        m_drive = std::max(drive, 0.0f);
    }

    void setType(Type type) {
        m_type = type;
    }

    float process(float input) {
        // Cutoff to angular frequency, then bilinear warp for TPT
        const float wc = 2.0f * static_cast<float>(M_PI) * m_cutoff / static_cast<float>(m_sampleRate);
        const float g = std::tan(wc * 0.5f); // TPT integrator gain

        // Feedback coefficient: resonance scaled to 4.0 for self-oscillation
        const float k = 4.0f * m_resonance;

        // Compute feedback value from 4th stage output (previous sample)
        float feedback = m_stage[3];

        // Apply drive saturation to feedback path
        if (m_drive > 0.0f) {
            feedback = std::tanh(feedback * m_drive) / std::max(m_drive, 0.001f);
        }

        // Input with resonance feedback
        float u = input - k * feedback;

        // Soft-clip the input to prevent instability at high resonance
        u = std::tanh(u);

        // 4-stage cascade of one-pole TPT lowpass filters
        // Each stage: y = g/(1+g) * (x - s) + s, where s is the state
        for (int i = 0; i < 4; ++i) {
            float v = g * (u - m_stage[i]) / (1.0f + g);
            float y = v + m_stage[i];
            m_stage[i] = y + v; // update state

            // Denormal protection
            if (std::fabs(m_stage[i]) < 1e-15f) m_stage[i] = 0.0f;

            u = y; // feed into next stage
        }

        switch (m_type) {
            case Type::LP24:
                return m_stage[3];
            case Type::HP24:
                // HP = input - LP (4th-order highpass from complementary output)
                return input - k * feedback - m_stage[3];
            default:
                return m_stage[3];
        }
    }

    void reset() {
        for (int i = 0; i < 4; ++i)
            m_stage[i] = 0.0f;
    }

private:
    double m_sampleRate = 44100.0;
    Type m_type = Type::LP24;
    float m_cutoff = 1000.0f;
    float m_resonance = 0.0f;
    float m_drive = 1.0f;

    // 4 one-pole filter states
    float m_stage[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace libdsp

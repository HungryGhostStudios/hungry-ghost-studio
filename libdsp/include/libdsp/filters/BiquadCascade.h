#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

namespace libdsp {

/**
 * Cascaded biquad filter (series chain of second-order sections).
 * Uses Direct Form II Transposed for numerical stability.
 * No JUCE dependency.
 */
class BiquadCascade {
public:
    using CoeffArray = std::array<float, 5>; // {b0, b1, b2, a1, a2}

    BiquadCascade() = default;

    void prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        reset();
    }

    void setNumStages(int numStages) {
        m_numStages = std::max(1, numStages);
        m_coeffs.resize(static_cast<size_t>(m_numStages));
        m_state.resize(static_cast<size_t>(m_numStages));
        reset();
    }

    /** Set coefficients for a specific stage. Array is {b0, b1, b2, a1, a2}. */
    void setCoefficients(int stage, const CoeffArray& coeffs) {
        if (stage >= 0 && stage < m_numStages)
            m_coeffs[static_cast<size_t>(stage)] = coeffs;
    }

    /** Process a single sample through all stages. */
    float process(float input) {
        float x = input;
        for (int i = 0; i < m_numStages; ++i) {
            x = processBiquad(i, x);
        }
        return x;
    }

    /** Process a buffer of samples in-place. */
    void processBatch(float* buffer, int numSamples) {
        for (int n = 0; n < numSamples; ++n) {
            buffer[n] = process(buffer[n]);
        }
    }

    void reset() {
        for (auto& s : m_state) {
            s.z1 = 0.0f;
            s.z2 = 0.0f;
        }
    }

    int getNumStages() const { return m_numStages; }
    double getSampleRate() const { return m_sampleRate; }

private:
    struct State {
        float z1 = 0.0f;
        float z2 = 0.0f;
    };

    /** Direct Form II Transposed biquad. */
    float processBiquad(int stage, float input) {
        auto& c = m_coeffs[static_cast<size_t>(stage)];
        auto& s = m_state[static_cast<size_t>(stage)];

        float output = c[0] * input + s.z1;
        s.z1 = c[1] * input - c[3] * output + s.z2;
        s.z2 = c[2] * input - c[4] * output;

        // Denormal protection
        if (std::fabs(s.z1) < 1e-15f) s.z1 = 0.0f;
        if (std::fabs(s.z2) < 1e-15f) s.z2 = 0.0f;

        return output;
    }

    double m_sampleRate = 44100.0;
    int m_numStages = 1;
    std::vector<CoeffArray> m_coeffs{1, {{1.0f, 0.0f, 0.0f, 0.0f, 0.0f}}};
    std::vector<State> m_state{1};
};

} // namespace libdsp

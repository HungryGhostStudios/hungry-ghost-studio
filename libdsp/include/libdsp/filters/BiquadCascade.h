#pragma once

#include "Coefficients.h"
#include <vector>
#include <array>
#include <cmath>

namespace libdsp {

/**
 * Series chain of biquad filter sections using Direct Form II Transposed.
 * Used by the EQ plugin for multi-band parametric filtering.
 */
class BiquadCascade {
public:
    BiquadCascade() = default;

    explicit BiquadCascade(int numStages)
        : m_coeffs(static_cast<size_t>(numStages)),
          m_states(static_cast<size_t>(numStages), {0.0f, 0.0f}) {}

    void setNumStages(int numStages) {
        m_coeffs.resize(static_cast<size_t>(numStages));
        m_states.resize(static_cast<size_t>(numStages), {0.0f, 0.0f});
    }

    int getNumStages() const {
        return static_cast<int>(m_coeffs.size());
    }

    void setCoeffs(int stage, const BiquadCoeffs& c) {
        if (stage >= 0 && stage < static_cast<int>(m_coeffs.size())) {
            m_coeffs[static_cast<size_t>(stage)] = c;
        }
    }

    float processSample(float x) {
        float out = x;
        for (size_t i = 0; i < m_coeffs.size(); ++i) {
            out = processBiquad(out, m_coeffs[i], m_states[i]);
        }
        return out;
    }

    void processBatch(float* buffer, int numSamples) {
        for (int n = 0; n < numSamples; ++n) {
            buffer[n] = processSample(buffer[n]);
        }
    }

    void reset() {
        for (auto& s : m_states) {
            s[0] = 0.0f;
            s[1] = 0.0f;
        }
    }

private:
    static float processBiquad(float x, const BiquadCoeffs& c, std::array<float, 2>& s) {
        // Direct Form II Transposed
        const float y = c.b0 * x + s[0];
        s[0] = c.b1 * x - c.a1 * y + s[1];
        s[1] = c.b2 * x - c.a2 * y;

        // Denormal protection
        if (std::fabs(s[0]) < 1e-15f) s[0] = 0.0f;
        if (std::fabs(s[1]) < 1e-15f) s[1] = 0.0f;

        return y;
    }

    std::vector<BiquadCoeffs> m_coeffs;
    std::vector<std::array<float, 2>> m_states;
};

} // namespace libdsp

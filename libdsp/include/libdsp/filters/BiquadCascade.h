#pragma once

#include <vector>
#include <cmath>
#include "Coefficients.h"

namespace libdsp {

/**
 * Cascade of biquad filter sections (Direct Form II Transposed).
 * Used for multi-band EQ — each stage applies one set of biquad coefficients.
 * No JUCE dependency.
 */
class BiquadCascade {
public:
    BiquadCascade() = default;

    void prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        reset();
    }

    /** Set the number of biquad stages in the cascade. */
    void setNumStages(int numStages) {
        if (numStages < 0) numStages = 0;
        m_stages.resize(static_cast<size_t>(numStages));
        reset();
    }

    /** Get the number of stages. */
    int getNumStages() const {
        return static_cast<int>(m_stages.size());
    }

    /** Set coefficients for a specific stage (0-indexed). */
    void setCoefficients(int stageIndex, const BiquadCoeffs& coeffs) {
        if (stageIndex >= 0 && stageIndex < static_cast<int>(m_stages.size())) {
            m_stages[static_cast<size_t>(stageIndex)].coeffs = coeffs;
        }
    }

    /** Process a single sample through all stages in series. */
    float process(float input) {
        float sample = input;
        for (auto& stage : m_stages) {
            sample = processStage(stage, sample);
        }
        return sample;
    }

    /** Process a buffer of samples in-place. */
    void processBatch(float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            buffer[i] = process(buffer[i]);
        }
    }

    void reset() {
        for (auto& stage : m_stages) {
            stage.z1 = 0.0f;
            stage.z2 = 0.0f;
        }
    }

    double getSampleRate() const { return m_sampleRate; }

private:
    struct Stage {
        BiquadCoeffs coeffs;
        float z1 = 0.0f;  // State variable 1
        float z2 = 0.0f;  // State variable 2
    };

    /** Direct Form II Transposed biquad. */
    static float processStage(Stage& s, float input) {
        const float output = s.coeffs.b0 * input + s.z1;
        s.z1 = s.coeffs.b1 * input - s.coeffs.a1 * output + s.z2;
        s.z2 = s.coeffs.b2 * input - s.coeffs.a2 * output;

        // Denormal protection
        if (std::fabs(s.z1) < 1e-15f) s.z1 = 0.0f;
        if (std::fabs(s.z2) < 1e-15f) s.z2 = 0.0f;

        return output;
    }

    double m_sampleRate = 44100.0;
    std::vector<Stage> m_stages;
};

} // namespace libdsp

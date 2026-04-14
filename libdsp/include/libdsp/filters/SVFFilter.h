#pragma once

#include <cmath>
#include <libdsp/util/Constants.h>

namespace libdsp {

/**
 * State Variable Filter using Topology-Preserving Transform (TPT / Zero-Delay Feedback).
 * Supports LP, HP, BP, Notch, AP, Peak, LowShelf, HighShelf types.
 * No JUCE dependency.
 */
class SVFFilter {
public:
    enum class Type {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        AllPass,
        Peak,
        LowShelf,
        HighShelf
    };

    SVFFilter() = default;

    void prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        reset();
        updateCoefficients();
    }

    void setType(Type type) {
        m_type = type;
        updateCoefficients();
    }

    void setFrequency(float frequencyHz) {
        m_frequency = frequencyHz;
        updateCoefficients();
    }

    void setQ(float q) {
        m_q = (q > 0.01f) ? q : 0.01f;
        updateCoefficients();
    }

    /** Set gain in dB (only used for Peak, LowShelf, HighShelf). */
    void setGain(float gainDb) {
        m_gainDb = gainDb;
        updateCoefficients();
    }

    /** Process a single sample. */
    float process(float input) {
        // TPT/ZDF SVF implementation
        // Based on Vadim Zavalishin's "The Art of VA Filter Design"
        const float v3 = input - m_ic2eq;
        const float v1 = m_a1 * m_ic1eq + m_a2 * v3;
        const float v2 = m_ic2eq + m_a2 * m_ic1eq + m_a3 * v3;

        m_ic1eq = 2.0f * v1 - m_ic1eq;
        m_ic2eq = 2.0f * v2 - m_ic2eq;

        // Denormal protection
        if (std::fabs(m_ic1eq) < 1e-15f) m_ic1eq = 0.0f;
        if (std::fabs(m_ic2eq) < 1e-15f) m_ic2eq = 0.0f;

        return m_m0 * input + m_m1 * v1 + m_m2 * v2;
    }

    void reset() {
        m_ic1eq = 0.0f;
        m_ic2eq = 0.0f;
    }

private:
    void updateCoefficients() {
        if (m_sampleRate <= 0.0) return;

        const float w = kPiF * m_frequency / static_cast<float>(m_sampleRate);
        const float g = std::tan(w);
        const float k = 1.0f / m_q;
        const float A = std::pow(10.0f, m_gainDb / 40.0f); // sqrt of linear gain

        // Core SVF coefficients
        m_a1 = 1.0f / (1.0f + g * (g + k));
        m_a2 = g * m_a1;
        m_a3 = g * m_a2;

        switch (m_type) {
            case Type::LowPass:
                m_m0 = 0.0f;
                m_m1 = 0.0f;
                m_m2 = 1.0f;
                break;
            case Type::HighPass:
                m_m0 = 1.0f;
                m_m1 = -k;
                m_m2 = -1.0f;
                break;
            case Type::BandPass:
                m_m0 = 0.0f;
                m_m1 = 1.0f;
                m_m2 = 0.0f;
                break;
            case Type::Notch:
                m_m0 = 1.0f;
                m_m1 = -k;
                m_m2 = 0.0f;
                break;
            case Type::AllPass:
                m_m0 = 1.0f;
                m_m1 = -2.0f * k;
                m_m2 = 0.0f;
                break;
            case Type::Peak: {
                // Peaking EQ: boost/cut around center frequency
                const float kAdj = 1.0f / (m_q * A);
                m_a1 = 1.0f / (1.0f + g * (g + kAdj));
                m_a2 = g * m_a1;
                m_a3 = g * m_a2;
                m_m0 = 1.0f;
                m_m1 = k * (A * A - 1.0f);
                m_m2 = 0.0f;
                break;
            }
            case Type::LowShelf: {
                const float gAdj = g / std::sqrt(A);
                const float kAdj = 1.0f / m_q;
                m_a1 = 1.0f / (1.0f + gAdj * (gAdj + kAdj));
                m_a2 = gAdj * m_a1;
                m_a3 = gAdj * m_a2;
                m_m0 = 1.0f;
                m_m1 = kAdj * (A - 1.0f);
                m_m2 = A * A - 1.0f;
                break;
            }
            case Type::HighShelf: {
                const float gAdj = g * std::sqrt(A);
                const float kAdj = 1.0f / m_q;
                m_a1 = 1.0f / (1.0f + gAdj * (gAdj + kAdj));
                m_a2 = gAdj * m_a1;
                m_a3 = gAdj * m_a2;
                m_m0 = A * A;
                m_m1 = kAdj * (1.0f - A) * A;
                m_m2 = 1.0f - A * A;
                break;
            }
        }
    }

    double m_sampleRate = 44100.0;
    Type m_type = Type::LowPass;
    float m_frequency = 1000.0f;
    float m_q = 0.707f;  // Butterworth default
    float m_gainDb = 0.0f;

    // SVF state
    float m_ic1eq = 0.0f;
    float m_ic2eq = 0.0f;

    // SVF coefficients
    float m_a1 = 0.0f;
    float m_a2 = 0.0f;
    float m_a3 = 0.0f;

    // Mix coefficients for output type selection
    float m_m0 = 0.0f;
    float m_m1 = 0.0f;
    float m_m2 = 1.0f;
};

} // namespace libdsp

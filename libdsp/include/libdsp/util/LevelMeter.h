#pragma once

#include <cmath>
#include <algorithm>

namespace libdsp {

/**
 * Peak/RMS level meter with configurable attack/release ballistics.
 * No JUCE dependency.
 */
class LevelMeter {
public:
    LevelMeter() = default;

    void prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        updateCoefficients();
        reset();
    }

    /** Set attack time in milliseconds. */
    void setAttack(float attackMs) {
        m_attackMs = attackMs;
        updateCoefficients();
    }

    /** Set release time in milliseconds. */
    void setRelease(float releaseMs) {
        m_releaseMs = releaseMs;
        updateCoefficients();
    }

    /** Push a single sample for level measurement. */
    void push(float sample) {
        const float absSample = std::fabs(sample);

        // Peak detector with ballistics
        if (absSample > m_peakLevel)
            m_peakLevel += m_attackCoeff * (absSample - m_peakLevel);
        else
            m_peakLevel += m_releaseCoeff * (absSample - m_peakLevel);

        // RMS accumulator (exponential moving average of squared signal)
        const float squared = sample * sample;
        if (squared > m_rmsSquared)
            m_rmsSquared += m_attackCoeff * (squared - m_rmsSquared);
        else
            m_rmsSquared += m_releaseCoeff * (squared - m_rmsSquared);

        // Denormal protection
        if (m_peakLevel < 1e-15f) m_peakLevel = 0.0f;
        if (m_rmsSquared < 1e-15f) m_rmsSquared = 0.0f;
    }

    /** Get current peak level (linear). */
    float getPeak() const { return m_peakLevel; }

    /** Get current peak level in dB. */
    float getPeakDb() const {
        return (m_peakLevel > 1e-10f) ? 20.0f * std::log10(m_peakLevel) : -120.0f;
    }

    /** Get current RMS level (linear). */
    float getRMS() const { return std::sqrt(m_rmsSquared); }

    /** Get current RMS level in dB. */
    float getRMSDb() const {
        const float rms = getRMS();
        return (rms > 1e-10f) ? 20.0f * std::log10(rms) : -120.0f;
    }

    /** Reset all levels to zero. */
    void reset() {
        m_peakLevel = 0.0f;
        m_rmsSquared = 0.0f;
    }

private:
    void updateCoefficients() {
        if (m_sampleRate <= 0.0) return;
        // One-pole filter coefficients from time constants
        m_attackCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(m_sampleRate) * m_attackMs * 0.001f));
        m_releaseCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(m_sampleRate) * m_releaseMs * 0.001f));
    }

    double m_sampleRate = 44100.0;
    float m_attackMs = 1.0f;      // Fast attack by default
    float m_releaseMs = 300.0f;   // Moderate release
    float m_attackCoeff = 0.0f;
    float m_releaseCoeff = 0.0f;
    float m_peakLevel = 0.0f;
    float m_rmsSquared = 0.0f;
};

} // namespace libdsp

#pragma once

#include <cmath>
#include <cstdlib>

namespace libdsp {

/**
 * Low-frequency oscillator with 5 waveform modes.
 * Pure C++17, no JUCE dependency. Phase accumulator with proper wrapping.
 */
class LFO {
public:
    enum class Waveform {
        Sine,
        Triangle,
        Saw,
        Square,
        SampleAndHold
    };

    LFO() = default;

    void prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        m_phase = 0.0;
        m_shValue = 0.0f;
        m_prevPhase = 0.0;
    }

    /** Set LFO frequency in Hz. */
    void setFrequency(float hz) {
        m_frequency = (hz < 0.0f) ? 0.0f : hz;
    }

    /** Set the waveform shape. */
    void setWaveform(Waveform wf) {
        m_waveform = wf;
    }

    /** Set phase offset in range [0, 1). */
    void setPhase(float phase) {
        m_phaseOffset = phase - std::floor(phase);
    }

    /** Process one sample and return value in [-1, 1]. */
    float process() {
        const double phaseInc = static_cast<double>(m_frequency) / m_sampleRate;
        const double currentPhase = m_phase + static_cast<double>(m_phaseOffset);
        const double wrappedPhase = currentPhase - std::floor(currentPhase);

        float output = 0.0f;

        switch (m_waveform) {
            case Waveform::Sine:
                output = static_cast<float>(std::sin(wrappedPhase * 2.0 * M_PI));
                break;

            case Waveform::Triangle:
                output = static_cast<float>(
                    (wrappedPhase < 0.5)
                        ? (4.0 * wrappedPhase - 1.0)
                        : (3.0 - 4.0 * wrappedPhase));
                break;

            case Waveform::Saw:
                output = static_cast<float>(2.0 * wrappedPhase - 1.0);
                break;

            case Waveform::Square:
                output = (wrappedPhase < 0.5) ? 1.0f : -1.0f;
                break;

            case Waveform::SampleAndHold: {
                // Detect phase wrap (new cycle)
                const double prevWrapped = m_prevPhase + static_cast<double>(m_phaseOffset);
                const double prevW = prevWrapped - std::floor(prevWrapped);
                if (wrappedPhase < prevW || m_firstSample) {
                    // Generate new random value in [-1, 1]
                    m_shValue = static_cast<float>(std::rand()) /
                                static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
                    m_firstSample = false;
                }
                output = m_shValue;
                break;
            }
        }

        m_prevPhase = m_phase;
        m_phase += phaseInc;
        // Wrap phase to avoid precision loss over time
        if (m_phase >= 1.0) {
            m_phase -= std::floor(m_phase);
        }

        return output;
    }

    /** Reset the LFO phase. */
    void reset() {
        m_phase = 0.0;
        m_prevPhase = 0.0;
        m_shValue = 0.0f;
        m_firstSample = true;
    }

private:
    double m_sampleRate = 44100.0;
    double m_phase = 0.0;
    double m_prevPhase = 0.0;
    float m_frequency = 1.0f;
    float m_phaseOffset = 0.0f;
    float m_shValue = 0.0f;
    bool m_firstSample = true;
    Waveform m_waveform = Waveform::Sine;
};

} // namespace libdsp

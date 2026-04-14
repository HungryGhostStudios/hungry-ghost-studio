#pragma once

#include <cmath>
#include <cstdint>
#include <random>
#include <libdsp/util/Constants.h>

namespace libdsp {

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
        m_shTriggered = false;
    }

    void setFrequency(float hz) {
        m_frequency = hz;
    }

    void setWaveform(Waveform wf) {
        m_waveform = wf;
    }

    /** Set phase offset in range [0, 1). Used for multi-voice chorus spread. */
    void setPhase(float phase) {
        m_phaseOffset = static_cast<double>(phase);
    }

    /** Generate next LFO sample. Returns value in range [-1, 1]. */
    float process() {
        double p = m_phase + m_phaseOffset;
        // Wrap to [0, 1)
        p -= std::floor(p);

        float output = 0.0f;

        switch (m_waveform) {
            case Waveform::Sine:
                output = static_cast<float>(std::sin(2.0 * kPi * p));
                break;

            case Waveform::Triangle:
                // Rising from -1 to 1 in first half, falling in second half
                output = static_cast<float>(p < 0.5
                    ? 4.0 * p - 1.0
                    : 3.0 - 4.0 * p);
                break;

            case Waveform::Saw:
                // Rising sawtooth: -1 at p=0, +1 at p=1
                output = static_cast<float>(2.0 * p - 1.0);
                break;

            case Waveform::Square:
                output = (p < 0.5) ? 1.0f : -1.0f;
                break;

            case Waveform::SampleAndHold:
                // Trigger new random value at phase wrap
                if (p < m_lastPhase) {
                    m_shTriggered = true;
                }
                if (m_shTriggered) {
                    m_shValue = m_distribution(m_rng);
                    m_shTriggered = false;
                }
                output = m_shValue;
                break;
        }

        m_lastPhase = p;

        // Advance phase
        m_phase += static_cast<double>(m_frequency) / m_sampleRate;
        // Keep phase in reasonable range to avoid floating-point drift
        if (m_phase >= 1.0) {
            m_phase -= std::floor(m_phase);
        }

        return output;
    }

    /** Reset LFO phase to zero. */
    void reset() {
        m_phase = 0.0;
        m_shValue = 0.0f;
        m_shTriggered = false;
        m_lastPhase = 0.0;
    }

private:
    double m_sampleRate = 44100.0;
    float m_frequency = 1.0f;
    Waveform m_waveform = Waveform::Sine;
    double m_phase = 0.0;
    double m_phaseOffset = 0.0;
    double m_lastPhase = 0.0;

    // Sample & Hold state
    float m_shValue = 0.0f;
    bool m_shTriggered = false;
    std::mt19937 m_rng{42};
    std::uniform_real_distribution<float> m_distribution{-1.0f, 1.0f};
};

} // namespace libdsp

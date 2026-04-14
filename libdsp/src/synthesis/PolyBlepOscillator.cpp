#include <libdsp/synthesis/PolyBlepOscillator.h>

namespace libdsp {

void PolyBlepOscillator::prepare(double sampleRate) {
    m_sampleRate = sampleRate;
    m_phase = 0.0;
    m_triState = 0.0f;
    m_phaseIncrement = static_cast<double>(m_frequency) / m_sampleRate;
}

void PolyBlepOscillator::setFrequency(float hz) {
    m_frequency = hz;
    m_phaseIncrement = static_cast<double>(hz) / m_sampleRate;
}

void PolyBlepOscillator::setWaveform(Waveform wf) {
    m_waveform = wf;
}

void PolyBlepOscillator::setPulseWidth(float pw) {
    // Clamp to [0.01, 0.99] to avoid degenerate cases
    m_pulseWidth = (pw < 0.01f) ? 0.01f : (pw > 0.99f) ? 0.99f : pw;
}

float PolyBlepOscillator::polyBlep(double t, double dt) const {
    // PolyBLEP correction: 2nd-order polynomial residual
    // t is phase position [0, 1), dt is phase increment
    if (t < dt) {
        // Just after discontinuity
        double x = t / dt;
        return static_cast<float>(x + x - x * x - 1.0);
    } else if (t > 1.0 - dt) {
        // Just before discontinuity
        double x = (t - 1.0) / dt;
        return static_cast<float>(x * x + x + x + 1.0);
    }
    return 0.0f;
}

float PolyBlepOscillator::process() {
    const double dt = m_phaseIncrement;
    float output = 0.0f;

    switch (m_waveform) {
        case Waveform::Saw: {
            // Naive saw: rises from -1 to 1 across one period
            output = static_cast<float>(2.0 * m_phase - 1.0);
            // Apply PolyBLEP at the discontinuity (phase wrap at 1.0)
            output -= polyBlep(m_phase, dt);
            break;
        }

        case Waveform::Square: {
            // Naive square
            double pw = static_cast<double>(m_pulseWidth);
            output = (m_phase < pw) ? 1.0f : -1.0f;

            // PolyBLEP at rising edge (phase = 0)
            output += polyBlep(m_phase, dt);
            // PolyBLEP at falling edge (phase = pulseWidth)
            double shiftedPhase = m_phase - pw;
            if (shiftedPhase < 0.0) shiftedPhase += 1.0;
            output -= polyBlep(shiftedPhase, dt);
            break;
        }

        case Waveform::Triangle: {
            // Generate square wave first (same as above)
            double pw = 0.5; // Triangle always uses 50% duty
            float sq = (m_phase < pw) ? 1.0f : -1.0f;
            sq += polyBlep(m_phase, dt);
            double shiftedPhase = m_phase - pw;
            if (shiftedPhase < 0.0) shiftedPhase += 1.0;
            sq -= polyBlep(shiftedPhase, dt);

            // Leaky integrator to convert square → triangle
            // Scale factor: 4 * frequency / sampleRate to normalize amplitude
            float integrationRate = static_cast<float>(4.0 * dt);
            m_triState += integrationRate * sq;

            // Soft leak to prevent DC drift
            m_triState *= 0.999f;

            output = m_triState;
            break;
        }
    }

    // Advance phase
    m_phase += dt;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
    }

    return output;
}

void PolyBlepOscillator::reset() {
    m_phase = 0.0;
    m_triState = 0.0f;
}

} // namespace libdsp

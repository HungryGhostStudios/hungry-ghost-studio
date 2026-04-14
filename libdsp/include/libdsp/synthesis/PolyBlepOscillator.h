#pragma once

#include <cmath>

namespace libdsp {

/**
 * Band-limited oscillator using PolyBLEP (polynomial band-limited step).
 * Supports Saw, Square (with pulse width), and Triangle waveforms.
 * Triangle is generated via leaky integrator of the square wave.
 * No JUCE dependency.
 */
class PolyBlepOscillator {
public:
    enum class Waveform {
        Saw,
        Square,
        Triangle
    };

    PolyBlepOscillator() = default;

    void prepare(double sampleRate);
    void setFrequency(float hz);
    void setWaveform(Waveform wf);
    void setPulseWidth(float pw);  // 0.0 to 1.0, default 0.5
    float process();
    void reset();

private:
    float polyBlep(double t, double dt) const;

    double m_sampleRate = 44100.0;
    double m_phase = 0.0;
    double m_phaseIncrement = 0.0;
    float m_frequency = 440.0f;
    float m_pulseWidth = 0.5f;
    Waveform m_waveform = Waveform::Saw;

    // Leaky integrator state for triangle
    float m_triState = 0.0f;
};

} // namespace libdsp

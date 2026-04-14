#include <libdsp/synthesis/PolyBlepOscillator.h>

namespace libdsp {
namespace synthesis {

void PolyBlepOscillator::prepare(double sampleRate)
{
    sampleRate_ = sampleRate;
    phase = 0.f;
    triIntegrator = 0.f;
    smoothFreq.prepare(sampleRate, 20.f);
    smoothFreq.reset(440.f);
    dt = static_cast<float>(440.0 / sampleRate_);
}

void PolyBlepOscillator::setFrequencyHz(float hz)
{
    smoothFreq.setTargetValue(hz);
}

void PolyBlepOscillator::setPulseWidth(float pw)
{
    pulseWidth = std::clamp(pw, 0.01f, 0.99f);
}

void PolyBlepOscillator::setWaveform(OscWaveform w)
{
    waveform = w;
}

float PolyBlepOscillator::polyBlep(float t) const
{
    // PolyBLEP correction: polynomial band-limited step function
    // t is phase relative to discontinuity, normalized by dt
    if (t < dt)
    {
        // t is in [0, dt) — just after the discontinuity
        float n = t / dt;
        return n + n - n * n - 1.f;
    }
    else if (t > 1.f - dt)
    {
        // t is in (1-dt, 1) — just before the discontinuity
        float n = (t - 1.f) / dt;
        return n * n + n + n + 1.f;
    }
    return 0.f;
}

float PolyBlepOscillator::processSample()
{
    // Update frequency smoothly
    float freq = smoothFreq.getNextValue();
    dt = static_cast<float>(freq / sampleRate_);
    dt = std::clamp(dt, 0.f, 0.5f); // Nyquist limit

    float sample = 0.f;

    switch (waveform)
    {
    case OscWaveform::Saw:
    {
        // Naive saw: 2*phase - 1 (range -1 to +1)
        sample = 2.f * phase - 1.f;
        // Apply PolyBLEP at the discontinuity (phase wraps at 1.0)
        sample -= polyBlep(phase);
        break;
    }
    case OscWaveform::Square:
    {
        // Naive square wave
        sample = (phase < pulseWidth) ? 1.f : -1.f;
        // Apply PolyBLEP at both transitions
        sample += polyBlep(phase);                              // rising edge at 0
        sample -= polyBlep(std::fmod(phase - pulseWidth + 1.f, 1.f)); // falling edge at pulseWidth
        break;
    }
    case OscWaveform::Triangle:
    {
        // Generate triangle via leaky integrator of the square wave
        float sq = (phase < pulseWidth) ? 1.f : -1.f;
        sq += polyBlep(phase);
        sq -= polyBlep(std::fmod(phase - pulseWidth + 1.f, 1.f));

        // Leaky integrator: integrates square to get triangle
        // Scale factor: 4*dt normalizes the triangle amplitude
        triIntegrator += 4.f * dt * sq;
        // Leaky coefficient to prevent DC drift
        triIntegrator *= 0.999f;
        sample = triIntegrator;
        break;
    }
    case OscWaveform::Sine:
    {
        constexpr float twoPi = 6.28318530718f;
        sample = std::sin(twoPi * phase);
        break;
    }
    }

    // Advance phase
    phase += dt;
    // Wrap phase to [0, 1)
    while (phase >= 1.f)
        phase -= 1.f;

    // Denormal protection
    if (std::fabs(sample) < 1e-15f)
        sample = 0.f;

    return sample;
}

} // namespace synthesis
} // namespace libdsp

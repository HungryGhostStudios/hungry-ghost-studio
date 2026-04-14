#pragma once

#include <cmath>
#include <algorithm>
#include <libdsp/util/SmoothParameter.h>

namespace libdsp {
namespace synthesis {

enum class OscWaveform { Saw, Square, Triangle, Sine };

class PolyBlepOscillator {
public:
    void prepare(double sampleRate);
    void setFrequencyHz(float hz);
    void setPulseWidth(float pw);   // 0.0..1.0 for square
    void setWaveform(OscWaveform w);
    float processSample();

private:
    float polyBlep(float t) const;

    float phase{};
    float dt{};
    double sampleRate_{44100.0};
    OscWaveform waveform{OscWaveform::Saw};
    float pulseWidth{0.5f};
    float triIntegrator{};
    SmoothParameter<float> smoothFreq{440.f};
};

} // namespace synthesis
} // namespace libdsp

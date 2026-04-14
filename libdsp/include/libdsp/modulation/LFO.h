#pragma once

#include <cmath>
#include <random>

namespace libdsp {

enum class LFOWaveform {
    Sine,
    Triangle,
    Saw,
    Square,
    SampleAndHold
};

class LFO {
public:
    LFO() = default;

    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        phase_ = 0.0;
        shValue_ = 0.0f;
        shTriggered_ = false;
    }

    void setFrequency(float hz) {
        frequency_ = hz;
    }

    void setWaveform(LFOWaveform waveform) {
        waveform_ = waveform;
    }

    /// Set phase offset in range [0, 1] — useful for multi-voice chorus spread
    void setPhase(float phaseOffset) {
        phaseOffset_ = phaseOffset;
    }

    /// Returns value in range [-1, 1]
    float process() {
        double phaseInc = frequency_ / sampleRate_;
        double currentPhase = phase_ + static_cast<double>(phaseOffset_);

        // Wrap to [0, 1)
        currentPhase -= std::floor(currentPhase);

        float output = 0.0f;

        switch (waveform_) {
            case LFOWaveform::Sine:
                output = static_cast<float>(std::sin(2.0 * M_PI * currentPhase));
                break;

            case LFOWaveform::Triangle:
                // Triangle: rises 0→1 in first half, falls 1→-1 in second half
                if (currentPhase < 0.5)
                    output = static_cast<float>(4.0 * currentPhase - 1.0);
                else
                    output = static_cast<float>(3.0 - 4.0 * currentPhase);
                break;

            case LFOWaveform::Saw:
                // Saw: rises from -1 to 1 over the period
                output = static_cast<float>(2.0 * currentPhase - 1.0);
                break;

            case LFOWaveform::Square:
                output = (currentPhase < 0.5) ? 1.0f : -1.0f;
                break;

            case LFOWaveform::SampleAndHold:
                // Trigger new random value at phase wrap
                if (currentPhase < phaseInc && !shTriggered_) {
                    shValue_ = dist_(rng_);
                    shTriggered_ = true;
                } else if (currentPhase >= phaseInc) {
                    shTriggered_ = false;
                }
                output = shValue_;
                break;
        }

        // Advance phase
        phase_ += phaseInc;
        if (phase_ >= 1.0)
            phase_ -= 1.0;

        return output;
    }

    void reset() {
        phase_ = 0.0;
        shValue_ = 0.0f;
        shTriggered_ = false;
    }

private:
    double sampleRate_ = 44100.0;
    double phase_ = 0.0;
    float frequency_ = 1.0f;
    float phaseOffset_ = 0.0f;
    LFOWaveform waveform_ = LFOWaveform::Sine;

    // Sample & Hold state
    float shValue_ = 0.0f;
    bool shTriggered_ = false;
    std::mt19937 rng_{42};
    std::uniform_real_distribution<float> dist_{-1.0f, 1.0f};
};

} // namespace libdsp

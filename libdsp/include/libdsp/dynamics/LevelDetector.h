#pragma once

#include <cmath>
#include <algorithm>

namespace libdsp {
namespace dynamics {

enum class DetectorMode { Peak, RMS };

class LevelDetector {
public:
    void prepare(double sampleRate)
    {
        fs = sampleRate;
        state = 0.f;
        rmsState = 0.f;
        updateCoeffs();
    }

    void setAttack(float ms)
    {
        attackMs = std::max(ms, 0.001f);
        updateCoeffs();
    }

    void setRelease(float ms)
    {
        releaseMs = std::max(ms, 0.001f);
        updateCoeffs();
    }

    void setMode(DetectorMode m)
    {
        mode = m;
    }

    float process(float sample)
    {
        float input = 0.f;

        if (mode == DetectorMode::RMS)
        {
            rmsState += rmsCoeff * (sample * sample - rmsState);
            input = std::sqrt(std::max(rmsState, 0.f));
        }
        else // Peak
        {
            input = std::fabs(sample);
        }

        // Ballistics: attack when input > state, release when input < state
        float coeff = (input > state) ? attackCoeff : releaseCoeff;
        state += coeff * (input - state);

        return state;
    }

private:
    void updateCoeffs()
    {
        if (fs <= 0.0)
            return;
        // One-pole coefficient from time constant: coeff = 1 - exp(-1 / (time_s * fs))
        attackCoeff = 1.f - std::exp(-1.f / (float)(attackMs * 0.001f * fs));
        releaseCoeff = 1.f - std::exp(-1.f / (float)(releaseMs * 0.001f * fs));
        // RMS smoothing at ~5ms equivalent
        rmsCoeff = 1.f - std::exp(-1.f / (float)(5.f * 0.001f * fs));
    }

    double fs = 44100.0;
    float attackMs = 10.f;
    float releaseMs = 100.f;
    DetectorMode mode = DetectorMode::RMS;

    float attackCoeff = 0.f;
    float releaseCoeff = 0.f;
    float rmsCoeff = 0.f;
    float state = 0.f;
    float rmsState = 0.f;
};

} // namespace dynamics
} // namespace libdsp

#pragma once

#include <cmath>
#include <algorithm>

namespace libdsp {

class LevelMeter {
public:
    void prepare(double sampleRate, float attackMs = 5.f, float releaseMs = 100.f)
    {
        fs = sampleRate;
        setAttack(attackMs);
        setRelease(releaseMs);
        reset();
    }

    void setAttack(float ms)
    {
        if (fs > 0.0)
            attackCoeff = 1.f - std::exp(-1.f / (float)(ms * 0.001f * fs));
    }

    void setRelease(float ms)
    {
        if (fs > 0.0)
            releaseCoeff = 1.f - std::exp(-1.f / (float)(ms * 0.001f * fs));
    }

    void push(float sample)
    {
        float absSample = std::fabs(sample);

        // Peak with ballistics
        float peakCoeff = (absSample > peakLevel) ? attackCoeff : releaseCoeff;
        peakLevel += peakCoeff * (absSample - peakLevel);

        // RMS accumulation
        rmsSum += absSample * absSample;
        ++rmsCount;
    }

    float getPeak() const { return peakLevel; }

    float getRMS() const
    {
        if (rmsCount == 0) return 0.f;
        return std::sqrt(rmsSum / static_cast<float>(rmsCount));
    }

    void reset()
    {
        peakLevel = 0.f;
        rmsSum = 0.f;
        rmsCount = 0;
    }

private:
    double fs = 44100.0;
    float attackCoeff = 0.f;
    float releaseCoeff = 0.f;
    float peakLevel = 0.f;
    float rmsSum = 0.f;
    int rmsCount = 0;
};

} // namespace libdsp

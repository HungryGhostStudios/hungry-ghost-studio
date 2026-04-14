#pragma once

#include <cmath>
#include <algorithm>

namespace libdsp {
namespace dynamics {

class GainComputer {
public:
    void setThreshold(float dB) { thresholdDb = dB; }
    void setRatio(float r) { ratio = std::max(r, 1.f); }
    void setKnee(float dB) { kneeDb = std::max(dB, 0.f); }

    // Returns gain reduction in dB (always <= 0)
    float process(float levelDb) const
    {
        float halfKnee = kneeDb * 0.5f;
        float overDb = levelDb - thresholdDb;

        float gainReduction = 0.f;

        if (kneeDb <= 0.f || overDb >= halfKnee)
        {
            // Hard knee or above knee region
            if (overDb > 0.f)
                gainReduction = overDb * (1.f / ratio - 1.f);
        }
        else if (overDb > -halfKnee)
        {
            // Soft knee region: quadratic interpolation
            float x = overDb + halfKnee;
            gainReduction = (1.f / ratio - 1.f) * x * x / (2.f * kneeDb);
        }
        // else: below knee — no gain reduction

        return std::min(gainReduction, 0.f);
    }

private:
    float thresholdDb = -20.f;
    float ratio = 4.f;
    float kneeDb = 6.f;
};

} // namespace dynamics
} // namespace libdsp

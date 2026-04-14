#pragma once

#include <cmath>
#include <array>

namespace libdsp {

/**
 * EQ coefficient calculator using the Audio EQ Cookbook formulas (Robert Bristow-Johnson).
 * Returns {b0, b1, b2, a1, a2} normalized arrays (a0 = 1).
 * No JUCE dependency.
 */
struct EQCoeffs {
    using CoeffArray = std::array<float, 5>; // {b0, b1, b2, a1, a2}
    static constexpr float pi = 3.14159265358979323846f;

    static CoeffArray makePeaking(float freq, float gainDb, float Q, float sampleRate) {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * pi * freq / sampleRate;
        const float sinw = std::sin(w0);
        const float cosw = std::cos(w0);
        const float alpha = sinw / (2.0f * Q);

        const float a0 = 1.0f + alpha / A;
        return {{
            (1.0f + alpha * A) / a0,
            (-2.0f * cosw) / a0,
            (1.0f - alpha * A) / a0,
            (-2.0f * cosw) / a0,
            (1.0f - alpha / A) / a0
        }};
    }

    static CoeffArray makeLowShelf(float freq, float gainDb, float Q, float sampleRate) {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * pi * freq / sampleRate;
        const float sinw = std::sin(w0);
        const float cosw = std::cos(w0);
        const float alpha = sinw / (2.0f * Q);
        const float sqrtA2alpha = 2.0f * std::sqrt(A) * alpha;

        const float a0 = (A + 1.0f) + (A - 1.0f) * cosw + sqrtA2alpha;
        return {{
            (A * ((A + 1.0f) - (A - 1.0f) * cosw + sqrtA2alpha)) / a0,
            (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw)) / a0,
            (A * ((A + 1.0f) - (A - 1.0f) * cosw - sqrtA2alpha)) / a0,
            (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosw)) / a0,
            ((A + 1.0f) + (A - 1.0f) * cosw - sqrtA2alpha) / a0
        }};
    }

    static CoeffArray makeHighShelf(float freq, float gainDb, float Q, float sampleRate) {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * pi * freq / sampleRate;
        const float sinw = std::sin(w0);
        const float cosw = std::cos(w0);
        const float alpha = sinw / (2.0f * Q);
        const float sqrtA2alpha = 2.0f * std::sqrt(A) * alpha;

        const float a0 = (A + 1.0f) - (A - 1.0f) * cosw + sqrtA2alpha;
        return {{
            (A * ((A + 1.0f) + (A - 1.0f) * cosw + sqrtA2alpha)) / a0,
            (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw)) / a0,
            (A * ((A + 1.0f) + (A - 1.0f) * cosw - sqrtA2alpha)) / a0,
            (2.0f * ((A - 1.0f) - (A + 1.0f) * cosw)) / a0,
            ((A + 1.0f) - (A - 1.0f) * cosw - sqrtA2alpha) / a0
        }};
    }

    static CoeffArray makeLP(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * pi * freq / sampleRate;
        const float sinw = std::sin(w0);
        const float cosw = std::cos(w0);
        const float alpha = sinw / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        const float b0 = (1.0f - cosw) / 2.0f;
        return {{
            b0 / a0,
            (1.0f - cosw) / a0,
            b0 / a0,
            (-2.0f * cosw) / a0,
            (1.0f - alpha) / a0
        }};
    }

    static CoeffArray makeHP(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * pi * freq / sampleRate;
        const float sinw = std::sin(w0);
        const float cosw = std::cos(w0);
        const float alpha = sinw / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        const float b0 = (1.0f + cosw) / 2.0f;
        return {{
            b0 / a0,
            -(1.0f + cosw) / a0,
            b0 / a0,
            (-2.0f * cosw) / a0,
            (1.0f - alpha) / a0
        }};
    }

    static CoeffArray makeNotch(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * pi * freq / sampleRate;
        const float sinw = std::sin(w0);
        const float cosw = std::cos(w0);
        const float alpha = sinw / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        return {{
            1.0f / a0,
            (-2.0f * cosw) / a0,
            1.0f / a0,
            (-2.0f * cosw) / a0,
            (1.0f - alpha) / a0
        }};
    }

    static CoeffArray makeAllpass(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * pi * freq / sampleRate;
        const float sinw = std::sin(w0);
        const float cosw = std::cos(w0);
        const float alpha = sinw / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        return {{
            (1.0f - alpha) / a0,
            (-2.0f * cosw) / a0,
            (1.0f + alpha) / a0,
            (-2.0f * cosw) / a0,
            (1.0f - alpha) / a0
        }};
    }
};

} // namespace libdsp

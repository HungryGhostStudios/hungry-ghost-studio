#pragma once

#include <array>
#include <cmath>

namespace libdsp {

/**
 * Biquad filter coefficients {b0, b1, b2, a1, a2}.
 * Normalized so a0 = 1 (not stored).
 * Uses Direct Form II Transposed convention.
 */
struct BiquadCoeffs {
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
};

/**
 * Static coefficient calculators for standard EQ filter types.
 * Based on Robert Bristow-Johnson's Audio EQ Cookbook.
 */
class EQCoeffs {
public:
    static BiquadCoeffs makePeaking(float freq, float gainDb, float Q, float sampleRate) {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * static_cast<float>(M_PI) * freq / sampleRate;
        const float sinW0 = std::sin(w0);
        const float cosW0 = std::cos(w0);
        const float alpha = sinW0 / (2.0f * Q);

        const float a0 = 1.0f + alpha / A;
        return {
            (1.0f + alpha * A) / a0,
            (-2.0f * cosW0) / a0,
            (1.0f - alpha * A) / a0,
            (-2.0f * cosW0) / a0,
            (1.0f - alpha / A) / a0
        };
    }

    static BiquadCoeffs makeLowShelf(float freq, float gainDb, float Q, float sampleRate) {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * static_cast<float>(M_PI) * freq / sampleRate;
        const float sinW0 = std::sin(w0);
        const float cosW0 = std::cos(w0);
        const float alpha = sinW0 / (2.0f * Q);
        const float twoSqrtAAlpha = 2.0f * std::sqrt(A) * alpha;

        const float a0 = (A + 1.0f) + (A - 1.0f) * cosW0 + twoSqrtAAlpha;
        return {
            (A * ((A + 1.0f) - (A - 1.0f) * cosW0 + twoSqrtAAlpha)) / a0,
            (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW0)) / a0,
            (A * ((A + 1.0f) - (A - 1.0f) * cosW0 - twoSqrtAAlpha)) / a0,
            (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosW0)) / a0,
            ((A + 1.0f) + (A - 1.0f) * cosW0 - twoSqrtAAlpha) / a0
        };
    }

    static BiquadCoeffs makeHighShelf(float freq, float gainDb, float Q, float sampleRate) {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * static_cast<float>(M_PI) * freq / sampleRate;
        const float sinW0 = std::sin(w0);
        const float cosW0 = std::cos(w0);
        const float alpha = sinW0 / (2.0f * Q);
        const float twoSqrtAAlpha = 2.0f * std::sqrt(A) * alpha;

        const float a0 = (A + 1.0f) - (A - 1.0f) * cosW0 + twoSqrtAAlpha;
        return {
            (A * ((A + 1.0f) + (A - 1.0f) * cosW0 + twoSqrtAAlpha)) / a0,
            (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW0)) / a0,
            (A * ((A + 1.0f) + (A - 1.0f) * cosW0 - twoSqrtAAlpha)) / a0,
            (2.0f * ((A - 1.0f) - (A + 1.0f) * cosW0)) / a0,
            ((A + 1.0f) - (A - 1.0f) * cosW0 - twoSqrtAAlpha) / a0
        };
    }

    static BiquadCoeffs makeLP(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * static_cast<float>(M_PI) * freq / sampleRate;
        const float sinW0 = std::sin(w0);
        const float cosW0 = std::cos(w0);
        const float alpha = sinW0 / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        return {
            ((1.0f - cosW0) / 2.0f) / a0,
            (1.0f - cosW0) / a0,
            ((1.0f - cosW0) / 2.0f) / a0,
            (-2.0f * cosW0) / a0,
            (1.0f - alpha) / a0
        };
    }

    static BiquadCoeffs makeHP(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * static_cast<float>(M_PI) * freq / sampleRate;
        const float sinW0 = std::sin(w0);
        const float cosW0 = std::cos(w0);
        const float alpha = sinW0 / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        return {
            ((1.0f + cosW0) / 2.0f) / a0,
            (-(1.0f + cosW0)) / a0,
            ((1.0f + cosW0) / 2.0f) / a0,
            (-2.0f * cosW0) / a0,
            (1.0f - alpha) / a0
        };
    }

    static BiquadCoeffs makeNotch(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * static_cast<float>(M_PI) * freq / sampleRate;
        const float sinW0 = std::sin(w0);
        const float cosW0 = std::cos(w0);
        const float alpha = sinW0 / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        return {
            1.0f / a0,
            (-2.0f * cosW0) / a0,
            1.0f / a0,
            (-2.0f * cosW0) / a0,
            (1.0f - alpha) / a0
        };
    }

    static BiquadCoeffs makeAllpass(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * static_cast<float>(M_PI) * freq / sampleRate;
        const float sinW0 = std::sin(w0);
        const float cosW0 = std::cos(w0);
        const float alpha = sinW0 / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        return {
            (1.0f - alpha) / a0,
            (-2.0f * cosW0) / a0,
            (1.0f + alpha) / a0,
            (-2.0f * cosW0) / a0,
            (1.0f - alpha) / a0
        };
    }

    static BiquadCoeffs makeBandpass(float freq, float Q, float sampleRate) {
        const float w0 = 2.0f * static_cast<float>(M_PI) * freq / sampleRate;
        const float sinW0 = std::sin(w0);
        const float cosW0 = std::cos(w0);
        const float alpha = sinW0 / (2.0f * Q);

        const float a0 = 1.0f + alpha;
        return {
            alpha / a0,
            0.0f,
            -alpha / a0,
            (-2.0f * cosW0) / a0,
            (1.0f - alpha) / a0
        };
    }
};

} // namespace libdsp

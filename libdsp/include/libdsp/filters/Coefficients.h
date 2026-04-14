#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace libdsp {

struct BiquadCoeffs {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
};

/**
 * Coefficient factory using RBJ Audio EQ Cookbook formulas.
 * All coefficients are pre-normalized (a0 = 1).
 */
namespace EQCoeffs {

inline BiquadCoeffs peakingEQ(double fc, double fs, double gainDB, double Q) {
    const double A = std::pow(10.0, gainDB / 40.0);
    const double w0 = 2.0 * M_PI * fc / fs;
    const double sinW0 = std::sin(w0);
    const double cosW0 = std::cos(w0);
    const double alpha = sinW0 / (2.0 * Q);

    const double b0 = 1.0 + alpha * A;
    const double b1 = -2.0 * cosW0;
    const double b2 = 1.0 - alpha * A;
    const double a0 = 1.0 + alpha / A;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha / A;

    const double inv = 1.0 / a0;
    return { static_cast<float>(b0 * inv), static_cast<float>(b1 * inv),
             static_cast<float>(b2 * inv), static_cast<float>(a1 * inv),
             static_cast<float>(a2 * inv) };
}

inline BiquadCoeffs lowShelf(double fc, double fs, double gainDB, double S = 1.0) {
    const double A = std::pow(10.0, gainDB / 40.0);
    const double w0 = 2.0 * M_PI * fc / fs;
    const double sinW0 = std::sin(w0);
    const double cosW0 = std::cos(w0);
    const double alpha = sinW0 / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
    const double sqrtA2alpha = 2.0 * std::sqrt(A) * alpha;

    const double b0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + sqrtA2alpha);
    const double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
    const double b2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - sqrtA2alpha);
    const double a0 = (A + 1.0) + (A - 1.0) * cosW0 + sqrtA2alpha;
    const double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
    const double a2 = (A + 1.0) + (A - 1.0) * cosW0 - sqrtA2alpha;

    const double inv = 1.0 / a0;
    return { static_cast<float>(b0 * inv), static_cast<float>(b1 * inv),
             static_cast<float>(b2 * inv), static_cast<float>(a1 * inv),
             static_cast<float>(a2 * inv) };
}

inline BiquadCoeffs highShelf(double fc, double fs, double gainDB, double S = 1.0) {
    const double A = std::pow(10.0, gainDB / 40.0);
    const double w0 = 2.0 * M_PI * fc / fs;
    const double sinW0 = std::sin(w0);
    const double cosW0 = std::cos(w0);
    const double alpha = sinW0 / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
    const double sqrtA2alpha = 2.0 * std::sqrt(A) * alpha;

    const double b0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + sqrtA2alpha);
    const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
    const double b2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - sqrtA2alpha);
    const double a0 = (A + 1.0) - (A - 1.0) * cosW0 + sqrtA2alpha;
    const double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
    const double a2 = (A + 1.0) - (A - 1.0) * cosW0 - sqrtA2alpha;

    const double inv = 1.0 / a0;
    return { static_cast<float>(b0 * inv), static_cast<float>(b1 * inv),
             static_cast<float>(b2 * inv), static_cast<float>(a1 * inv),
             static_cast<float>(a2 * inv) };
}

inline BiquadCoeffs lowPass(double fc, double fs, double Q) {
    const double w0 = 2.0 * M_PI * fc / fs;
    const double sinW0 = std::sin(w0);
    const double cosW0 = std::cos(w0);
    const double alpha = sinW0 / (2.0 * Q);

    const double b0 = (1.0 - cosW0) / 2.0;
    const double b1 = 1.0 - cosW0;
    const double b2 = (1.0 - cosW0) / 2.0;
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha;

    const double inv = 1.0 / a0;
    return { static_cast<float>(b0 * inv), static_cast<float>(b1 * inv),
             static_cast<float>(b2 * inv), static_cast<float>(a1 * inv),
             static_cast<float>(a2 * inv) };
}

inline BiquadCoeffs highPass(double fc, double fs, double Q) {
    const double w0 = 2.0 * M_PI * fc / fs;
    const double sinW0 = std::sin(w0);
    const double cosW0 = std::cos(w0);
    const double alpha = sinW0 / (2.0 * Q);

    const double b0 = (1.0 + cosW0) / 2.0;
    const double b1 = -(1.0 + cosW0);
    const double b2 = (1.0 + cosW0) / 2.0;
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha;

    const double inv = 1.0 / a0;
    return { static_cast<float>(b0 * inv), static_cast<float>(b1 * inv),
             static_cast<float>(b2 * inv), static_cast<float>(a1 * inv),
             static_cast<float>(a2 * inv) };
}

inline BiquadCoeffs notch(double fc, double fs, double Q) {
    const double w0 = 2.0 * M_PI * fc / fs;
    const double sinW0 = std::sin(w0);
    const double cosW0 = std::cos(w0);
    const double alpha = sinW0 / (2.0 * Q);

    const double b0 = 1.0;
    const double b1 = -2.0 * cosW0;
    const double b2 = 1.0;
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha;

    const double inv = 1.0 / a0;
    return { static_cast<float>(b0 * inv), static_cast<float>(b1 * inv),
             static_cast<float>(b2 * inv), static_cast<float>(a1 * inv),
             static_cast<float>(a2 * inv) };
}

inline BiquadCoeffs allPass(double fc, double fs, double Q) {
    const double w0 = 2.0 * M_PI * fc / fs;
    const double sinW0 = std::sin(w0);
    const double cosW0 = std::cos(w0);
    const double alpha = sinW0 / (2.0 * Q);

    const double b0 = 1.0 - alpha;
    const double b1 = -2.0 * cosW0;
    const double b2 = 1.0 + alpha;
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha;

    const double inv = 1.0 / a0;
    return { static_cast<float>(b0 * inv), static_cast<float>(b1 * inv),
             static_cast<float>(b2 * inv), static_cast<float>(a1 * inv),
             static_cast<float>(a2 * inv) };
}

} // namespace EQCoeffs
} // namespace libdsp

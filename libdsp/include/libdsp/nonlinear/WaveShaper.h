#pragma once

#include "ADAA.h"
#include <cmath>
#include <algorithm>

namespace libdsp {

/// Waveshaping modes supported by WaveShaper.
enum class WaveShaperMode {
    Soft,   // tanh soft clip
    Hard,   // hard clip
    Tape,   // asymmetric tape saturation
    Fold,   // sine-based wavefolder
    Asym    // asymmetric positive/negative curves
};

/// Multi-mode waveshaper with first-order ADAA anti-aliasing and DC blocking.
///
/// Modes:
///   Soft  — tanh(drive * x)
///   Hard  — clamp to [-1, 1]
///   Tape  — asymmetric soft saturation (different + / − curves)
///   Fold  — sin-based wavefolder
///   Asym  — different positive/negative shaping curves
class WaveShaper {
public:
    WaveShaper() { reset(); }

    void prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        // DC blocker coefficient: HPF at ~5 Hz
        // y[n] = x[n] - x[n-1] + R * y[n-1], R = 1 - (2*pi*fc/fs)
        m_dcBlockR = 1.0f - (2.0f * kPi * 5.0f / static_cast<float>(sampleRate));
        reset();
    }

    void reset() {
        m_adaaSoft.reset();
        m_adaaHard.reset();
        m_adaaTape.reset();
        m_adaaFold.reset();
        m_adaaAsym.reset();
        m_dcX1 = 0.0f;
        m_dcY1 = 0.0f;
    }

    void setMode(WaveShaperMode mode) { m_mode = mode; }
    void setDrive(float drive) { m_drive = drive; }
    void setBias(float bias) { m_bias = bias; }
    void setDCBlock(bool enabled) { m_dcBlockEnabled = enabled; }

    float process(float sample) {
        float x = sample * m_drive + m_bias;
        float out;

        switch (m_mode) {
            case WaveShaperMode::Soft:
                out = m_adaaSoft.process(x);
                break;
            case WaveShaperMode::Hard:
                out = m_adaaHard.process(x);
                break;
            case WaveShaperMode::Tape:
                out = m_adaaTape.process(x);
                break;
            case WaveShaperMode::Fold:
                out = m_adaaFold.process(x);
                break;
            case WaveShaperMode::Asym:
                out = m_adaaAsym.process(x);
                break;
            default:
                out = x;
                break;
        }

        // DC blocker (first-order high-pass at ~5 Hz)
        if (m_dcBlockEnabled) {
            float dcOut = out - m_dcX1 + m_dcBlockR * m_dcY1;
            m_dcX1 = out;
            m_dcY1 = dcOut;
            out = dcOut;
        }

        return out;
    }

private:
    static constexpr float kPi = 3.14159265358979323846f;

    // --- Shaping functions and their antiderivatives ---

    // Soft clip: tanh(x), antiderivative: log(cosh(x))
    struct SoftFunc {
        float operator()(float x) const { return std::tanh(x); }
    };
    struct SoftAnti {
        float operator()(float x) const { return std::log(std::cosh(x)); }
    };

    // Hard clip: clamp(x, -1, 1), antiderivative: piecewise
    struct HardFunc {
        float operator()(float x) const { return std::clamp(x, -1.0f, 1.0f); }
    };
    struct HardAnti {
        float operator()(float x) const {
            if (x <= -1.0f) return -x - 0.5f;
            if (x >= 1.0f)  return  x - 0.5f;
            return 0.5f * x * x;
        }
    };

    // Tape: asymmetric soft saturation
    //   x >= 0: tanh(x)
    //   x <  0: tanh(x) * (1 + 0.5*x / (1 + |x|))
    // Simplified: positive side is clean tanh, negative side is softer
    struct TapeFunc {
        float operator()(float x) const {
            if (x >= 0.0f) return std::tanh(x);
            return std::tanh(x) * (1.0f + 0.5f * x / (1.0f + std::fabs(x)));
        }
    };
    struct TapeAnti {
        float operator()(float x) const {
            // Numerical integration approximation via the midpoint of [0, x]
            // For ADAA to work, we need a continuous antiderivative.
            // Use: antiderivative(tanh(x)) = log(cosh(x)) for positive,
            // and a smooth approximation for the negative asymmetric part.
            if (x >= 0.0f) return std::log(std::cosh(x));
            // For the negative side, approximate: ∫ tanh(t)*(1+0.5t/(1+|t|)) dt
            // ≈ log(cosh(x)) + 0.5*(x - log(1+|x|)) for x < 0
            float absX = std::fabs(x);
            return std::log(std::cosh(x)) + 0.5f * (x + std::log(1.0f + absX));
        }
    };

    // Fold: sin-based wavefolder: sin(pi/2 * x)
    // Antiderivative: -2/(pi) * cos(pi/2 * x)
    struct FoldFunc {
        float operator()(float x) const {
            return std::sin(kHalfPi * x);
        }
        static constexpr float kHalfPi = 3.14159265358979323846f * 0.5f;
    };
    struct FoldAnti {
        float operator()(float x) const {
            return -(2.0f / kPi) * std::cos(kHalfPi * x);
        }
        static constexpr float kHalfPi = 3.14159265358979323846f * 0.5f;
        static constexpr float kPi = 3.14159265358979323846f;
    };

    // Asymmetric: different positive/negative curves
    //   x >= 0: x / (1 + x)        (soft saturation, asymptotes at 1)
    //   x <  0: x / (1 + |x|) * 1.5 (stronger negative saturation)
    struct AsymFunc {
        float operator()(float x) const {
            if (x >= 0.0f) return x / (1.0f + x);
            return 1.5f * x / (1.0f + std::fabs(x));
        }
    };
    struct AsymAnti {
        float operator()(float x) const {
            // ∫ x/(1+x) dx = x - log(1+x) for x >= 0
            // ∫ 1.5*x/(1+|x|) dx = 1.5*(|x| + log(1+|x|))*sign for x < 0
            if (x >= 0.0f) return x - std::log(1.0f + x);
            float absX = std::fabs(x);
            return -1.5f * (absX - std::log(1.0f + absX));
        }
    };

    // ADAA processors for each mode
    ADAA<SoftFunc, SoftAnti> m_adaaSoft{SoftFunc{}, SoftAnti{}};
    ADAA<HardFunc, HardAnti> m_adaaHard{HardFunc{}, HardAnti{}};
    ADAA<TapeFunc, TapeAnti> m_adaaTape{TapeFunc{}, TapeAnti{}};
    ADAA<FoldFunc, FoldAnti> m_adaaFold{FoldFunc{}, FoldAnti{}};
    ADAA<AsymFunc, AsymAnti> m_adaaAsym{AsymFunc{}, AsymAnti{}};

    WaveShaperMode m_mode = WaveShaperMode::Soft;
    float m_drive = 1.0f;
    float m_bias = 0.0f;
    double m_sampleRate = 44100.0;
    bool m_dcBlockEnabled = true;

    // DC blocker state
    float m_dcBlockR = 0.999f;
    float m_dcX1 = 0.0f;
    float m_dcY1 = 0.0f;
};

} // namespace libdsp

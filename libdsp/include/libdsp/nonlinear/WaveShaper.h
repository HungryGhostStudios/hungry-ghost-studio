#pragma once

#include <cmath>
#include <algorithm>

namespace libdsp {

/**
 * WaveShaper with 5 modes and optional first-order ADAA anti-aliasing.
 * Modes: Soft (tanh), Hard (clip), Tape (asymmetric), Fold (wavefolder), Asym (different +/- curves).
 * Includes integrated DC blocker (~5Hz high-pass).
 * No JUCE dependency.
 */
class WaveShaper {
public:
    enum class Mode {
        Soft,   // tanh
        Hard,   // hard clip
        Tape,   // asymmetric soft saturation
        Fold,   // sin-based wavefolder
        Asym    // different positive/negative curves
    };

    WaveShaper() = default;

    void prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        // DC blocker coefficient: ~5Hz highpass
        m_dcBlockR = 1.0f - (2.0f * static_cast<float>(M_PI) * 5.0f / static_cast<float>(sampleRate));
        reset();
    }

    void setMode(Mode mode) { m_mode = mode; }
    void setDrive(float drive) { m_drive = std::max(drive, 0.0f); }
    void setBias(float bias) { m_bias = std::clamp(bias, -0.5f, 0.5f); }
    void setDCBlock(bool enabled) { m_dcBlockEnabled = enabled; }

    /** Enable/disable ADAA anti-aliasing. */
    void setADAA(bool enabled) { m_adaaEnabled = enabled; }

    float process(float input) {
        float x = input * (1.0f + m_drive) + m_bias;
        float shaped;

        if (m_adaaEnabled) {
            shaped = processADAA(x);
        } else {
            shaped = applyShaping(x);
        }

        // DC blocker
        if (m_dcBlockEnabled) {
            float dcOut = shaped - m_dcBlockX1 + m_dcBlockR * m_dcBlockY1;
            m_dcBlockX1 = shaped;
            m_dcBlockY1 = dcOut;

            // Denormal protection
            if (std::fabs(m_dcBlockY1) < 1e-15f) m_dcBlockY1 = 0.0f;

            shaped = dcOut;
        }

        return shaped;
    }

    void reset() {
        m_xPrev = 0.0f;
        m_adPrev = 0.0f;
        m_dcBlockX1 = 0.0f;
        m_dcBlockY1 = 0.0f;
    }

private:
    float applyShaping(float x) const {
        switch (m_mode) {
            case Mode::Soft:
                return std::tanh(x);
            case Mode::Hard:
                return std::clamp(x, -1.0f, 1.0f);
            case Mode::Tape:
                // Asymmetric soft saturation
                if (x >= 0.0f)
                    return std::tanh(x);
                else
                    return std::tanh(x * 0.8f) * 1.2f;
            case Mode::Fold:
                // Sin-based wavefolder
                return std::sin(x * static_cast<float>(M_PI) * 0.5f);
            case Mode::Asym:
                // Different positive/negative curves
                if (x >= 0.0f)
                    return 1.0f - std::exp(-x);
                else
                    return -std::tanh(-x);
            default:
                return std::tanh(x);
        }
    }

    float antiderivative(float x) const {
        switch (m_mode) {
            case Mode::Soft:
                // AD of tanh(x) = ln(cosh(x))
                return std::log(std::cosh(x));
            case Mode::Hard: {
                // AD of clamp(x,-1,1)
                if (x < -1.0f) return -x - 0.5f;
                if (x > 1.0f) return x - 0.5f;
                return 0.5f * x * x;
            }
            case Mode::Tape:
                if (x >= 0.0f)
                    return std::log(std::cosh(x));
                else
                    return 1.2f * std::log(std::cosh(x * 0.8f)) / 0.8f;
            case Mode::Fold:
                // AD of sin(pi/2 * x) = -2/pi * cos(pi/2 * x)
                return -2.0f / static_cast<float>(M_PI) * std::cos(x * static_cast<float>(M_PI) * 0.5f);
            case Mode::Asym:
                if (x >= 0.0f)
                    return x + std::exp(-x) - 1.0f;
                else
                    return std::log(std::cosh(-x));
            default:
                return std::log(std::cosh(x));
        }
    }

    float processADAA(float x) {
        float ad = antiderivative(x);
        float diff = x - m_xPrev;
        float result;

        if (std::fabs(diff) < 1e-5f) {
            result = applyShaping(0.5f * (x + m_xPrev));
        } else {
            result = (ad - m_adPrev) / diff;
        }

        m_xPrev = x;
        m_adPrev = ad;
        return result;
    }

    double m_sampleRate = 44100.0;
    Mode m_mode = Mode::Soft;
    float m_drive = 0.0f;
    float m_bias = 0.0f;
    bool m_dcBlockEnabled = true;
    bool m_adaaEnabled = true;

    // ADAA state
    float m_xPrev = 0.0f;
    float m_adPrev = 0.0f;

    // DC blocker state
    float m_dcBlockR = 0.9993f;
    float m_dcBlockX1 = 0.0f;
    float m_dcBlockY1 = 0.0f;
};

} // namespace libdsp

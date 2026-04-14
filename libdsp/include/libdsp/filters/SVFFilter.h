#pragma once

#include <cmath>
#include <algorithm>

namespace libdsp {

class SVFFilter {
public:
    enum class Type {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        AllPass,
        Peak,
        LowShelf,
        HighShelf
    };

    SVFFilter() = default;

    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        reset();
        updateCoefficients();
    }

    void setType(Type type) {
        type_ = type;
        updateCoefficients();
    }

    void setFrequency(float hz) {
        frequency_ = std::clamp(hz, 20.0f, static_cast<float>(sampleRate_ * 0.49));
        updateCoefficients();
    }

    void setQ(float q) {
        q_ = std::max(q, 0.01f);
        updateCoefficients();
    }

    void setGain(float dB) {
        gainDb_ = dB;
        updateCoefficients();
    }

    float process(float input) {
        // TPT/ZDF SVF implementation
        // Based on Vadim Zavalishin's "The Art of VA Filter Design"
        const float v3 = input - ic2eq_;
        const float v1 = a1_ * ic1eq_ + a2_ * v3;
        const float v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;

        // Update state (with denormal protection)
        ic1eq_ = 2.0f * v1 - ic1eq_;
        ic2eq_ = 2.0f * v2 - ic2eq_;

        // Flush denormals
        if (std::abs(ic1eq_) < 1e-20f) ic1eq_ = 0.0f;
        if (std::abs(ic2eq_) < 1e-20f) ic2eq_ = 0.0f;

        // Mix outputs based on filter type
        return m0_ * input + m1_ * v1 + m2_ * v2;
    }

    void reset() {
        ic1eq_ = 0.0f;
        ic2eq_ = 0.0f;
    }

private:
    void updateCoefficients() {
        if (sampleRate_ <= 0.0) return;

        const float A = std::pow(10.0f, gainDb_ / 40.0f); // sqrt of linear gain
        const float g = std::tan(static_cast<float>(M_PI) * frequency_ / static_cast<float>(sampleRate_));
        const float k = 1.0f / q_;

        // TPT SVF coefficients
        a1_ = 1.0f / (1.0f + g * (g + k));
        a2_ = g * a1_;
        a3_ = g * a2_;

        switch (type_) {
        case Type::LowPass:
            m0_ = 0.0f;
            m1_ = 0.0f;
            m2_ = 1.0f;
            break;
        case Type::HighPass:
            m0_ = 1.0f;
            m1_ = -k;
            m2_ = -1.0f;
            break;
        case Type::BandPass:
            m0_ = 0.0f;
            m1_ = 1.0f;
            m2_ = 0.0f;
            break;
        case Type::Notch:
            m0_ = 1.0f;
            m1_ = -k;
            m2_ = 0.0f;
            break;
        case Type::AllPass:
            m0_ = 1.0f;
            m1_ = -2.0f * k;
            m2_ = 0.0f;
            break;
        case Type::Peak: {
            const float kA = k / A;
            a1_ = 1.0f / (1.0f + g * (g + kA));
            a2_ = g * a1_;
            a3_ = g * a2_;
            m0_ = 1.0f;
            m1_ = k * (A * A - 1.0f);
            m2_ = 0.0f;
            break;
        }
        case Type::LowShelf: {
            const float gA = g * std::sqrt(A);
            a1_ = 1.0f / (1.0f + gA * (gA + k));
            a2_ = gA * a1_;
            a3_ = gA * a2_;
            m0_ = 1.0f;
            m1_ = k * (A - 1.0f);
            m2_ = A * A - 1.0f;
            break;
        }
        case Type::HighShelf: {
            const float gA = g / std::sqrt(A);
            a1_ = 1.0f / (1.0f + gA * (gA + k));
            a2_ = gA * a1_;
            a3_ = gA * a2_;
            m0_ = A * A;
            m1_ = k * (1.0f - A) * A;
            m2_ = 1.0f - A * A;
            break;
        }
        }
    }

    double sampleRate_ = 44100.0;
    Type type_ = Type::LowPass;
    float frequency_ = 1000.0f;
    float q_ = 0.707f; // Butterworth Q
    float gainDb_ = 0.0f;

    // TPT SVF coefficients
    float a1_ = 0.0f, a2_ = 0.0f, a3_ = 0.0f;
    // Output mix coefficients
    float m0_ = 0.0f, m1_ = 0.0f, m2_ = 1.0f;
    // State variables
    float ic1eq_ = 0.0f, ic2eq_ = 0.0f;
};

} // namespace libdsp

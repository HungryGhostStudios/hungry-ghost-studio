#pragma once

#include <cmath>

namespace libdsp {

enum class SVFType
{
    LowPass,
    HighPass,
    BandPass,
    Notch,
    AllPass,
    Peak,
    LowShelf,
    HighShelf
};

class SVFFilter
{
public:
    SVFFilter() = default;

    void prepare(double sampleRate) noexcept
    {
        fs = sampleRate;
        updateCoefficients();
        reset();
    }

    void setType(SVFType newType) noexcept
    {
        type = newType;
        updateCoefficients();
    }

    void setFrequency(float hz) noexcept
    {
        frequency = hz;
        updateCoefficients();
    }

    void setQ(float newQ) noexcept
    {
        q = newQ;
        updateCoefficients();
    }

    void setGain(float dB) noexcept
    {
        gainDb = dB;
        updateCoefficients();
    }

    float process(float input) noexcept
    {
        const float v3 = input - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        // Denormal protection
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        if (std::abs(ic1eq) < 1e-15f) ic1eq = 0.0f;
        if (std::abs(ic2eq) < 1e-15f) ic2eq = 0.0f;

        switch (type)
        {
            case SVFType::LowPass:   return v2;
            case SVFType::HighPass:  return input - k * v1 - v2;
            case SVFType::BandPass:  return v1;
            case SVFType::Notch:     return input - k * v1;
            case SVFType::AllPass:   return input - 2.0f * k * v1;
            case SVFType::Peak:      return input + m1 * v1 + m2 * v2;
            case SVFType::LowShelf:  return input + m1 * v1 + m2 * v2;
            case SVFType::HighShelf: return input + m1 * v1 + m2 * v2;
        }
        return input;
    }

    void reset() noexcept
    {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }

private:
    void updateCoefficients() noexcept
    {
        const float w = std::tan(static_cast<float>(3.14159265358979323846) * frequency / static_cast<float>(fs));
        const float A = std::pow(10.0f, gainDb / 40.0f);

        switch (type)
        {
            case SVFType::LowPass:
            case SVFType::HighPass:
            case SVFType::BandPass:
            case SVFType::Notch:
            case SVFType::AllPass:
            {
                k = 1.0f / q;
                a1 = 1.0f / (1.0f + k * w + w * w);
                a2 = w * a1;
                a3 = w * a2;
                m1 = 0.0f;
                m2 = 0.0f;
                break;
            }
            case SVFType::Peak:
            {
                k = 1.0f / (q * A);
                a1 = 1.0f / (1.0f + k * w + w * w);
                a2 = w * a1;
                a3 = w * a2;
                m1 = k * (A * A - 1.0f);
                m2 = 0.0f;
                break;
            }
            case SVFType::LowShelf:
            {
                k = 1.0f / q;
                a1 = 1.0f / (1.0f + k * w + w * w);
                a2 = w * a1;
                a3 = w * a2;
                m1 = k * (A - 1.0f);
                m2 = A * A - 1.0f;
                break;
            }
            case SVFType::HighShelf:
            {
                k = 1.0f / q;
                a1 = 1.0f / (1.0f + k * w + w * w);
                a2 = w * a1;
                a3 = w * a2;
                m1 = k * (1.0f - A);
                m2 = 0.0f;
                // For high shelf, adjust the output mix
                m1 = k * (A - 1.0f);
                m2 = -(A * A - 1.0f);
                break;
            }
        }
    }

    double fs{44100.0};
    SVFType type{SVFType::LowPass};
    float frequency{1000.0f};
    float q{0.707f};
    float gainDb{0.0f};

    // TPT state variables
    float ic1eq{0.0f};
    float ic2eq{0.0f};

    // Coefficients
    float k{0.0f};
    float a1{0.0f};
    float a2{0.0f};
    float a3{0.0f};
    float m1{0.0f};
    float m2{0.0f};
};

} // namespace libdsp

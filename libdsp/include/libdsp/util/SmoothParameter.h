#pragma once

#include <cmath>

namespace libdsp {

template <typename T = float>
class SmoothParameter
{
public:
    SmoothParameter() = default;

    explicit SmoothParameter(T initialValue)
        : currentValue(initialValue), targetValue(initialValue) {}

    void prepare(double sampleRate, T cutoffHz = static_cast<T>(20))
    {
        const auto twoPi = static_cast<T>(2.0 * 3.14159265358979323846);
        coeff = static_cast<T>(1.0) - std::exp(-twoPi * cutoffHz / static_cast<T>(sampleRate));
    }

    void setTargetValue(T newTarget) noexcept
    {
        targetValue = newTarget;
    }

    T getNextValue() noexcept
    {
        currentValue += coeff * (targetValue - currentValue);
        return currentValue;
    }

    T getCurrentValue() const noexcept { return currentValue; }

    void reset(T value) noexcept
    {
        currentValue = value;
        targetValue = value;
    }

    bool isSmoothing() const noexcept
    {
        return std::abs(targetValue - currentValue) > static_cast<T>(1e-6);
    }

private:
    T currentValue{};
    T targetValue{};
    T coeff{static_cast<T>(0.1)};
};

} // namespace libdsp

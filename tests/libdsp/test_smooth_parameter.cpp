#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/util/SmoothParameter.h>

using Catch::Matchers::WithinAbs;

TEST_CASE("SmoothParameter converges to target value", "[SmoothParameter]")
{
    libdsp::SmoothParameter<float> param(0.0f);
    param.prepare(44100.0);
    param.setTargetValue(1.0f);

    // Run for enough samples to converge (44100 * 0.1s = ~4410 samples is plenty for 20Hz cutoff)
    float value = 0.0f;
    for (int i = 0; i < 4410; ++i)
        value = param.getNextValue();

    REQUIRE_THAT(value, WithinAbs(1.0f, 0.001f));
}

TEST_CASE("SmoothParameter reset jumps immediately", "[SmoothParameter]")
{
    libdsp::SmoothParameter<float> param(0.0f);
    param.prepare(44100.0);
    param.setTargetValue(5.0f);

    // Advance a few samples so we're mid-smoothing
    for (int i = 0; i < 10; ++i)
        param.getNextValue();

    param.reset(10.0f);
    REQUIRE_THAT(param.getCurrentValue(), WithinAbs(10.0f, 1e-6f));
    REQUIRE_THAT(param.getNextValue(), WithinAbs(10.0f, 1e-6f));
}

TEST_CASE("SmoothParameter works with different sample rates", "[SmoothParameter]")
{
    libdsp::SmoothParameter<float> paramLow(0.0f);
    libdsp::SmoothParameter<float> paramHigh(0.0f);

    paramLow.prepare(22050.0);
    paramHigh.prepare(96000.0);

    paramLow.setTargetValue(1.0f);
    paramHigh.setTargetValue(1.0f);

    // At lower sample rate, fewer samples needed to converge the same real-time duration
    // Run 2000 samples on each
    float valLow = 0.0f, valHigh = 0.0f;
    for (int i = 0; i < 2000; ++i)
    {
        valLow = paramLow.getNextValue();
        valHigh = paramHigh.getNextValue();
    }

    // Both should be close to target, but low SR converges faster in sample count
    // because each sample represents more time
    REQUIRE(valLow > valHigh);
    REQUIRE_THAT(valLow, WithinAbs(1.0f, 0.01f));
}

TEST_CASE("SmoothParameter isSmoothing reflects state", "[SmoothParameter]")
{
    libdsp::SmoothParameter<float> param(1.0f);
    param.prepare(44100.0);

    REQUIRE_FALSE(param.isSmoothing());

    param.setTargetValue(2.0f);
    REQUIRE(param.isSmoothing());

    // Run until converged
    for (int i = 0; i < 44100; ++i)
        param.getNextValue();

    REQUIRE_FALSE(param.isSmoothing());
}

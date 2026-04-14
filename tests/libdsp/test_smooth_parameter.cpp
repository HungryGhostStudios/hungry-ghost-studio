#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/util/SmoothParameter.h>

using Catch::Matchers::WithinAbs;

TEST_CASE("SmoothParameter converges to target value", "[SmoothParameter]")
{
    libdsp::SmoothParameter<float> param(0.0f);
    param.prepare(44100.0);
    param.setTargetValue(1.0f);

    float value = 0.0f;
    for (int i = 0; i < 44100; ++i)
        value = param.getNextValue();

    REQUIRE_THAT(value, WithinAbs(1.0f, 1e-4f));
}

TEST_CASE("SmoothParameter reset jumps immediately", "[SmoothParameter]")
{
    libdsp::SmoothParameter<float> param(0.0f);
    param.prepare(44100.0);
    param.reset(5.0f);

    REQUIRE_THAT(param.getCurrentValue(), WithinAbs(5.0f, 1e-6f));
    REQUIRE_FALSE(param.isSmoothing());
}

TEST_CASE("SmoothParameter prepare with different sample rates", "[SmoothParameter]")
{
    libdsp::SmoothParameter<float> paramLow(0.0f);
    libdsp::SmoothParameter<float> paramHigh(0.0f);

    paramLow.prepare(22050.0);
    paramHigh.prepare(96000.0);

    paramLow.setTargetValue(1.0f);
    paramHigh.setTargetValue(1.0f);

    // After 100 samples, lower sample rate should converge faster
    // (larger coefficient per sample)
    float valueLow = 0.0f, valueHigh = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        valueLow = paramLow.getNextValue();
        valueHigh = paramHigh.getNextValue();
    }

    REQUIRE(valueLow > valueHigh);
}

TEST_CASE("SmoothParameter isSmoothing reports correctly", "[SmoothParameter]")
{
    libdsp::SmoothParameter<float> param(0.0f);
    param.prepare(44100.0);

    REQUIRE_FALSE(param.isSmoothing());

    param.setTargetValue(1.0f);
    REQUIRE(param.isSmoothing());

    // Run until converged
    for (int i = 0; i < 44100; ++i)
        param.getNextValue();

    REQUIRE_FALSE(param.isSmoothing());
}

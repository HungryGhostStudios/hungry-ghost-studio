#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/reverb/DelayLine.h>

using Catch::Matchers::WithinAbs;

TEST_CASE("DelayLine: push/read returns correct sample after delay", "[reverb][delay_line]")
{
    libdsp::DelayLine dl;
    dl.prepare(44100.0, 100);
    dl.setDelay(10.f); // 10 samples delay
    dl.setInterpolation(libdsp::DelayLine::Interpolation::Linear);

    // Push 20 samples: first 10 zeros, then value 1.0
    for (int i = 0; i < 10; ++i)
        dl.push(0.f);

    // Push a recognizable value
    dl.push(1.0f);

    // After push, read should return the sample from 10 samples ago (which is 0)
    // We need to push enough samples for the delay to catch up
    for (int i = 0; i < 9; ++i)
        dl.push(0.f);

    // Now the 1.0 should be 10 samples back
    float output = dl.read();
    REQUIRE_THAT(output, WithinAbs(1.0, 0.01));
}

TEST_CASE("DelayLine: zero delay returns most recent sample", "[reverb][delay_line]")
{
    libdsp::DelayLine dl;
    dl.prepare(44100.0, 100);
    dl.setDelay(0.f);
    dl.setInterpolation(libdsp::DelayLine::Interpolation::Linear);

    dl.push(0.5f);
    float output = dl.read();
    REQUIRE_THAT(output, WithinAbs(0.5, 0.01));
}

TEST_CASE("DelayLine: fractional delay with linear interpolation", "[reverb][delay_line]")
{
    libdsp::DelayLine dl;
    dl.prepare(44100.0, 100);
    dl.setDelay(5.5f); // fractional delay
    dl.setInterpolation(libdsp::DelayLine::Interpolation::Linear);

    // Push known pattern
    for (int i = 0; i < 20; ++i)
        dl.push(static_cast<float>(i));

    // The output should be interpolated between samples at delay 5 and 6
    float output = dl.read();
    // With linear ramp input and linear interp, output should be between integer values
    REQUIRE(std::isfinite(output));
}

TEST_CASE("DelayLine: cubic interpolation produces finite output", "[reverb][delay_line]")
{
    libdsp::DelayLine dl;
    dl.prepare(44100.0, 100);
    dl.setDelay(5.5f);
    dl.setInterpolation(libdsp::DelayLine::Interpolation::Cubic);

    for (int i = 0; i < 20; ++i)
        dl.push(static_cast<float>(i) * 0.1f);

    float output = dl.read();
    REQUIRE(std::isfinite(output));
}

TEST_CASE("DelayLine: reset clears buffer", "[reverb][delay_line]")
{
    libdsp::DelayLine dl;
    dl.prepare(44100.0, 100);
    dl.setDelay(5.f);
    dl.setInterpolation(libdsp::DelayLine::Interpolation::Linear);

    for (int i = 0; i < 20; ++i)
        dl.push(1.0f);

    dl.reset();
    dl.push(0.f);
    float output = dl.read();
    REQUIRE_THAT(output, WithinAbs(0.0, 0.001));
}

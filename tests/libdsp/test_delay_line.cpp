#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/reverb/DelayLine.h>

#include <cmath>

using namespace libdsp;
using Catch::Matchers::WithinAbs;

namespace {
constexpr double kSampleRate = 44100.0;
}

TEST_CASE("DelayLine: pushed sample returned after correct delay", "[delay]")
{
    DelayLine dl;
    dl.prepare(kSampleRate, 100);
    dl.setDelay(10.f); // 10 samples delay
    dl.setInterpolation(DelayLine::Interpolation::Linear);

    // Push a unique sample and then fill with zeros
    dl.push(1.0f);
    for (int i = 0; i < 10; ++i)
        dl.push(0.0f);

    // After pushing 10 zeros after the 1.0, the delay line should output 1.0
    float out = dl.read();
    REQUIRE_THAT(out, WithinAbs(1.0, 0.01));
}

TEST_CASE("DelayLine: zero delay returns most recent sample", "[delay]")
{
    DelayLine dl;
    dl.prepare(kSampleRate, 100);
    dl.setDelay(0.f);
    dl.setInterpolation(DelayLine::Interpolation::Linear);

    dl.push(0.5f);
    float out = dl.read();
    REQUIRE_THAT(out, WithinAbs(0.5, 0.01));
}

TEST_CASE("DelayLine: fractional delay with linear interpolation", "[delay]")
{
    DelayLine dl;
    dl.prepare(kSampleRate, 100);
    dl.setDelay(5.5f); // Fractional delay
    dl.setInterpolation(DelayLine::Interpolation::Linear);

    // Push known values
    for (int i = 0; i < 20; ++i)
        dl.push(static_cast<float>(i));

    // At 5.5 sample delay, should interpolate between samples
    float out = dl.read();
    // The output should be between two adjacent pushed values
    REQUIRE(!std::isnan(out));
    REQUIRE(!std::isinf(out));
}

TEST_CASE("DelayLine: cubic interpolation produces valid output", "[delay]")
{
    DelayLine dl;
    dl.prepare(kSampleRate, 100);
    dl.setDelay(10.5f);
    dl.setInterpolation(DelayLine::Interpolation::Cubic);

    for (int i = 0; i < 30; ++i)
        dl.push(std::sin(2.f * static_cast<float>(M_PI) * 440.f * static_cast<float>(i) / static_cast<float>(kSampleRate)));

    float out = dl.read();
    REQUIRE(!std::isnan(out));
    REQUIRE(!std::isinf(out));
    REQUIRE(std::fabs(out) <= 1.5f); // Cubic can overshoot slightly
}

TEST_CASE("DelayLine: reset clears buffer", "[delay]")
{
    DelayLine dl;
    dl.prepare(kSampleRate, 100);
    dl.setDelay(5.f);

    // Fill with non-zero
    for (int i = 0; i < 20; ++i)
        dl.push(1.0f);

    dl.reset();

    // After reset, reading should give zero
    float out = dl.read();
    REQUIRE_THAT(out, WithinAbs(0.0, 1e-10));
}

TEST_CASE("DelayLine: delay clamped to max", "[delay]")
{
    DelayLine dl;
    dl.prepare(kSampleRate, 50);
    dl.setDelay(1000.f); // Way beyond max of 50

    // Should not crash
    for (int i = 0; i < 100; ++i)
        dl.push(static_cast<float>(i) * 0.01f);

    float out = dl.read();
    REQUIRE(!std::isnan(out));
    REQUIRE(!std::isinf(out));
}

TEST_CASE("DelayLine: setDelayMs converts correctly", "[delay]")
{
    DelayLine dl;
    dl.prepare(kSampleRate, 4410);
    // 10ms at 44100 Hz = 441 samples
    dl.setDelayMs(10.f);

    // Push an impulse followed by 441 zeros
    dl.push(1.0f);
    for (int i = 0; i < 441; ++i)
        dl.push(0.0f);

    float out = dl.read();
    REQUIRE_THAT(out, WithinAbs(1.0, 0.01));
}

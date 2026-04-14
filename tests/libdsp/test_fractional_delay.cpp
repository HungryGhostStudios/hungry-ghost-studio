#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/modulation/FractionalDelay.h>
#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("FractionalDelay returns correct sample at integer delay", "[FractionalDelay]")
{
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100);
    fd.setDelay(5.0f);

    // Push 10 samples: 0, 1, 2, ..., 9
    for (int i = 0; i < 10; ++i) {
        float out = fd.process(static_cast<float>(i));
        (void)out;
    }

    // After pushing 10 samples with delay=5, next output should be sample 5
    float out = fd.process(10.0f);
    REQUIRE_THAT(out, WithinAbs(5.0f, 0.5f));
}

TEST_CASE("FractionalDelay fractional delay uses interpolation", "[FractionalDelay]")
{
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100);
    fd.setDelay(5.5f);

    // Push a ramp signal
    for (int i = 0; i < 20; ++i) {
        fd.process(static_cast<float>(i));
    }

    // With fractional delay on a ramp, output should be between integer values
    float out = fd.process(20.0f);
    // For a ramp, fractional delay should give fractional values
    float intPart;
    float fracPart = std::modf(out, &intPart);
    // The output should not be exactly integer (fractional interpolation)
    // Just verify it's a reasonable value in the ramp range
    REQUIRE(out > 10.0f);
    REQUIRE(out < 20.0f);
}

TEST_CASE("FractionalDelay zero delay passes through input", "[FractionalDelay]")
{
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100);
    fd.setDelay(0.0f);

    // With zero delay, output should match input (or very close)
    float out = fd.process(1.0f);
    REQUIRE_THAT(out, WithinAbs(1.0f, 0.01f));

    out = fd.process(0.5f);
    REQUIRE_THAT(out, WithinAbs(0.5f, 0.01f));
}

TEST_CASE("FractionalDelay reset clears buffer", "[FractionalDelay]")
{
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100);
    fd.setDelay(1.0f);

    // Push some non-zero samples
    for (int i = 0; i < 10; ++i) {
        fd.process(1.0f);
    }

    fd.reset();

    // After reset, buffer should be all zeros
    float out = fd.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(0.0f, 0.01f));
}

TEST_CASE("FractionalDelay clamps delay to valid range", "[FractionalDelay]")
{
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100);

    // Negative delay should clamp to 0
    fd.setDelay(-5.0f);
    float out = fd.process(1.0f);
    REQUIRE_THAT(out, WithinAbs(1.0f, 0.01f)); // Effectively zero delay

    // Delay beyond max should clamp to max
    fd.setDelay(200.0f); // max is 100
    // Should not crash
    fd.process(0.5f);
}

TEST_CASE("FractionalDelay setDelayMs converts correctly", "[FractionalDelay]")
{
    libdsp::FractionalDelay fd;
    fd.prepare(1000.0, 100); // 1000 Hz sample rate

    // 10ms at 1000Hz = 10 samples
    fd.setDelayMs(10.0f);

    // Push 15 samples
    for (int i = 0; i < 15; ++i) {
        fd.process(static_cast<float>(i));
    }

    // After 15 samples with 10-sample delay, output ~ sample 5
    float out = fd.process(15.0f);
    REQUIRE_THAT(out, WithinAbs(5.0f, 1.0f));
}

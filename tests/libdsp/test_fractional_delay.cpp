#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/modulation/FractionalDelay.h>
#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("FractionalDelay - Integer delay", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100.0f); // 100ms max delay
    fd.setDelay(10.0f); // 10 samples delay

    // Push 10 samples of silence
    for (int i = 0; i < 10; i++) {
        float out = fd.process(0.0f);
        REQUIRE_THAT(out, WithinAbs(0.0, 0.001));
    }

    // Push impulse
    fd.process(1.0f);

    // The impulse should appear 10 samples later
    for (int i = 0; i < 9; i++) {
        fd.process(0.0f);
    }

    float out = fd.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(1.0, 0.05));
}

TEST_CASE("FractionalDelay - Fractional delay interpolation", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100.0f);
    fd.setDelay(5.5f); // Fractional delay

    // Push a ramp signal
    for (int i = 0; i < 20; i++) {
        fd.process(static_cast<float>(i));
    }

    // After enough samples, the output should be a delayed version of the ramp
    float out = fd.process(20.0f);
    // Should be approximately 20 - 5.5 = 14.5
    REQUIRE_THAT(out, WithinAbs(14.5, 0.5));
}

TEST_CASE("FractionalDelay - setDelayMs", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100.0f);

    // 1ms at 44100 Hz = 44.1 samples
    fd.setDelayMs(1.0f);

    // Push enough samples to fill the delay
    for (int i = 0; i < 50; i++) {
        fd.process(0.0f);
    }

    // Push impulse and check it arrives ~44 samples later
    fd.process(1.0f);
    float maxVal = 0.0f;
    int maxPos = 0;
    for (int i = 0; i < 50; i++) {
        float out = fd.process(0.0f);
        if (std::abs(out) > maxVal) {
            maxVal = std::abs(out);
            maxPos = i + 1;
        }
    }

    // Peak should be around sample 44
    REQUIRE(maxPos >= 43);
    REQUIRE(maxPos <= 45);
    REQUIRE(maxVal > 0.5f);
}

TEST_CASE("FractionalDelay - Reset clears buffer", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100.0f);
    fd.setDelay(5.0f);

    // Push some non-zero values
    for (int i = 0; i < 20; i++) {
        fd.process(1.0f);
    }

    fd.reset();

    // After reset, output should be zero
    float out = fd.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(0.0, 0.001));
}

TEST_CASE("FractionalDelay - Zero delay passthrough", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(44100.0, 100.0f);
    fd.setDelay(0.0f);

    // With zero delay, output should equal input
    for (int i = 0; i < 100; i++) {
        float input = static_cast<float>(i) * 0.01f;
        float out = fd.process(input);
        REQUIRE_THAT(out, WithinAbs(input, 0.001));
    }
}

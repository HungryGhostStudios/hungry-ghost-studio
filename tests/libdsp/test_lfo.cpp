#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/modulation/LFO.h>
#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("LFO Sine waveform produces correct range", "[LFO]")
{
    libdsp::LFO lfo;
    lfo.prepare(44100.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Sine);

    float minVal = 2.0f, maxVal = -2.0f;
    for (int i = 0; i < 44100; ++i) {
        float val = lfo.process();
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }

    // Sine should span [-1, 1]
    REQUIRE(minVal < -0.99f);
    REQUIRE(maxVal > 0.99f);
    REQUIRE(minVal >= -1.001f);
    REQUIRE(maxVal <= 1.001f);
}

TEST_CASE("LFO Sine starts at zero with no phase offset", "[LFO]")
{
    libdsp::LFO lfo;
    lfo.prepare(44100.0);
    lfo.setFrequency(100.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Sine);

    float first = lfo.process();
    REQUIRE_THAT(first, WithinAbs(0.0f, 0.02f));
}

TEST_CASE("LFO Triangle waveform produces correct shape", "[LFO]")
{
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Triangle);

    // At phase 0 → -1, phase 0.25 → 0, phase 0.5 → 1, phase 0.75 → 0
    float val0 = lfo.process(); // phase ~0
    REQUIRE_THAT(val0, WithinAbs(-1.0f, 0.05f));

    // Advance to phase 0.25 (250 samples at 1000Hz, 1Hz)
    for (int i = 1; i < 250; ++i) lfo.process();
    float val25 = lfo.process();
    REQUIRE_THAT(val25, WithinAbs(0.0f, 0.05f));
}

TEST_CASE("LFO Saw waveform ramps from -1 to 1", "[LFO]")
{
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Saw);

    float first = lfo.process(); // phase ~0 → -1
    REQUIRE_THAT(first, WithinAbs(-1.0f, 0.01f));

    // Advance to midpoint (phase 0.5)
    for (int i = 1; i < 500; ++i) lfo.process();
    float mid = lfo.process();
    REQUIRE_THAT(mid, WithinAbs(0.0f, 0.01f));
}

TEST_CASE("LFO Square waveform alternates between 1 and -1", "[LFO]")
{
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Square);

    // First half → 1.0
    float first = lfo.process();
    REQUIRE_THAT(first, WithinAbs(1.0f, 0.01f));

    // Second half → -1.0
    for (int i = 1; i < 500; ++i) lfo.process();
    float second = lfo.process();
    REQUIRE_THAT(second, WithinAbs(-1.0f, 0.01f));
}

TEST_CASE("LFO SampleAndHold changes value each cycle", "[LFO]")
{
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::SampleAndHold);

    // Get first cycle value
    float val1 = lfo.process();
    // Value should stay the same throughout the cycle
    for (int i = 1; i < 999; ++i) {
        float v = lfo.process();
        REQUIRE_THAT(v, WithinAbs(val1, 1e-6f));
    }

    // S&H output should be in [-1, 1]
    REQUIRE(val1 >= -1.0f);
    REQUIRE(val1 <= 1.0f);
}

TEST_CASE("LFO phase offset shifts output", "[LFO]")
{
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Sine);
    lfo.setPhase(0.25f); // 90 degree offset

    // sin(2*pi*0.25) = sin(pi/2) = 1.0
    float first = lfo.process();
    REQUIRE_THAT(first, WithinAbs(1.0f, 0.02f));
}

TEST_CASE("LFO reset returns phase to zero", "[LFO]")
{
    libdsp::LFO lfo;
    lfo.prepare(44100.0);
    lfo.setFrequency(100.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Sine);

    // Run for a while
    for (int i = 0; i < 10000; ++i) lfo.process();

    lfo.reset();
    float afterReset = lfo.process();
    REQUIRE_THAT(afterReset, WithinAbs(0.0f, 0.02f));
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/modulation/LFO.h>
#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("LFO - Sine waveform", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(1000.0); // 1kHz sample rate
    lfo.setFrequency(1.0f); // 1Hz
    lfo.setWaveform(libdsp::LFOWaveform::Sine);

    // At phase 0, sin(0) = 0
    float val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(0.0, 0.01));

    // After 250 samples (quarter period), sin(pi/2) = 1
    for (int i = 1; i < 250; i++) lfo.process();
    val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(1.0, 0.02));

    // After 500 samples (half period), sin(pi) = 0
    for (int i = 251; i < 500; i++) lfo.process();
    val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(0.0, 0.02));
}

TEST_CASE("LFO - Triangle waveform", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFOWaveform::Triangle);

    // At phase 0: 4*0 - 1 = -1
    float val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(-1.0, 0.01));

    // At phase 0.25: 4*0.25 - 1 = 0
    for (int i = 1; i < 250; i++) lfo.process();
    val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(0.0, 0.02));

    // At phase 0.5: 4*0.5 - 1 = 1 (boundary, about to go into second half)
    for (int i = 250; i < 500; i++) lfo.process();
    val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(1.0, 0.02));
}

TEST_CASE("LFO - Square waveform", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFOWaveform::Square);

    // First half should be +1
    float val = lfo.process();
    REQUIRE(val == 1.0f);

    // At phase 0.5+ should be -1
    for (int i = 1; i < 500; i++) lfo.process();
    val = lfo.process();
    REQUIRE(val == -1.0f);
}

TEST_CASE("LFO - Saw waveform", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFOWaveform::Saw);

    // At phase 0: 2*0 - 1 = -1
    float val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(-1.0, 0.01));

    // At phase 0.5: 2*0.5 - 1 = 0
    for (int i = 1; i < 500; i++) lfo.process();
    val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(0.0, 0.01));
}

TEST_CASE("LFO - Phase offset", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFOWaveform::Sine);
    lfo.setPhase(0.25f); // 90 degree offset

    // sin(2*pi*0.25) = sin(pi/2) = 1
    float val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(1.0, 0.02));
}

TEST_CASE("LFO - Output range [-1, 1]", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(44100.0);
    lfo.setFrequency(5.0f);

    for (auto wf : {libdsp::LFOWaveform::Sine, libdsp::LFOWaveform::Triangle,
                     libdsp::LFOWaveform::Saw, libdsp::LFOWaveform::Square,
                     libdsp::LFOWaveform::SampleAndHold}) {
        lfo.setWaveform(wf);
        lfo.reset();
        for (int i = 0; i < 44100; i++) {
            float val = lfo.process();
            REQUIRE(val >= -1.0f);
            REQUIRE(val <= 1.0f);
        }
    }
}

TEST_CASE("LFO - Reset", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(1000.0);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFOWaveform::Sine);

    // Process some samples
    for (int i = 0; i < 300; i++) lfo.process();

    // Reset and verify it starts from phase 0 again
    lfo.reset();
    float val = lfo.process();
    REQUIRE_THAT(val, WithinAbs(0.0, 0.01));
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/modulation/LFO.h>
#include <libdsp/modulation/FractionalDelay.h>

#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace {

constexpr double kSampleRate = 44100.0;

} // namespace

// ===== LFO Tests =====

TEST_CASE("LFO sine waveform produces values in [-1, 1]", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(kSampleRate);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Sine);

    for (int i = 0; i < 44100; ++i) {
        float val = lfo.process();
        REQUIRE(val >= -1.0f);
        REQUIRE(val <= 1.0f);
    }
}

TEST_CASE("LFO sine completes one cycle at set frequency", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(kSampleRate);
    lfo.setFrequency(1.0f); // 1 Hz
    lfo.setWaveform(libdsp::LFO::Waveform::Sine);

    // At 1 Hz and 44100 sample rate, one full cycle = 44100 samples
    // At sample 0: sin(0) = 0
    // At sample 11025 (quarter cycle): sin(pi/2) ≈ 1
    float firstSample = lfo.process();
    REQUIRE_THAT(firstSample, WithinAbs(0.0, 0.01));

    // Skip to quarter cycle
    for (int i = 1; i < 11025; ++i) lfo.process();
    float quarterCycle = lfo.process();
    REQUIRE_THAT(quarterCycle, WithinAbs(1.0, 0.01));
}

TEST_CASE("LFO triangle waveform produces values in [-1, 1]", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(kSampleRate);
    lfo.setFrequency(10.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Triangle);

    float minVal = 2.0f, maxVal = -2.0f;
    for (int i = 0; i < 44100; ++i) {
        float val = lfo.process();
        REQUIRE(val >= -1.0f);
        REQUIRE(val <= 1.0f);
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
    }
    // Should reach near extremes over 10 full cycles
    REQUIRE(maxVal > 0.95f);
    REQUIRE(minVal < -0.95f);
}

TEST_CASE("LFO saw waveform sweeps from -1 to 1", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(kSampleRate);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Saw);

    float firstSample = lfo.process();
    // At phase 0, saw = 2*0 - 1 = -1
    REQUIRE_THAT(firstSample, WithinAbs(-1.0, 0.01));
}

TEST_CASE("LFO square waveform produces +1 or -1", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(kSampleRate);
    lfo.setFrequency(1.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::Square);

    for (int i = 0; i < 44100; ++i) {
        float val = lfo.process();
        REQUIRE((val == 1.0f || val == -1.0f));
    }
}

TEST_CASE("LFO sample-and-hold produces values in [-1, 1]", "[lfo]") {
    libdsp::LFO lfo;
    lfo.prepare(kSampleRate);
    lfo.setFrequency(10.0f);
    lfo.setWaveform(libdsp::LFO::Waveform::SampleAndHold);

    for (int i = 0; i < 44100; ++i) {
        float val = lfo.process();
        REQUIRE(val >= -1.0f);
        REQUIRE(val <= 1.0f);
    }
}

TEST_CASE("LFO phase offset shifts the waveform", "[lfo]") {
    libdsp::LFO lfo1, lfo2;
    lfo1.prepare(kSampleRate);
    lfo2.prepare(kSampleRate);
    lfo1.setFrequency(1.0f);
    lfo2.setFrequency(1.0f);
    lfo1.setWaveform(libdsp::LFO::Waveform::Sine);
    lfo2.setWaveform(libdsp::LFO::Waveform::Sine);

    lfo1.setPhase(0.0f);
    lfo2.setPhase(0.25f); // 90 degrees offset

    // lfo1 at phase 0 = sin(0) = 0
    // lfo2 at phase 0.25 = sin(pi/2) = 1
    float val1 = lfo1.process();
    float val2 = lfo2.process();

    REQUIRE_THAT(val1, WithinAbs(0.0, 0.01));
    REQUIRE_THAT(val2, WithinAbs(1.0, 0.01));
}

// ===== FractionalDelay Tests =====

TEST_CASE("FractionalDelay with integer delay returns exact delayed sample", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(kSampleRate, 100.0f); // 100ms max

    // Set delay to exactly 10 samples
    fd.setDelay(10.0f);

    // Push an impulse and then zeros
    float impulseOut = fd.process(1.0f);
    // Impulse shouldn't appear yet (delay = 10)
    (void)impulseOut;

    for (int i = 1; i < 10; ++i) {
        float val = fd.process(0.0f);
        // Should be near zero before delay time
        REQUIRE(std::fabs(val) < 0.1f);
    }

    // At sample 10, we should get the impulse back
    float delayed = fd.process(0.0f);
    REQUIRE(std::fabs(delayed) > 0.5f);
}

TEST_CASE("FractionalDelay passes through DC signal after settling", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(kSampleRate, 50.0f);
    fd.setDelay(100.0f); // 100 samples

    // Feed constant value and let it settle
    for (int i = 0; i < 200; ++i) {
        fd.process(0.75f);
    }

    // After settling, output should match input
    float out = fd.process(0.75f);
    REQUIRE_THAT(out, WithinAbs(0.75, 0.01));
}

TEST_CASE("FractionalDelay reset clears buffer", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(kSampleRate, 50.0f);
    fd.setDelay(10.0f);

    // Feed some signal
    for (int i = 0; i < 50; ++i) {
        fd.process(1.0f);
    }

    fd.reset();

    // After reset, output should be silent
    float out = fd.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(0.0, 0.001));
}

TEST_CASE("FractionalDelay setDelayMs converts correctly", "[fractional_delay]") {
    libdsp::FractionalDelay fd;
    fd.prepare(kSampleRate, 100.0f);

    // 1ms at 44100 Hz = 44.1 samples
    fd.setDelayMs(1.0f);

    // Feed an impulse and check it arrives around sample 44
    fd.process(1.0f);
    for (int i = 1; i < 44; ++i) {
        fd.process(0.0f);
    }
    float atSample44 = fd.process(0.0f);
    // Should have significant energy near the delay time
    REQUIRE(std::fabs(atSample44) > 0.1f);
}

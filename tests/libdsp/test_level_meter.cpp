#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/util/LevelMeter.h>

#include <cmath>

using namespace libdsp;
using Catch::Matchers::WithinAbs;

namespace {
constexpr double kSampleRate = 44100.0;
}

TEST_CASE("LevelMeter: peak tracks input amplitude", "[util][level_meter]")
{
    LevelMeter meter;
    meter.prepare(kSampleRate);
    meter.setAttack(0.1f);  // Very fast attack
    meter.setRelease(300.f);

    // Feed constant amplitude
    for (int i = 0; i < 4096; ++i)
        meter.push(0.8f);

    float peak = meter.getPeak();
    REQUIRE(peak > 0.7f);
    REQUIRE(peak < 0.9f);
}

TEST_CASE("LevelMeter: RMS of sine wave is correct", "[util][level_meter]")
{
    LevelMeter meter;
    meter.prepare(kSampleRate);
    meter.setAttack(1.f);
    meter.setRelease(100.f);

    const float amplitude = 1.0f;
    const float freq = 440.f;

    // Feed one second of sine wave
    for (int i = 0; i < 44100; ++i)
    {
        float sample = amplitude * std::sin(2.f * static_cast<float>(M_PI) * freq * static_cast<float>(i) / static_cast<float>(kSampleRate));
        meter.push(sample);
    }

    float rms = meter.getRMS();
    float expectedRMS = amplitude / std::sqrt(2.f); // ~0.707

    // Allow generous tolerance for ballistics-smoothed RMS
    REQUIRE(rms > expectedRMS * 0.5f);
    REQUIRE(rms < expectedRMS * 1.5f);
}

TEST_CASE("LevelMeter: dB conversion correct for known level", "[util][level_meter]")
{
    LevelMeter meter;
    meter.prepare(kSampleRate);
    meter.setAttack(0.01f);
    meter.setRelease(300.f);

    // Feed unity amplitude
    for (int i = 0; i < 8192; ++i)
        meter.push(1.0f);

    float peakDb = meter.getPeakDb();
    // Peak of 1.0 should be ~0 dBFS
    REQUIRE(peakDb > -1.f);
    REQUIRE(peakDb < 1.f);
}

TEST_CASE("LevelMeter: silence returns very low dB", "[util][level_meter]")
{
    LevelMeter meter;
    meter.prepare(kSampleRate);

    // Push silence for a while to let release drain
    for (int i = 0; i < 44100; ++i)
        meter.push(0.0f);

    REQUIRE(meter.getPeakDb() <= -120.f);
    REQUIRE(meter.getRMSDb() <= -120.f);
}

TEST_CASE("LevelMeter: reset clears levels", "[util][level_meter]")
{
    LevelMeter meter;
    meter.prepare(kSampleRate);

    // Feed signal
    for (int i = 0; i < 1000; ++i)
        meter.push(0.9f);

    meter.reset();

    REQUIRE_THAT(meter.getPeak(), WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(meter.getRMS(), WithinAbs(0.0, 1e-10));
}

TEST_CASE("LevelMeter: peak tracks negative samples", "[util][level_meter]")
{
    LevelMeter meter;
    meter.prepare(kSampleRate);
    meter.setAttack(0.01f);
    meter.setRelease(300.f);

    for (int i = 0; i < 4096; ++i)
        meter.push(-0.6f);

    float peak = meter.getPeak();
    REQUIRE(peak > 0.5f); // Should track absolute value
}

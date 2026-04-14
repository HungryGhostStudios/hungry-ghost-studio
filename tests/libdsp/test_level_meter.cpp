#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/util/LevelMeter.h>

#include <cmath>

using Catch::Matchers::WithinAbs;

static constexpr double kSampleRate = 44100.0;

TEST_CASE("LevelMeter: peak tracks input amplitude", "[util][level_meter]")
{
    libdsp::LevelMeter meter;
    meter.prepare(kSampleRate);
    meter.setAttack(1.f);   // very fast attack
    meter.setRelease(300.f);

    // Feed constant amplitude 0.8
    for (int i = 0; i < 4410; ++i) // 100ms
        meter.push(0.8f);

    // Peak should converge near 0.8
    REQUIRE_THAT(meter.getPeak(), WithinAbs(0.8, 0.05));
}

TEST_CASE("LevelMeter: RMS of sine approximates peak/sqrt(2)", "[util][level_meter]")
{
    libdsp::LevelMeter meter;
    meter.prepare(kSampleRate);
    meter.setAttack(1.f);
    meter.setRelease(300.f);

    // Feed 1kHz sine at amplitude 1.0 for 1 second
    for (int i = 0; i < 44100; ++i) {
        float sample = static_cast<float>(std::sin(2.0 * M_PI * 1000.0 * i / kSampleRate));
        meter.push(sample);
    }

    // RMS should be ~0.707
    REQUIRE_THAT(meter.getRMS(), WithinAbs(0.707, 0.05));
}

TEST_CASE("LevelMeter: silence produces zero level", "[util][level_meter]")
{
    libdsp::LevelMeter meter;
    meter.prepare(kSampleRate);

    for (int i = 0; i < 1000; ++i)
        meter.push(0.f);

    REQUIRE(meter.getPeak() == 0.f);
    REQUIRE(meter.getRMS() == 0.f);
}

TEST_CASE("LevelMeter: getPeakDb returns correct dB", "[util][level_meter]")
{
    libdsp::LevelMeter meter;
    meter.prepare(kSampleRate);
    meter.setAttack(0.1f); // very fast

    // Feed constant 1.0 amplitude
    for (int i = 0; i < 4410; ++i)
        meter.push(1.0f);

    // Peak should be near 1.0, so dB should be near 0
    REQUIRE_THAT(meter.getPeakDb(), WithinAbs(0.0, 1.0));
}

TEST_CASE("LevelMeter: reset clears state", "[util][level_meter]")
{
    libdsp::LevelMeter meter;
    meter.prepare(kSampleRate);

    for (int i = 0; i < 4410; ++i)
        meter.push(0.5f);

    REQUIRE(meter.getPeak() > 0.f);

    meter.reset();
    REQUIRE(meter.getPeak() == 0.f);
    REQUIRE(meter.getRMS() == 0.f);
}

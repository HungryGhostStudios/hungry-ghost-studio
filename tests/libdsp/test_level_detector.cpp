#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/dynamics/LevelDetector.h>

#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

static constexpr double kSampleRate = 44100.0;

TEST_CASE("LevelDetector: peak mode tracks amplitude", "[dynamics][level_detector]")
{
    libdsp::dynamics::LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(libdsp::dynamics::DetectorMode::Peak);
    det.setAttack(1.f);   // fast attack
    det.setRelease(50.f);

    // Feed constant amplitude signal
    float level = 0.f;
    for (int i = 0; i < 4410; ++i) // 100ms of signal
        level = det.process(0.5f);

    // Peak detector should converge near 0.5
    REQUIRE_THAT(level, WithinAbs(0.5, 0.05));
}

TEST_CASE("LevelDetector: RMS mode tracks sine amplitude", "[dynamics][level_detector]")
{
    libdsp::dynamics::LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(libdsp::dynamics::DetectorMode::RMS);
    det.setAttack(5.f);
    det.setRelease(50.f);

    // Feed 1kHz sine wave at amplitude 1.0
    float level = 0.f;
    for (int i = 0; i < 44100; ++i) { // 1 second
        float sample = std::sin(2.0 * M_PI * 1000.0 * i / kSampleRate);
        level = det.process(sample);
    }

    // RMS of sine = peak / sqrt(2) ≈ 0.707
    REQUIRE_THAT(level, WithinAbs(0.707, 0.05));
}

TEST_CASE("LevelDetector: attack/release envelope tracking", "[dynamics][level_detector]")
{
    libdsp::dynamics::LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(libdsp::dynamics::DetectorMode::Peak);
    det.setAttack(10.f);
    det.setRelease(100.f);

    // Burst of signal then silence
    float peakDuringBurst = 0.f;
    for (int i = 0; i < 4410; ++i) // 100ms burst
        peakDuringBurst = det.process(1.0f);

    // Should have risen significantly
    REQUIRE(peakDuringBurst > 0.5f);

    // Now silence — level should decay
    float afterRelease = 0.f;
    for (int i = 0; i < 44100; ++i) // 1 second of silence
        afterRelease = det.process(0.0f);

    REQUIRE(afterRelease < 0.01f);
}

TEST_CASE("LevelDetector: silence produces zero output", "[dynamics][level_detector]")
{
    libdsp::dynamics::LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(libdsp::dynamics::DetectorMode::Peak);

    float level = 0.f;
    for (int i = 0; i < 1000; ++i)
        level = det.process(0.0f);

    REQUIRE(level == 0.f);
}

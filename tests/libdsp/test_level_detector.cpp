#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/dynamics/LevelDetector.h>

#include <cmath>
#include <vector>

using namespace libdsp::dynamics;
using Catch::Matchers::WithinAbs;

namespace {
constexpr double kSampleRate = 44100.0;
}

TEST_CASE("LevelDetector: tracks peak of constant signal", "[dynamics][level_detector]")
{
    LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(DetectorMode::Peak);
    det.setAttack(0.01f);  // Very fast attack
    det.setRelease(100.f);

    // Feed a constant amplitude signal
    const float amplitude = 0.8f;
    float level = 0.f;
    for (int i = 0; i < 4096; ++i)
        level = det.process(amplitude);

    // After settling, should be close to input amplitude
    REQUIRE_THAT(level, WithinAbs(static_cast<double>(amplitude), 0.05));
}

TEST_CASE("LevelDetector: tracks RMS of sine wave", "[dynamics][level_detector]")
{
    LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(DetectorMode::RMS);
    det.setAttack(1.f);
    det.setRelease(50.f);

    // Feed a sine wave — RMS of sine = amplitude / sqrt(2)
    const float amplitude = 1.0f;
    const float freq = 440.f;
    float level = 0.f;
    const int numSamples = 44100; // 1 second
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = amplitude * std::sin(2.f * static_cast<float>(M_PI) * freq * static_cast<float>(i) / static_cast<float>(kSampleRate));
        level = det.process(sample);
    }

    float expectedRMS = amplitude / std::sqrt(2.f);
    // RMS tracking with ballistics won't be exact, allow generous tolerance
    REQUIRE(level > expectedRMS * 0.5f);
    REQUIRE(level < expectedRMS * 1.5f);
}

TEST_CASE("LevelDetector: attack is faster than release", "[dynamics][level_detector]")
{
    LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(DetectorMode::Peak);
    det.setAttack(1.f);    // 1ms attack
    det.setRelease(200.f); // 200ms release

    // Attack phase: feed loud signal
    for (int i = 0; i < 441; ++i) // ~10ms
        det.process(1.0f);
    float peakAfterAttack = det.process(1.0f);

    // Now feed silence and check how fast it drops
    float levelAfterShortSilence = 0.f;
    for (int i = 0; i < 441; ++i) // ~10ms of silence
        levelAfterShortSilence = det.process(0.0f);

    // After 10ms of silence with 200ms release, level should still be well above 0
    REQUIRE(levelAfterShortSilence > 0.5f);
}

TEST_CASE("LevelDetector: output is zero for silent input", "[dynamics][level_detector]")
{
    LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(DetectorMode::Peak);
    det.setAttack(1.f);
    det.setRelease(10.f);

    float level = 0.f;
    for (int i = 0; i < 44100; ++i)
        level = det.process(0.0f);

    REQUIRE_THAT(level, WithinAbs(0.0, 1e-10));
}

TEST_CASE("LevelDetector: handles negative samples correctly", "[dynamics][level_detector]")
{
    LevelDetector det;
    det.prepare(kSampleRate);
    det.setMode(DetectorMode::Peak);
    det.setAttack(0.01f);
    det.setRelease(100.f);

    // Feed negative amplitude — peak mode uses fabs
    float level = 0.f;
    for (int i = 0; i < 4096; ++i)
        level = det.process(-0.7f);

    REQUIRE(level > 0.5f); // Should track absolute value
}

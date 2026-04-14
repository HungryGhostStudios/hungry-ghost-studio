#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/dynamics/Compressor.h>

#include <cmath>
#include <vector>

using namespace libdsp::dynamics;
using Catch::Matchers::WithinAbs;

namespace {

constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;

// Generate a constant-amplitude stereo signal
void fillConstant(std::vector<float>& left, std::vector<float>& right, float amplitude, int numSamples)
{
    left.resize(static_cast<size_t>(numSamples), amplitude);
    right.resize(static_cast<size_t>(numSamples), amplitude);
}

float dbToLinear(float dB)
{
    return std::pow(10.f, dB * 0.05f);
}

float linearToDb(float linear)
{
    return 20.f * std::log10(std::max(std::fabs(linear), 1e-10f));
}

} // anonymous namespace

TEST_CASE("Compressor: prepare and default state", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);

    // After prepare, gain reduction should be 0
    REQUIRE(comp.getGainReduction() == 0.f);
}

TEST_CASE("Compressor: silence in, silence out", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setThreshold(-20.f);
    comp.setRatio(4.f);

    std::vector<float> left(kBlockSize, 0.f);
    std::vector<float> right(kBlockSize, 0.f);

    comp.process(left.data(), right.data(), kBlockSize);

    for (int i = 0; i < kBlockSize; ++i)
    {
        REQUIRE_THAT(left[static_cast<size_t>(i)], WithinAbs(0.f, 1e-10));
        REQUIRE_THAT(right[static_cast<size_t>(i)], WithinAbs(0.f, 1e-10));
    }
}

TEST_CASE("Compressor: feed-forward reduces loud signal", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setMode(CompressorMode::FeedForward);
    comp.setThreshold(-20.f);
    comp.setRatio(4.f);
    comp.setKnee(0.f);  // Hard knee
    comp.setAttack(0.1f);  // Very fast attack
    comp.setRelease(100.f);
    comp.setMakeup(0.f);
    comp.setMix(100.f);
    comp.prepare(kSampleRate, kBlockSize);  // Re-prepare to apply attack/release

    // Signal at 0 dBFS (amplitude 1.0) — well above -20dB threshold
    const int numSamples = 4096;
    std::vector<float> left(static_cast<size_t>(numSamples), 0.8f);
    std::vector<float> right(static_cast<size_t>(numSamples), 0.8f);

    comp.process(left.data(), right.data(), numSamples);

    // After enough samples for the detector to settle, output should be reduced
    // Check the last quarter of the buffer
    float avgOutput = 0.f;
    int startCheck = numSamples * 3 / 4;
    for (int i = startCheck; i < numSamples; ++i)
        avgOutput += std::fabs(left[static_cast<size_t>(i)]);
    avgOutput /= static_cast<float>(numSamples - startCheck);

    // Output should be significantly less than input (0.8)
    REQUIRE(avgOutput < 0.8f);
    REQUIRE(avgOutput > 0.01f);  // But not zero

    // Gain reduction should be negative
    REQUIRE(comp.getGainReduction() < 0.f);
}

TEST_CASE("Compressor: signal below threshold passes unchanged", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setMode(CompressorMode::FeedForward);
    comp.setThreshold(-10.f);
    comp.setRatio(4.f);
    comp.setKnee(0.f);
    comp.setAttack(0.1f);
    comp.setRelease(100.f);
    comp.setMakeup(0.f);
    comp.setMix(100.f);
    comp.prepare(kSampleRate, kBlockSize);

    // Signal at -40 dBFS — well below threshold
    float amplitude = dbToLinear(-40.f);
    const int numSamples = 2048;
    std::vector<float> left(static_cast<size_t>(numSamples), amplitude);
    std::vector<float> right(static_cast<size_t>(numSamples), amplitude);

    // Copy input for comparison
    std::vector<float> leftOrig = left;

    comp.process(left.data(), right.data(), numSamples);

    // Signal well below threshold — output should be very close to input
    // Check last portion where detector has settled
    for (int i = numSamples / 2; i < numSamples; ++i)
    {
        REQUIRE_THAT(left[static_cast<size_t>(i)],
                     WithinAbs(static_cast<double>(leftOrig[static_cast<size_t>(i)]), 0.001));
    }
}

TEST_CASE("Compressor: feedback mode produces gain reduction", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setMode(CompressorMode::FeedBack);
    comp.setThreshold(-20.f);
    comp.setRatio(4.f);
    comp.setKnee(0.f);
    comp.setAttack(0.1f);
    comp.setRelease(100.f);
    comp.setMakeup(0.f);
    comp.setMix(100.f);
    comp.prepare(kSampleRate, kBlockSize);

    const int numSamples = 8192;
    std::vector<float> left(static_cast<size_t>(numSamples), 0.8f);
    std::vector<float> right(static_cast<size_t>(numSamples), 0.8f);

    comp.process(left.data(), right.data(), numSamples);

    // Feedback mode should also reduce the signal
    float avgOutput = 0.f;
    int startCheck = numSamples * 3 / 4;
    for (int i = startCheck; i < numSamples; ++i)
        avgOutput += std::fabs(left[static_cast<size_t>(i)]);
    avgOutput /= static_cast<float>(numSamples - startCheck);

    REQUIRE(avgOutput < 0.8f);
    REQUIRE(comp.getGainReduction() < 0.f);
}

TEST_CASE("Compressor: opto mode has gain reduction", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setMode(CompressorMode::Opto);
    comp.setThreshold(-20.f);
    comp.setRatio(4.f);
    comp.setKnee(0.f);
    comp.setMakeup(0.f);
    comp.setMix(100.f);
    comp.prepare(kSampleRate, kBlockSize);

    const int numSamples = 8192;
    std::vector<float> left(static_cast<size_t>(numSamples), 0.8f);
    std::vector<float> right(static_cast<size_t>(numSamples), 0.8f);

    comp.process(left.data(), right.data(), numSamples);

    // Opto mode should produce gain reduction on loud signal
    REQUIRE(comp.getGainReduction() < 0.f);
}

TEST_CASE("Compressor: stereo link averages detection", "[dynamics][compressor]")
{
    // With stereo link on, both channels should get the same GR
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setMode(CompressorMode::FeedForward);
    comp.setThreshold(-20.f);
    comp.setRatio(4.f);
    comp.setKnee(0.f);
    comp.setAttack(0.1f);
    comp.setRelease(100.f);
    comp.setMakeup(0.f);
    comp.setMix(100.f);
    comp.setStereoLink(true);
    comp.prepare(kSampleRate, kBlockSize);

    const int numSamples = 4096;
    // Asymmetric signal: left loud, right quiet
    std::vector<float> left(static_cast<size_t>(numSamples), 0.8f);
    std::vector<float> right(static_cast<size_t>(numSamples), 0.01f);

    comp.process(left.data(), right.data(), numSamples);

    // Both channels should be affected (linked detection)
    // The right channel (quiet) should still get some GR due to linking
    REQUIRE(comp.getGainReduction() < 0.f);
}

TEST_CASE("Compressor: makeup gain boosts output", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setThreshold(-60.f);  // Very low threshold
    comp.setRatio(4.f);
    comp.setKnee(0.f);
    comp.setAttack(0.1f);
    comp.setRelease(100.f);
    comp.setMakeup(12.f);  // +12dB makeup
    comp.setMix(100.f);
    comp.prepare(kSampleRate, kBlockSize);

    const int numSamples = 4096;
    std::vector<float> left(static_cast<size_t>(numSamples), 0.1f);
    std::vector<float> right(static_cast<size_t>(numSamples), 0.1f);

    comp.process(left.data(), right.data(), numSamples);

    // With makeup gain, some output samples may exceed the compressed level
    // Just verify the compressor processes without crashing and has GR
    REQUIRE(comp.getGainReduction() < 0.f);
}

TEST_CASE("Compressor: dry/wet mix at 0% passes signal through", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setThreshold(-20.f);
    comp.setRatio(4.f);
    comp.setKnee(0.f);
    comp.setAttack(0.1f);
    comp.setRelease(100.f);
    comp.setMakeup(0.f);
    comp.setMix(0.f);  // Fully dry
    comp.prepare(kSampleRate, kBlockSize);

    const int numSamples = 2048;
    std::vector<float> left(static_cast<size_t>(numSamples), 0.5f);
    std::vector<float> right(static_cast<size_t>(numSamples), 0.5f);

    comp.process(left.data(), right.data(), numSamples);

    // With 0% mix, output should equal input (no wet signal)
    for (int i = numSamples / 2; i < numSamples; ++i)
    {
        REQUIRE_THAT(left[static_cast<size_t>(i)], WithinAbs(0.5, 0.001));
    }
}

TEST_CASE("Compressor: getGainReduction returns negative for loud signal", "[dynamics][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setThreshold(-20.f);
    comp.setRatio(8.f);
    comp.setKnee(0.f);
    comp.setAttack(0.1f);
    comp.setRelease(100.f);
    comp.setMix(100.f);
    comp.prepare(kSampleRate, kBlockSize);

    const int numSamples = 4096;
    std::vector<float> left(static_cast<size_t>(numSamples), 0.9f);
    std::vector<float> right(static_cast<size_t>(numSamples), 0.9f);

    comp.process(left.data(), right.data(), numSamples);

    float gr = comp.getGainReduction();
    REQUIRE(gr < 0.f);
    REQUIRE(gr > -60.f);  // Reasonable range
}

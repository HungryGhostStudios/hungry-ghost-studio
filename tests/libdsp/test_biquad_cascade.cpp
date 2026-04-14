#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/filters/BiquadCascade.h>
#include <libdsp/filters/Coefficients.h>
#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

static constexpr float kSampleRate = 44100.0f;
static constexpr double kPi = 3.14159265358979323846;

// Helper: measure magnitude response at a given frequency by feeding a sine burst
static float measureMagnitude(libdsp::BiquadCascade& cascade, float testFreqHz, float sr, int numCycles = 50) {
    cascade.reset();
    const int samplesPerCycle = static_cast<int>(sr / testFreqHz);
    const int totalSamples = samplesPerCycle * numCycles;
    const int measureStart = samplesPerCycle * (numCycles / 2); // skip transient

    float peakInput = 0.0f;
    float peakOutput = 0.0f;

    for (int i = 0; i < totalSamples; ++i) {
        const float input = std::sin(2.0f * static_cast<float>(kPi) * testFreqHz * static_cast<float>(i) / sr);
        const float output = cascade.process(input);
        if (i >= measureStart) {
            peakInput = std::max(peakInput, std::fabs(input));
            peakOutput = std::max(peakOutput, std::fabs(output));
        }
    }
    return (peakInput > 0.0f) ? (peakOutput / peakInput) : 0.0f;
}

// ---- BiquadCascade tests ----

TEST_CASE("BiquadCascade passthrough with default coefficients", "[BiquadCascade]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    // Default BiquadCoeffs is b0=1, rest=0 → passthrough
    for (int i = 0; i < 100; ++i) {
        const float input = std::sin(2.0f * static_cast<float>(kPi) * 440.0f * static_cast<float>(i) / kSampleRate);
        REQUIRE_THAT(cascade.process(input), WithinAbs(input, 1e-6f));
    }
}

TEST_CASE("BiquadCascade setNumStages and reset", "[BiquadCascade]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);

    cascade.setNumStages(4);
    REQUIRE(cascade.getNumStages() == 4);

    cascade.setNumStages(2);
    REQUIRE(cascade.getNumStages() == 2);

    cascade.setNumStages(0);
    REQUIRE(cascade.getNumStages() == 0);

    // Zero stages = passthrough
    REQUIRE_THAT(cascade.process(1.0f), WithinAbs(1.0f, 1e-6f));
}

TEST_CASE("BiquadCascade LP filter attenuates high frequencies", "[BiquadCascade]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto lpCoeffs = libdsp::EQCoeffs::makeLP(1000.0f, 0.707f, kSampleRate);
    cascade.setCoefficients(0, lpCoeffs);

    // Low frequency should pass through (~1.0 magnitude)
    float magLow = measureMagnitude(cascade, 100.0f, kSampleRate);
    REQUIRE(magLow > 0.9f);

    // High frequency should be attenuated
    float magHigh = measureMagnitude(cascade, 10000.0f, kSampleRate);
    REQUIRE(magHigh < 0.2f);
}

TEST_CASE("BiquadCascade HP filter attenuates low frequencies", "[BiquadCascade]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto hpCoeffs = libdsp::EQCoeffs::makeHP(1000.0f, 0.707f, kSampleRate);
    cascade.setCoefficients(0, hpCoeffs);

    float magLow = measureMagnitude(cascade, 100.0f, kSampleRate);
    REQUIRE(magLow < 0.2f);

    float magHigh = measureMagnitude(cascade, 10000.0f, kSampleRate);
    REQUIRE(magHigh > 0.9f);
}

TEST_CASE("BiquadCascade multi-stage steeper rolloff", "[BiquadCascade]")
{
    // Two cascaded LP filters should give steeper rolloff than one
    libdsp::BiquadCascade single;
    single.prepare(kSampleRate);
    single.setNumStages(1);
    auto lp = libdsp::EQCoeffs::makeLP(1000.0f, 0.707f, kSampleRate);
    single.setCoefficients(0, lp);

    libdsp::BiquadCascade dual;
    dual.prepare(kSampleRate);
    dual.setNumStages(2);
    dual.setCoefficients(0, lp);
    dual.setCoefficients(1, lp);

    float singleMag = measureMagnitude(single, 5000.0f, kSampleRate);
    float dualMag = measureMagnitude(dual, 5000.0f, kSampleRate);

    // Dual cascade should attenuate more
    REQUIRE(dualMag < singleMag);
}

TEST_CASE("BiquadCascade processBatch matches sample-by-sample", "[BiquadCascade]")
{
    auto lp = libdsp::EQCoeffs::makeLP(2000.0f, 0.707f, kSampleRate);

    // Process sample by sample
    libdsp::BiquadCascade cascadeA;
    cascadeA.prepare(kSampleRate);
    cascadeA.setNumStages(1);
    cascadeA.setCoefficients(0, lp);

    // Process batch
    libdsp::BiquadCascade cascadeB;
    cascadeB.prepare(kSampleRate);
    cascadeB.setNumStages(1);
    cascadeB.setCoefficients(0, lp);

    const int N = 256;
    std::vector<float> bufferA(N), bufferB(N);
    for (int i = 0; i < N; ++i) {
        bufferA[i] = std::sin(2.0f * static_cast<float>(kPi) * 440.0f * static_cast<float>(i) / kSampleRate);
        bufferB[i] = bufferA[i];
    }

    // Sample by sample
    for (int i = 0; i < N; ++i)
        bufferA[i] = cascadeA.process(bufferA[i]);

    // Batch
    cascadeB.processBatch(bufferB.data(), N);

    for (int i = 0; i < N; ++i)
        REQUIRE_THAT(bufferA[i], WithinAbs(bufferB[i], 1e-6f));
}

// ---- EQCoeffs tests ----

TEST_CASE("EQCoeffs makePeaking boosts at center frequency", "[EQCoeffs]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto peak = libdsp::EQCoeffs::makePeaking(1000.0f, 12.0f, 1.0f, kSampleRate);
    cascade.setCoefficients(0, peak);

    float magCenter = measureMagnitude(cascade, 1000.0f, kSampleRate);
    float magFar = measureMagnitude(cascade, 100.0f, kSampleRate);

    // Center should be boosted (~4x for 12dB)
    REQUIRE(magCenter > 3.0f);
    // Far from center should be near unity
    REQUIRE_THAT(magFar, WithinAbs(1.0f, 0.2f));
}

TEST_CASE("EQCoeffs makePeaking cuts at center frequency", "[EQCoeffs]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto peak = libdsp::EQCoeffs::makePeaking(1000.0f, -12.0f, 1.0f, kSampleRate);
    cascade.setCoefficients(0, peak);

    float magCenter = measureMagnitude(cascade, 1000.0f, kSampleRate);

    // Center should be cut (~0.25 for -12dB)
    REQUIRE(magCenter < 0.35f);
}

TEST_CASE("EQCoeffs makeNotch removes center frequency", "[EQCoeffs]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto notch = libdsp::EQCoeffs::makeNotch(1000.0f, 10.0f, kSampleRate);
    cascade.setCoefficients(0, notch);

    float magCenter = measureMagnitude(cascade, 1000.0f, kSampleRate);
    float magFar = measureMagnitude(cascade, 5000.0f, kSampleRate);

    REQUIRE(magCenter < 0.1f);
    REQUIRE(magFar > 0.9f);
}

TEST_CASE("EQCoeffs makeAllpass preserves magnitude", "[EQCoeffs]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto ap = libdsp::EQCoeffs::makeAllpass(1000.0f, 0.707f, kSampleRate);
    cascade.setCoefficients(0, ap);

    // All frequencies should have unity magnitude
    REQUIRE_THAT(measureMagnitude(cascade, 200.0f, kSampleRate), WithinAbs(1.0f, 0.05f));
    REQUIRE_THAT(measureMagnitude(cascade, 1000.0f, kSampleRate), WithinAbs(1.0f, 0.05f));
    REQUIRE_THAT(measureMagnitude(cascade, 5000.0f, kSampleRate), WithinAbs(1.0f, 0.05f));
}

TEST_CASE("EQCoeffs makeLowShelf boosts low frequencies", "[EQCoeffs]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto ls = libdsp::EQCoeffs::makeLowShelf(1000.0f, 6.0f, 0.707f, kSampleRate);
    cascade.setCoefficients(0, ls);

    float magLow = measureMagnitude(cascade, 100.0f, kSampleRate);
    float magHigh = measureMagnitude(cascade, 10000.0f, kSampleRate);

    // Low should be boosted (~2x for 6dB)
    REQUIRE(magLow > 1.8f);
    // High should be near unity
    REQUIRE_THAT(magHigh, WithinAbs(1.0f, 0.15f));
}

TEST_CASE("EQCoeffs makeHighShelf boosts high frequencies", "[EQCoeffs]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto hs = libdsp::EQCoeffs::makeHighShelf(1000.0f, 6.0f, 0.707f, kSampleRate);
    cascade.setCoefficients(0, hs);

    float magLow = measureMagnitude(cascade, 100.0f, kSampleRate);
    float magHigh = measureMagnitude(cascade, 10000.0f, kSampleRate);

    // Low should be near unity
    REQUIRE_THAT(magLow, WithinAbs(1.0f, 0.15f));
    // High should be boosted
    REQUIRE(magHigh > 1.8f);
}

TEST_CASE("EQCoeffs makeBandpass passes center frequency", "[EQCoeffs]")
{
    libdsp::BiquadCascade cascade;
    cascade.prepare(kSampleRate);
    cascade.setNumStages(1);

    auto bp = libdsp::EQCoeffs::makeBandpass(1000.0f, 1.0f, kSampleRate);
    cascade.setCoefficients(0, bp);

    float magCenter = measureMagnitude(cascade, 1000.0f, kSampleRate);
    float magFar = measureMagnitude(cascade, 10000.0f, kSampleRate);

    // Center should pass (near unity for Q=1)
    REQUIRE(magCenter > 0.6f);
    // Far frequencies should be attenuated
    REQUIRE(magFar < magCenter);
}

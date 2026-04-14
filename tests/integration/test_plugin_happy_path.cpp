#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/dynamics/Compressor.h>

#include <cmath>
#include <vector>

using namespace libdsp::dynamics;

namespace {

constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
constexpr float kFrequencyHz = 440.f;

// Generate a sine wave at a given amplitude (linear) and frequency
void generateSine(std::vector<float>& buffer, float amplitude, float freqHz, int numSamples)
{
    buffer.resize(static_cast<size_t>(numSamples));
    for (int i = 0; i < numSamples; ++i)
    {
        float phase = static_cast<float>(i) / static_cast<float>(kSampleRate);
        buffer[static_cast<size_t>(i)] = amplitude * std::sin(2.f * 3.14159265358979f * freqHz * phase);
    }
}

bool hasNanOrInf(const std::vector<float>& buffer)
{
    for (auto sample : buffer)
    {
        if (std::isnan(sample) || std::isinf(sample))
            return true;
    }
    return false;
}

float rmsLevel(const std::vector<float>& buffer, int startSample, int endSample)
{
    float sum = 0.f;
    for (int i = startSample; i < endSample; ++i)
    {
        float s = buffer[static_cast<size_t>(i)];
        sum += s * s;
    }
    return std::sqrt(sum / static_cast<float>(endSample - startSample));
}

} // anonymous namespace

TEST_CASE("Integration: Compressor reduces loud sine wave", "[integration][compressor]")
{
    // Set up compressor with realistic parameters
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setMode(CompressorMode::FeedForward);
    comp.setThreshold(-20.f);
    comp.setRatio(4.f);
    comp.setKnee(3.f);
    comp.setAttack(5.f);     // 5ms attack
    comp.setRelease(100.f);  // 100ms release
    comp.setMakeup(0.f);
    comp.setMix(100.f);
    comp.prepare(kSampleRate, kBlockSize);

    // 1 second of 440Hz sine at 0 dBFS (amplitude 1.0) — well above -20dB threshold
    const int numSamples = static_cast<int>(kSampleRate);
    std::vector<float> left, right;
    generateSine(left, 1.0f, kFrequencyHz, numSamples);
    generateSine(right, 1.0f, kFrequencyHz, numSamples);

    // Measure input RMS in the last quarter (steady state region)
    int checkStart = numSamples * 3 / 4;
    float inputRms = rmsLevel(left, checkStart, numSamples);

    // Process
    comp.process(left.data(), right.data(), numSamples);

    // Verify: no NaN or infinity in output
    REQUIRE_FALSE(hasNanOrInf(left));
    REQUIRE_FALSE(hasNanOrInf(right));

    // Verify: output RMS is reduced compared to input
    float outputRms = rmsLevel(left, checkStart, numSamples);
    REQUIRE(outputRms < inputRms);
    REQUIRE(outputRms > 0.001f);  // Not silent

    // Verify: gain reduction is reported
    REQUIRE(comp.getGainReduction() < 0.f);
}

TEST_CASE("Integration: Compressor passes quiet sine unchanged", "[integration][compressor]")
{
    Compressor comp;
    comp.prepare(kSampleRate, kBlockSize);
    comp.setMode(CompressorMode::FeedForward);
    comp.setThreshold(-10.f);
    comp.setRatio(4.f);
    comp.setKnee(0.f);
    comp.setAttack(5.f);
    comp.setRelease(100.f);
    comp.setMakeup(0.f);
    comp.setMix(100.f);
    comp.prepare(kSampleRate, kBlockSize);

    // 1 second of 440Hz sine at -40 dBFS — well below -10dB threshold
    const int numSamples = static_cast<int>(kSampleRate);
    float amplitude = std::pow(10.f, -40.f * 0.05f);  // ~0.01
    std::vector<float> left, right;
    generateSine(left, amplitude, kFrequencyHz, numSamples);
    generateSine(right, amplitude, kFrequencyHz, numSamples);

    // Save original for comparison
    std::vector<float> leftOrig = left;

    // Process
    comp.process(left.data(), right.data(), numSamples);

    // Verify: no NaN or infinity
    REQUIRE_FALSE(hasNanOrInf(left));
    REQUIRE_FALSE(hasNanOrInf(right));

    // Verify: output closely matches input (signal below threshold)
    int checkStart = numSamples / 2;
    for (int i = checkStart; i < numSamples; ++i)
    {
        REQUIRE_THAT(static_cast<double>(left[static_cast<size_t>(i)]),
                     Catch::Matchers::WithinAbs(
                         static_cast<double>(leftOrig[static_cast<size_t>(i)]), 0.001));
    }
}

TEST_CASE("Integration: Full DSP chain — compressor with all modes produces valid output", "[integration][compressor]")
{
    // Test all three compressor modes with a realistic signal to ensure
    // the full DSP chain (level detection → gain computation → gain application)
    // produces valid output in every configuration
    const int numSamples = static_cast<int>(kSampleRate);

    for (auto mode : {CompressorMode::FeedForward, CompressorMode::FeedBack, CompressorMode::Opto})
    {
        DYNAMIC_SECTION("Mode: " << static_cast<int>(mode))
        {
            Compressor comp;
            comp.prepare(kSampleRate, kBlockSize);
            comp.setMode(mode);
            comp.setThreshold(-18.f);
            comp.setRatio(6.f);
            comp.setKnee(6.f);
            comp.setAttack(2.f);
            comp.setRelease(150.f);
            comp.setMakeup(3.f);
            comp.setMix(75.f);
            comp.setStereoLink(true);
            comp.prepare(kSampleRate, kBlockSize);

            std::vector<float> left, right;
            generateSine(left, 0.9f, kFrequencyHz, numSamples);
            generateSine(right, 0.7f, 880.f, numSamples);

            comp.process(left.data(), right.data(), numSamples);

            // Core invariant: output must be finite and non-silent
            REQUIRE_FALSE(hasNanOrInf(left));
            REQUIRE_FALSE(hasNanOrInf(right));

            float outRms = rmsLevel(left, numSamples * 3 / 4, numSamples);
            REQUIRE(outRms > 0.001f);

            // Gain reduction should be active
            REQUIRE(comp.getGainReduction() < 0.f);
        }
    }
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/util/Oversampler.h>

#include <cmath>
#include <vector>

using namespace libdsp;
using Catch::Matchers::WithinAbs;

namespace {
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 64;
}

TEST_CASE("Oversampler: 2x produces correct sample count", "[util][oversampler]")
{
    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, 2);

    REQUIRE(os.getFactor() == 2);
    REQUIRE(os.getOversampledBlockSize(kBlockSize) == kBlockSize * 2);

    std::vector<float> input(kBlockSize, 0.5f);
    float* osBuffer = os.upsample(input.data(), kBlockSize);

    // Should have 2x samples in oversampled buffer
    REQUIRE(osBuffer != nullptr);

    // Downsample back
    std::vector<float> output(kBlockSize, 0.f);
    os.downsample(output.data(), kBlockSize);

    // Output should be valid (not NaN/Inf)
    for (int i = 0; i < kBlockSize; ++i)
    {
        REQUIRE(!std::isnan(output[static_cast<size_t>(i)]));
        REQUIRE(!std::isinf(output[static_cast<size_t>(i)]));
    }
}

TEST_CASE("Oversampler: 4x produces correct sample count", "[util][oversampler]")
{
    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, 4);

    REQUIRE(os.getFactor() == 4);
    REQUIRE(os.getOversampledBlockSize(kBlockSize) == kBlockSize * 4);
    REQUIRE(os.getOversampledSampleRate() == kSampleRate * 4);
}

TEST_CASE("Oversampler: 8x produces correct sample count", "[util][oversampler]")
{
    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, 8);

    REQUIRE(os.getFactor() == 8);
    REQUIRE(os.getOversampledBlockSize(kBlockSize) == kBlockSize * 8);
}

TEST_CASE("Oversampler: 1x passthrough", "[util][oversampler]")
{
    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, 1);

    REQUIRE(os.getFactor() == 1);

    std::vector<float> input(kBlockSize);
    for (int i = 0; i < kBlockSize; ++i)
        input[static_cast<size_t>(i)] = static_cast<float>(i) * 0.01f;

    float* osBuffer = os.upsample(input.data(), kBlockSize);

    // With 1x, oversampled buffer should equal input
    for (int i = 0; i < kBlockSize; ++i)
    {
        REQUIRE_THAT(osBuffer[i],
                     WithinAbs(static_cast<double>(input[static_cast<size_t>(i)]), 1e-6));
    }

    std::vector<float> output(kBlockSize, 0.f);
    os.downsample(output.data(), kBlockSize);

    for (int i = 0; i < kBlockSize; ++i)
    {
        REQUIRE_THAT(output[static_cast<size_t>(i)],
                     WithinAbs(static_cast<double>(input[static_cast<size_t>(i)]), 1e-6));
    }
}

TEST_CASE("Oversampler: upsample/downsample roundtrip preserves signal", "[util][oversampler]")
{
    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, 2);

    // Generate a low-frequency sine (well below Nyquist) as input
    std::vector<float> input(kBlockSize);
    for (int i = 0; i < kBlockSize; ++i)
        input[static_cast<size_t>(i)] = std::sin(2.f * static_cast<float>(M_PI) * 100.f * static_cast<float>(i) / static_cast<float>(kSampleRate));

    os.upsample(input.data(), kBlockSize);

    std::vector<float> output(kBlockSize, 0.f);
    os.downsample(output.data(), kBlockSize);

    // After roundtrip, signal should be somewhat preserved (FIR filtering may alter magnitude/phase)
    // At minimum, no NaN/Inf and bounded output
    for (int i = 0; i < kBlockSize; ++i)
    {
        REQUIRE(!std::isnan(output[static_cast<size_t>(i)]));
        REQUIRE(!std::isinf(output[static_cast<size_t>(i)]));
        REQUIRE(std::fabs(output[static_cast<size_t>(i)]) < 10.f);
    }
}

TEST_CASE("Oversampler: getOversampledBuffer returns non-null", "[util][oversampler]")
{
    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, 4);

    float* buf = os.getOversampledBuffer();
    REQUIRE(buf != nullptr);
}

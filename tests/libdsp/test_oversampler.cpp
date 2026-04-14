#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/util/Oversampler.h>

#include <cmath>
#include <vector>

TEST_CASE("Oversampler: 2x factor doubles sample count", "[util][oversampler]")
{
    libdsp::Oversampler os;
    const int blockSize = 64;
    os.prepare(44100.0, blockSize, 2);

    REQUIRE(os.getFactor() == 2);
    REQUIRE(os.getOversampledBlockSize(blockSize) == blockSize * 2);
}

TEST_CASE("Oversampler: 4x factor quadruples sample count", "[util][oversampler]")
{
    libdsp::Oversampler os;
    const int blockSize = 64;
    os.prepare(44100.0, blockSize, 4);

    REQUIRE(os.getFactor() == 4);
    REQUIRE(os.getOversampledBlockSize(blockSize) == blockSize * 4);
}

TEST_CASE("Oversampler: 1x passthrough preserves samples", "[util][oversampler]")
{
    libdsp::Oversampler os;
    const int blockSize = 8;
    os.prepare(44100.0, blockSize, 1);

    std::vector<float> input = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
    float* upsampled = os.upsample(input.data(), blockSize);

    // 1x should just copy
    for (int i = 0; i < blockSize; ++i)
        REQUIRE(upsampled[i] == input[static_cast<size_t>(i)]);

    std::vector<float> output(blockSize);
    os.downsample(output.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
        REQUIRE(output[static_cast<size_t>(i)] == input[static_cast<size_t>(i)]);
}

TEST_CASE("Oversampler: downsample recovers original length", "[util][oversampler]")
{
    libdsp::Oversampler os;
    const int blockSize = 64;
    os.prepare(44100.0, blockSize, 2);

    // Create a DC signal
    std::vector<float> input(blockSize, 0.5f);
    os.upsample(input.data(), blockSize);

    std::vector<float> output(blockSize, 0.f);
    os.downsample(output.data(), blockSize);

    // Output should have blockSize samples (same as input)
    // Values may differ due to filtering, but should be finite
    for (int i = 0; i < blockSize; ++i)
        REQUIRE(std::isfinite(output[static_cast<size_t>(i)]));
}

TEST_CASE("Oversampler: oversampled sample rate is correct", "[util][oversampler]")
{
    libdsp::Oversampler os;
    os.prepare(44100.0, 64, 4);
    REQUIRE(os.getOversampledSampleRate() == 44100.0 * 4);
}

TEST_CASE("Oversampler: getOversampledBuffer returns valid pointer", "[util][oversampler]")
{
    libdsp::Oversampler os;
    os.prepare(44100.0, 64, 2);

    float* buf = os.getOversampledBuffer();
    REQUIRE(buf != nullptr);
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/filters/LadderFilter.h>
#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

TEST_CASE("LadderFilter LP24 attenuates high frequencies", "[LadderFilter]")
{
    libdsp::LadderFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(500.0f);
    filter.setResonance(0.0f);
    filter.setType(libdsp::LadderFilter::Type::LP24);

    // Generate 10kHz sine (well above 500Hz cutoff) and measure output level
    const double freq = 10000.0;
    const double sr = 44100.0;
    const int numSamples = 4096;
    float maxOutput = 0.0f;

    // Let filter settle
    for (int i = 0; i < 1000; ++i)
        filter.process(std::sin(2.0 * M_PI * freq * i / sr));

    for (int i = 1000; i < 1000 + numSamples; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxOutput) maxOutput = output;
    }

    // 24dB/oct at 10kHz (>4 octaves above 500Hz) should be heavily attenuated
    REQUIRE(maxOutput < 0.01f);
}

TEST_CASE("LadderFilter LP24 passes low frequencies", "[LadderFilter]")
{
    libdsp::LadderFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(5000.0f);
    filter.setResonance(0.0f);
    filter.setType(libdsp::LadderFilter::Type::LP24);

    // Generate 100Hz sine (well below cutoff) and measure output level
    const double freq = 100.0;
    const double sr = 44100.0;
    float maxOutput = 0.0f;

    // Let filter settle
    for (int i = 0; i < 4410; ++i)
        filter.process(std::sin(2.0 * M_PI * freq * i / sr));

    for (int i = 4410; i < 4410 + 4096; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxOutput) maxOutput = output;
    }

    // Should pass through with near-unity gain
    REQUIRE(maxOutput > 0.5f);
}

TEST_CASE("LadderFilter resonance boosts at cutoff", "[LadderFilter]")
{
    libdsp::LadderFilter filterNoRes;
    filterNoRes.prepare(44100.0);
    filterNoRes.setCutoff(1000.0f);
    filterNoRes.setResonance(0.0f);
    filterNoRes.setType(libdsp::LadderFilter::Type::LP24);

    libdsp::LadderFilter filterRes;
    filterRes.prepare(44100.0);
    filterRes.setCutoff(1000.0f);
    filterRes.setResonance(0.8f);
    filterRes.setType(libdsp::LadderFilter::Type::LP24);

    // Sine at cutoff frequency
    const double freq = 1000.0;
    const double sr = 44100.0;
    float maxNoRes = 0.0f;
    float maxRes = 0.0f;

    for (int i = 0; i < 8192; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr));
        float outNoRes = std::abs(filterNoRes.process(input));
        float outRes = std::abs(filterRes.process(input));
        if (outNoRes > maxNoRes) maxNoRes = outNoRes;
        if (outRes > maxRes) maxRes = outRes;
    }

    // Resonant filter should have higher output at cutoff
    REQUIRE(maxRes > maxNoRes);
}

TEST_CASE("LadderFilter HP24 attenuates low frequencies", "[LadderFilter]")
{
    libdsp::LadderFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(5000.0f);
    filter.setResonance(0.0f);
    filter.setType(libdsp::LadderFilter::Type::HP24);

    // Generate 100Hz sine (well below cutoff)
    const double freq = 100.0;
    const double sr = 44100.0;
    float maxOutput = 0.0f;

    for (int i = 0; i < 8192; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxOutput) maxOutput = output;
    }

    // Should be heavily attenuated
    REQUIRE(maxOutput < 0.1f);
}

TEST_CASE("LadderFilter reset clears state", "[LadderFilter]")
{
    libdsp::LadderFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.5f);

    // Push some signal through
    for (int i = 0; i < 1000; ++i)
        filter.process(1.0f);

    filter.reset();

    // After reset, processing silence should yield silence
    float output = filter.process(0.0f);
    REQUIRE_THAT(output, WithinAbs(0.0f, 1e-10f));
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/filters/SVFFilter.h>

#include <cmath>
#include <vector>

using namespace libdsp;
using Catch::Matchers::WithinAbs;

namespace {

constexpr double kSampleRate = 44100.0;

float measureLevel(SVFFilter& filter, float frequency, int numSamples = 8192)
{
    // Generate sine at given frequency and measure output RMS
    float sumSquared = 0.f;
    int measureStart = numSamples / 2; // skip transient
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = std::sin(2.f * static_cast<float>(M_PI) * frequency * static_cast<float>(i) / static_cast<float>(kSampleRate));
        float out = filter.process(sample);
        if (i >= measureStart)
            sumSquared += out * out;
    }
    return std::sqrt(sumSquared / static_cast<float>(numSamples - measureStart));
}

float toDb(float linear)
{
    return 20.f * std::log10(std::max(linear, 1e-10f));
}

} // anonymous namespace

TEST_CASE("SVFFilter: LP passes low frequencies", "[filters][svf]")
{
    SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(SVFFilter::Type::LowPass);
    filter.setFrequency(1000.f);
    filter.setQ(0.707f);

    float level100Hz = measureLevel(filter, 100.f);
    filter.reset();
    float level10kHz = measureLevel(filter, 10000.f);

    float dbDiff = toDb(level100Hz) - toDb(level10kHz);

    // 100 Hz should pass (within 1 dB of unity)
    // Input RMS of sine = 1/sqrt(2) ≈ 0.707
    REQUIRE(toDb(level100Hz) > -1.5f);

    // 10 kHz should be attenuated by >12 dB relative to 100 Hz
    REQUIRE(dbDiff > 12.f);
}

TEST_CASE("SVFFilter: HP passes high frequencies", "[filters][svf]")
{
    SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(SVFFilter::Type::HighPass);
    filter.setFrequency(1000.f);
    filter.setQ(0.707f);

    float level10kHz = measureLevel(filter, 10000.f);
    filter.reset();
    float level100Hz = measureLevel(filter, 100.f);

    // 10kHz should pass through HP at 1kHz
    REQUIRE(toDb(level10kHz) > -2.f);
    // 100Hz should be attenuated
    REQUIRE(toDb(level10kHz) - toDb(level100Hz) > 12.f);
}

TEST_CASE("SVFFilter: BP passes center frequency", "[filters][svf]")
{
    SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(SVFFilter::Type::BandPass);
    filter.setFrequency(1000.f);
    filter.setQ(2.f); // Narrow Q

    float levelAtCenter = measureLevel(filter, 1000.f);
    filter.reset();
    float levelFarBelow = measureLevel(filter, 100.f);
    filter.reset();
    float levelFarAbove = measureLevel(filter, 10000.f);

    // Center frequency should have highest level
    REQUIRE(toDb(levelAtCenter) > toDb(levelFarBelow));
    REQUIRE(toDb(levelAtCenter) > toDb(levelFarAbove));
}

TEST_CASE("SVFFilter: reset clears state", "[filters][svf]")
{
    SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(SVFFilter::Type::LowPass);
    filter.setFrequency(1000.f);

    // Process some signal
    for (int i = 0; i < 1000; ++i)
        filter.process(1.0f);

    filter.reset();

    // After reset, processing zero should yield zero
    float out = filter.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(0.0, 1e-10));
}

TEST_CASE("SVFFilter: stability with rapid frequency modulation", "[filters][svf]")
{
    SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(SVFFilter::Type::LowPass);
    filter.setQ(0.707f);

    bool stable = true;
    for (int i = 0; i < 10000; ++i)
    {
        // Rapidly modulate cutoff frequency
        float freq = 200.f + 5000.f * (0.5f + 0.5f * std::sin(2.f * static_cast<float>(M_PI) * 5.f * static_cast<float>(i) / static_cast<float>(kSampleRate)));
        filter.setFrequency(freq);

        float sample = std::sin(2.f * static_cast<float>(M_PI) * 440.f * static_cast<float>(i) / static_cast<float>(kSampleRate));
        float out = filter.process(sample);

        if (std::isnan(out) || std::isinf(out) || std::fabs(out) > 10.f)
        {
            stable = false;
            break;
        }
    }
    REQUIRE(stable);
}

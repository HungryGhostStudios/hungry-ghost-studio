#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/filters/SVFFilter.h>

#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

static constexpr double kSampleRate = 44100.0;

// Helper: measure RMS of filter output for a given sine frequency
static float measureFilterResponse(libdsp::SVFFilter& filter, double freqHz, int numSamples = 8192)
{
    float sumSq = 0.f;
    // Skip transient
    for (int i = 0; i < 2048; ++i)
        filter.process(static_cast<float>(std::sin(2.0 * M_PI * freqHz * i / kSampleRate)));

    for (int i = 0; i < numSamples; ++i) {
        float out = filter.process(static_cast<float>(std::sin(2.0 * M_PI * freqHz * (i + 2048) / kSampleRate)));
        sumSq += out * out;
    }
    return std::sqrt(sumSq / numSamples);
}

TEST_CASE("SVFFilter: LP passes below cutoff", "[filters][svf]")
{
    libdsp::SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(libdsp::SVFFilter::Type::LowPass);
    filter.setFrequency(1000.f);
    filter.setQ(0.707f);

    // 100 Hz should pass through nearly unchanged
    float lowResponse = measureFilterResponse(filter, 100.0);
    // RMS of unit sine ≈ 0.707
    REQUIRE(lowResponse > 0.5f);
}

TEST_CASE("SVFFilter: LP attenuates above cutoff", "[filters][svf]")
{
    libdsp::SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(libdsp::SVFFilter::Type::LowPass);
    filter.setFrequency(1000.f);
    filter.setQ(0.707f);

    float lowResponse = measureFilterResponse(filter, 100.0);
    filter.reset();
    float highResponse = measureFilterResponse(filter, 10000.0);

    // 10kHz should be significantly attenuated compared to 100Hz
    REQUIRE(highResponse < lowResponse * 0.2f);
}

TEST_CASE("SVFFilter: HP attenuates low frequencies", "[filters][svf]")
{
    libdsp::SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(libdsp::SVFFilter::Type::HighPass);
    filter.setFrequency(1000.f);
    filter.setQ(0.707f);

    float lowResponse = measureFilterResponse(filter, 100.0);
    filter.reset();
    float highResponse = measureFilterResponse(filter, 10000.0);

    // Low freq should be attenuated, high freq should pass
    REQUIRE(lowResponse < highResponse * 0.2f);
}

TEST_CASE("SVFFilter: BP passes center frequency", "[filters][svf]")
{
    libdsp::SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(libdsp::SVFFilter::Type::BandPass);
    filter.setFrequency(1000.f);
    filter.setQ(2.0f);

    float centerResponse = measureFilterResponse(filter, 1000.0);
    filter.reset();
    float offResponse = measureFilterResponse(filter, 100.0);

    // Center frequency should have higher response
    REQUIRE(centerResponse > offResponse);
}

TEST_CASE("SVFFilter: Notch removes center frequency", "[filters][svf]")
{
    libdsp::SVFFilter filter;
    filter.prepare(kSampleRate);
    filter.setType(libdsp::SVFFilter::Type::Notch);
    filter.setFrequency(1000.f);
    filter.setQ(2.0f);

    float centerResponse = measureFilterResponse(filter, 1000.0);
    filter.reset();
    float offResponse = measureFilterResponse(filter, 100.0);

    // Center frequency should be attenuated relative to off-center
    REQUIRE(centerResponse < offResponse * 0.5f);
}

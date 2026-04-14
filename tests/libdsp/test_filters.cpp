#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/filters/SVFFilter.h>
#include <libdsp/filters/LadderFilter.h>
#include <libdsp/filters/BiquadCascade.h>
#include <libdsp/filters/Coefficients.h>
#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

// ============================================================================
// SVFFilter Tests
// ============================================================================

TEST_CASE("SVFFilter LP passes 100Hz sine at 1kHz cutoff within 1dB", "[SVFFilter]")
{
    libdsp::SVFFilter filter;
    filter.prepare(44100.0);
    filter.setType(libdsp::SVFFilter::Type::LowPass);
    filter.setFrequency(1000.0f);
    filter.setQ(0.707f);

    const double freq = 100.0;
    const double sr = 44100.0;
    const int settle = 4410; // 100ms settling
    const int measure = 4096;
    float maxOutput = 0.0f;

    // Let filter settle
    for (int i = 0; i < settle; ++i)
        filter.process(static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr)));

    for (int i = settle; i < settle + measure; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxOutput) maxOutput = output;
    }

    // Within 1dB of unity means linear gain between ~0.891 and ~1.122
    REQUIRE(maxOutput > 0.89f);
    REQUIRE(maxOutput < 1.13f);
}

TEST_CASE("SVFFilter LP attenuates 10kHz by more than 12dB at 1kHz cutoff", "[SVFFilter]")
{
    libdsp::SVFFilter filter;
    filter.prepare(44100.0);
    filter.setType(libdsp::SVFFilter::Type::LowPass);
    filter.setFrequency(1000.0f);
    filter.setQ(0.707f);

    const double freq = 10000.0;
    const double sr = 44100.0;
    float maxOutput = 0.0f;

    // Settle
    for (int i = 0; i < 2000; ++i)
        filter.process(static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr)));

    for (int i = 2000; i < 6000; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxOutput) maxOutput = output;
    }

    // 12dB attenuation = linear gain < 0.25
    REQUIRE(maxOutput < 0.25f);
}

TEST_CASE("SVFFilter HP passes high frequencies and attenuates low", "[SVFFilter]")
{
    libdsp::SVFFilter filter;
    filter.prepare(44100.0);
    filter.setType(libdsp::SVFFilter::Type::HighPass);
    filter.setFrequency(1000.0f);
    filter.setQ(0.707f);

    const double sr = 44100.0;

    // Test that 100Hz is attenuated
    float maxLow = 0.0f;
    for (int i = 0; i < 8192; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * 100.0 * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxLow) maxLow = output;
    }
    REQUIRE(maxLow < 0.25f);

    // Test that 10kHz passes through
    filter.reset();
    float maxHigh = 0.0f;
    for (int i = 0; i < 4410; ++i)
        filter.process(static_cast<float>(std::sin(2.0 * M_PI * 10000.0 * i / sr)));
    for (int i = 4410; i < 8192; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * 10000.0 * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxHigh) maxHigh = output;
    }
    REQUIRE(maxHigh > 0.5f);
}

TEST_CASE("SVFFilter BandPass passes at center frequency", "[SVFFilter]")
{
    libdsp::SVFFilter filter;
    filter.prepare(44100.0);
    filter.setType(libdsp::SVFFilter::Type::BandPass);
    filter.setFrequency(1000.0f);
    filter.setQ(2.0f);

    const double sr = 44100.0;
    float maxAtCenter = 0.0f;
    float maxOffCenter = 0.0f;

    // Measure at 1kHz (center)
    for (int i = 0; i < 4410; ++i)
        filter.process(static_cast<float>(std::sin(2.0 * M_PI * 1000.0 * i / sr)));
    for (int i = 4410; i < 8192; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * 1000.0 * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxAtCenter) maxAtCenter = output;
    }

    // Measure at 100Hz (off-center)
    filter.reset();
    for (int i = 0; i < 4410; ++i)
        filter.process(static_cast<float>(std::sin(2.0 * M_PI * 100.0 * i / sr)));
    for (int i = 4410; i < 8192; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * 100.0 * i / sr));
        float output = std::abs(filter.process(input));
        if (output > maxOffCenter) maxOffCenter = output;
    }

    REQUIRE(maxAtCenter > maxOffCenter * 2.0f);
}

TEST_CASE("SVFFilter reset clears state", "[SVFFilter]")
{
    libdsp::SVFFilter filter;
    filter.prepare(44100.0);
    filter.setFrequency(1000.0f);

    // Push signal through
    for (int i = 0; i < 1000; ++i)
        filter.process(1.0f);

    filter.reset();

    // After reset, processing silence should yield silence
    float output = filter.process(0.0f);
    REQUIRE_THAT(output, WithinAbs(0.0f, 1e-10f));
}

TEST_CASE("SVFFilter stability with rapid frequency modulation", "[SVFFilter]")
{
    libdsp::SVFFilter filter;
    filter.prepare(44100.0);
    filter.setType(libdsp::SVFFilter::Type::LowPass);
    filter.setQ(0.707f);

    const double sr = 44100.0;
    bool stable = true;

    // Rapidly modulate cutoff frequency while processing signal
    for (int i = 0; i < 44100; ++i) {
        // Sweep frequency from 100 to 10000 Hz
        float freq = 100.0f + 9900.0f * static_cast<float>(i) / 44100.0f;
        filter.setFrequency(freq);

        float input = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * i / sr));
        float output = filter.process(input);

        if (std::isnan(output) || std::isinf(output) || std::abs(output) > 10.0f) {
            stable = false;
            break;
        }
    }

    REQUIRE(stable);
}

// ============================================================================
// LadderFilter Tests
// ============================================================================

TEST_CASE("LadderFilter LP24 attenuates above cutoff at approx 24dB/oct", "[LadderFilter]")
{
    libdsp::LadderFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.0f);
    filter.setType(libdsp::LadderFilter::Type::LP24);

    const double sr = 44100.0;

    // Measure at 2kHz (1 octave above cutoff)
    float max2k = 0.0f;
    for (int i = 0; i < 2000; ++i)
        filter.process(static_cast<float>(std::sin(2.0 * M_PI * 2000.0 * i / sr)));
    for (int i = 2000; i < 6000; ++i) {
        float out = std::abs(filter.process(static_cast<float>(std::sin(2.0 * M_PI * 2000.0 * i / sr))));
        if (out > max2k) max2k = out;
    }

    // Measure at 4kHz (2 octaves above cutoff)
    filter.reset();
    float max4k = 0.0f;
    for (int i = 0; i < 2000; ++i)
        filter.process(static_cast<float>(std::sin(2.0 * M_PI * 4000.0 * i / sr)));
    for (int i = 2000; i < 6000; ++i) {
        float out = std::abs(filter.process(static_cast<float>(std::sin(2.0 * M_PI * 4000.0 * i / sr))));
        if (out > max4k) max4k = out;
    }

    // At ~24dB/oct, 4kHz should be ~24dB lower than 2kHz (i.e., ratio > 10x)
    // Use conservative bound: 4kHz should be significantly less than 2kHz
    REQUIRE(max4k < max2k * 0.2f);
}

TEST_CASE("LadderFilter stability with rapid cutoff modulation", "[LadderFilter]")
{
    libdsp::LadderFilter filter;
    filter.prepare(44100.0);
    filter.setResonance(0.7f);
    filter.setType(libdsp::LadderFilter::Type::LP24);

    const double sr = 44100.0;
    bool stable = true;

    for (int i = 0; i < 44100; ++i) {
        float cutoff = 200.0f + 10000.0f * (0.5f + 0.5f * std::sin(2.0 * M_PI * 5.0 * i / sr));
        filter.setCutoff(cutoff);

        float input = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * i / sr));
        float output = filter.process(input);

        if (std::isnan(output) || std::isinf(output) || std::abs(output) > 20.0f) {
            stable = false;
            break;
        }
    }

    REQUIRE(stable);
}

// ============================================================================
// BiquadCascade Tests
// ============================================================================

TEST_CASE("BiquadCascade peaking EQ boosts target frequency", "[BiquadCascade]")
{
    libdsp::BiquadCascade bq;
    bq.prepare(44100.0);
    bq.setNumStages(1);
    bq.setCoefficients(0, libdsp::EQCoeffs::makePeaking(1000.0f, 12.0f, 1.0f, 44100.0f));

    const double sr = 44100.0;

    // Measure at 1kHz (boosted)
    float maxBoosted = 0.0f;
    for (int i = 0; i < 4096; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * 1000.0 * i / sr));
        float out = std::abs(bq.process(input));
        if (out > maxBoosted) maxBoosted = out;
    }

    // Measure at 100Hz (not boosted)
    bq.reset();
    bq.setCoefficients(0, libdsp::EQCoeffs::makePeaking(1000.0f, 12.0f, 1.0f, 44100.0f));
    float maxFlat = 0.0f;
    for (int i = 0; i < 4410; ++i)
        bq.process(static_cast<float>(std::sin(2.0 * M_PI * 100.0 * i / sr)));
    for (int i = 4410; i < 8820; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * 100.0 * i / sr));
        float out = std::abs(bq.process(input));
        if (out > maxFlat) maxFlat = out;
    }

    // 12dB boost = ~4x, so target should be significantly louder
    REQUIRE(maxBoosted > maxFlat * 2.0f);
}

TEST_CASE("BiquadCascade stability with rapid coefficient changes", "[BiquadCascade]")
{
    libdsp::BiquadCascade bq;
    bq.prepare(44100.0);
    bq.setNumStages(1);

    const double sr = 44100.0;
    bool stable = true;

    for (int i = 0; i < 44100; ++i) {
        // Sweep frequency
        float freq = 200.0f + 8000.0f * static_cast<float>(i) / 44100.0f;
        bq.setCoefficients(0, libdsp::EQCoeffs::makePeaking(freq, 6.0f, 1.0f, 44100.0f));

        float input = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * i / sr));
        float output = bq.process(input);

        if (std::isnan(output) || std::isinf(output) || std::abs(output) > 50.0f) {
            stable = false;
            break;
        }
    }

    REQUIRE(stable);
}

#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/filters/BiquadCascade.h>
#include <libdsp/filters/Coefficients.h>
#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("EQCoeffs makeLP attenuates high frequencies", "[Coefficients]")
{
    libdsp::BiquadCascade bq;
    bq.prepare(44100.0);
    bq.setNumStages(1);
    bq.setCoefficients(0, libdsp::EQCoeffs::makeLP(1000.0f, 0.707f, 44100.0f));

    // Feed 10kHz sine and measure output
    const double freq = 10000.0;
    const double sr = 44100.0;
    float maxOutput = 0.0f;

    for (int i = 0; i < 2000; ++i)
        bq.process(static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr)));

    for (int i = 2000; i < 4000; ++i) {
        float out = std::abs(bq.process(static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr))));
        if (out > maxOutput) maxOutput = out;
    }

    REQUIRE(maxOutput < 0.1f);
}

TEST_CASE("EQCoeffs makeHP attenuates low frequencies", "[Coefficients]")
{
    libdsp::BiquadCascade bq;
    bq.prepare(44100.0);
    bq.setNumStages(1);
    bq.setCoefficients(0, libdsp::EQCoeffs::makeHP(5000.0f, 0.707f, 44100.0f));

    // Feed 100Hz sine
    const double freq = 100.0;
    const double sr = 44100.0;
    float maxOutput = 0.0f;

    for (int i = 0; i < 4410; ++i)
        bq.process(static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr)));

    for (int i = 4410; i < 8820; ++i) {
        float out = std::abs(bq.process(static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr))));
        if (out > maxOutput) maxOutput = out;
    }

    REQUIRE(maxOutput < 0.05f);
}

TEST_CASE("EQCoeffs makePeaking boosts at center frequency", "[Coefficients]")
{
    // Flat reference
    libdsp::BiquadCascade flat;
    flat.prepare(44100.0);
    flat.setNumStages(1);
    flat.setCoefficients(0, libdsp::EQCoeffs::makePeaking(1000.0f, 0.0f, 1.0f, 44100.0f));

    // +12dB peaking
    libdsp::BiquadCascade boosted;
    boosted.prepare(44100.0);
    boosted.setNumStages(1);
    boosted.setCoefficients(0, libdsp::EQCoeffs::makePeaking(1000.0f, 12.0f, 1.0f, 44100.0f));

    const double freq = 1000.0;
    const double sr = 44100.0;
    float maxFlat = 0.0f, maxBoosted = 0.0f;

    for (int i = 0; i < 4096; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr));
        float outFlat = std::abs(flat.process(input));
        float outBoosted = std::abs(boosted.process(input));
        if (outFlat > maxFlat) maxFlat = outFlat;
        if (outBoosted > maxBoosted) maxBoosted = outBoosted;
    }

    REQUIRE(maxBoosted > maxFlat * 2.0f); // 12dB is ~4x linear
}

TEST_CASE("EQCoeffs makeNotch removes center frequency", "[Coefficients]")
{
    libdsp::BiquadCascade bq;
    bq.prepare(44100.0);
    bq.setNumStages(1);
    bq.setCoefficients(0, libdsp::EQCoeffs::makeNotch(1000.0f, 10.0f, 44100.0f));

    const double freq = 1000.0;
    const double sr = 44100.0;
    float maxOutput = 0.0f;

    // Let settle
    for (int i = 0; i < 4096; ++i)
        bq.process(static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr)));

    for (int i = 4096; i < 8192; ++i) {
        float out = std::abs(bq.process(static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr))));
        if (out > maxOutput) maxOutput = out;
    }

    REQUIRE(maxOutput < 0.05f);
}

TEST_CASE("BiquadCascade multi-stage increases slope", "[BiquadCascade]")
{
    // Single LP stage
    libdsp::BiquadCascade single;
    single.prepare(44100.0);
    single.setNumStages(1);
    single.setCoefficients(0, libdsp::EQCoeffs::makeLP(1000.0f, 0.707f, 44100.0f));

    // Two LP stages cascaded (4th order)
    libdsp::BiquadCascade cascade;
    cascade.prepare(44100.0);
    cascade.setNumStages(2);
    cascade.setCoefficients(0, libdsp::EQCoeffs::makeLP(1000.0f, 0.707f, 44100.0f));
    cascade.setCoefficients(1, libdsp::EQCoeffs::makeLP(1000.0f, 0.707f, 44100.0f));

    const double freq = 8000.0;
    const double sr = 44100.0;
    float maxSingle = 0.0f, maxCascade = 0.0f;

    for (int i = 0; i < 4096; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr));
        float out1 = std::abs(single.process(input));
        float out2 = std::abs(cascade.process(input));
        if (out1 > maxSingle) maxSingle = out1;
        if (out2 > maxCascade) maxCascade = out2;
    }

    // Cascade should have more attenuation
    REQUIRE(maxCascade < maxSingle);
}

TEST_CASE("BiquadCascade reset clears state", "[BiquadCascade]")
{
    libdsp::BiquadCascade bq;
    bq.prepare(44100.0);
    bq.setNumStages(1);
    bq.setCoefficients(0, libdsp::EQCoeffs::makeLP(1000.0f, 0.707f, 44100.0f));

    for (int i = 0; i < 1000; ++i)
        bq.process(1.0f);

    bq.reset();
    float output = bq.process(0.0f);
    REQUIRE_THAT(output, WithinAbs(0.0f, 1e-10f));
}

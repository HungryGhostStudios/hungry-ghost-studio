#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/filters/BiquadCascade.h>
#include <libdsp/filters/Coefficients.h>
#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

static constexpr double kSampleRate = 48000.0;

// Helper: generate a sine wave buffer
static std::vector<float> generateSine(double freq, double sampleRate, int numSamples) {
    std::vector<float> buf(static_cast<size_t>(numSamples));
    for (int i = 0; i < numSamples; ++i) {
        buf[static_cast<size_t>(i)] = static_cast<float>(
            std::sin(2.0 * M_PI * freq * i / sampleRate));
    }
    return buf;
}

// Helper: measure RMS of a buffer
static float rms(const std::vector<float>& buf) {
    double sum = 0.0;
    for (auto s : buf) sum += static_cast<double>(s) * s;
    return static_cast<float>(std::sqrt(sum / static_cast<double>(buf.size())));
}

TEST_CASE("BiquadCascade default construction has zero stages", "[BiquadCascade]") {
    libdsp::BiquadCascade bq;
    REQUIRE(bq.getNumStages() == 0);
    // Pass-through when no stages
    REQUIRE_THAT(bq.processSample(0.5f), WithinAbs(0.5f, 1e-6f));
}

TEST_CASE("BiquadCascade with unity coefficients passes signal through", "[BiquadCascade]") {
    libdsp::BiquadCascade bq(2);
    // Unity: b0=1, b1=0, b2=0, a1=0, a2=0
    libdsp::BiquadCoeffs unity{1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    bq.setCoeffs(0, unity);
    bq.setCoeffs(1, unity);

    for (int i = 0; i < 100; ++i) {
        float in = static_cast<float>(std::sin(0.1 * i));
        REQUIRE_THAT(bq.processSample(in), WithinAbs(in, 1e-6f));
    }
}

TEST_CASE("EQCoeffs lowPass attenuates above cutoff", "[Coefficients]") {
    auto coeffs = libdsp::EQCoeffs::lowPass(1000.0, kSampleRate, 0.707);

    libdsp::BiquadCascade bq(1);
    bq.setCoeffs(0, coeffs);

    // Process 200Hz sine — should pass mostly through
    auto lowSine = generateSine(200.0, kSampleRate, 4096);
    std::vector<float> lowOut(lowSine.size());
    for (size_t i = 0; i < lowSine.size(); ++i)
        lowOut[i] = bq.processSample(lowSine[i]);

    // Skip transient
    float lowRms = rms(std::vector<float>(lowOut.begin() + 512, lowOut.end()));
    float lowInRms = rms(std::vector<float>(lowSine.begin() + 512, lowSine.end()));

    bq.reset();

    // Process 5000Hz sine — should be attenuated
    auto highSine = generateSine(5000.0, kSampleRate, 4096);
    std::vector<float> highOut(highSine.size());
    for (size_t i = 0; i < highSine.size(); ++i)
        highOut[i] = bq.processSample(highSine[i]);

    float highRms = rms(std::vector<float>(highOut.begin() + 512, highOut.end()));
    float highInRms = rms(std::vector<float>(highSine.begin() + 512, highSine.end()));

    float lowGain = lowRms / lowInRms;
    float highGain = highRms / highInRms;

    // Low freq should pass (~0dB), high freq should be attenuated significantly
    REQUIRE(lowGain > 0.9f);
    REQUIRE(highGain < 0.3f);
}

TEST_CASE("EQCoeffs highPass attenuates below cutoff", "[Coefficients]") {
    auto coeffs = libdsp::EQCoeffs::highPass(1000.0, kSampleRate, 0.707);

    libdsp::BiquadCascade bq(1);
    bq.setCoeffs(0, coeffs);

    // 100Hz should be attenuated
    auto lowSine = generateSine(100.0, kSampleRate, 4096);
    std::vector<float> lowOut(lowSine.size());
    for (size_t i = 0; i < lowSine.size(); ++i)
        lowOut[i] = bq.processSample(lowSine[i]);

    float lowRms = rms(std::vector<float>(lowOut.begin() + 512, lowOut.end()));
    float lowInRms = rms(std::vector<float>(lowSine.begin() + 512, lowSine.end()));

    bq.reset();

    // 5000Hz should pass
    auto highSine = generateSine(5000.0, kSampleRate, 4096);
    std::vector<float> highOut(highSine.size());
    for (size_t i = 0; i < highSine.size(); ++i)
        highOut[i] = bq.processSample(highSine[i]);

    float highRms = rms(std::vector<float>(highOut.begin() + 512, highOut.end()));
    float highInRms = rms(std::vector<float>(highSine.begin() + 512, highSine.end()));

    REQUIRE(lowRms / lowInRms < 0.15f);
    REQUIRE(highRms / highInRms > 0.9f);
}

TEST_CASE("EQCoeffs peakingEQ boosts at center frequency", "[Coefficients]") {
    auto coeffs = libdsp::EQCoeffs::peakingEQ(1000.0, kSampleRate, 12.0, 2.0);

    libdsp::BiquadCascade bq(1);
    bq.setCoeffs(0, coeffs);

    // 1000Hz should be boosted
    auto sine1k = generateSine(1000.0, kSampleRate, 4096);
    std::vector<float> out1k(sine1k.size());
    for (size_t i = 0; i < sine1k.size(); ++i)
        out1k[i] = bq.processSample(sine1k[i]);

    float outRms = rms(std::vector<float>(out1k.begin() + 512, out1k.end()));
    float inRms = rms(std::vector<float>(sine1k.begin() + 512, sine1k.end()));

    // 12dB boost means ~4x gain
    float gain = outRms / inRms;
    REQUIRE(gain > 3.0f);
    REQUIRE(gain < 5.0f);
}

TEST_CASE("EQCoeffs notch attenuates at center frequency", "[Coefficients]") {
    auto coeffs = libdsp::EQCoeffs::notch(1000.0, kSampleRate, 10.0);

    libdsp::BiquadCascade bq(1);
    bq.setCoeffs(0, coeffs);

    auto sine1k = generateSine(1000.0, kSampleRate, 8192);
    std::vector<float> out(sine1k.size());
    for (size_t i = 0; i < sine1k.size(); ++i)
        out[i] = bq.processSample(sine1k[i]);

    float outRms = rms(std::vector<float>(out.begin() + 1024, out.end()));
    float inRms = rms(std::vector<float>(sine1k.begin() + 1024, sine1k.end()));

    // Notch should heavily attenuate the center frequency
    REQUIRE(outRms / inRms < 0.05f);
}

TEST_CASE("BiquadCascade reset clears state", "[BiquadCascade]") {
    auto coeffs = libdsp::EQCoeffs::lowPass(1000.0, kSampleRate, 0.707);
    libdsp::BiquadCascade bq(1);
    bq.setCoeffs(0, coeffs);

    // Process some samples to build up state
    for (int i = 0; i < 100; ++i)
        bq.processSample(1.0f);

    bq.reset();

    // After reset, processing 0 should yield 0
    REQUIRE_THAT(bq.processSample(0.0f), WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("BiquadCascade processBatch matches sample-by-sample", "[BiquadCascade]") {
    auto coeffs = libdsp::EQCoeffs::peakingEQ(2000.0, kSampleRate, 6.0, 1.0);

    libdsp::BiquadCascade bq1(1);
    bq1.setCoeffs(0, coeffs);

    libdsp::BiquadCascade bq2(1);
    bq2.setCoeffs(0, coeffs);

    auto sine = generateSine(2000.0, kSampleRate, 256);
    std::vector<float> batchOut(sine);

    // Sample-by-sample
    std::vector<float> sampleOut(sine.size());
    for (size_t i = 0; i < sine.size(); ++i)
        sampleOut[i] = bq1.processSample(sine[i]);

    // Batch
    bq2.processBatch(batchOut.data(), static_cast<int>(batchOut.size()));

    for (size_t i = 0; i < sine.size(); ++i)
        REQUIRE_THAT(batchOut[i], WithinAbs(sampleOut[i], 1e-6f));
}

TEST_CASE("EQCoeffs allPass has unity magnitude", "[Coefficients]") {
    auto coeffs = libdsp::EQCoeffs::allPass(1000.0, kSampleRate, 0.707);

    libdsp::BiquadCascade bq(1);
    bq.setCoeffs(0, coeffs);

    auto sine = generateSine(1000.0, kSampleRate, 8192);
    std::vector<float> out(sine.size());
    for (size_t i = 0; i < sine.size(); ++i)
        out[i] = bq.processSample(sine[i]);

    float outRms = rms(std::vector<float>(out.begin() + 1024, out.end()));
    float inRms = rms(std::vector<float>(sine.begin() + 1024, sine.end()));

    // All-pass should have unity gain
    REQUIRE_THAT(outRms / inRms, WithinAbs(1.0f, 0.01f));
}

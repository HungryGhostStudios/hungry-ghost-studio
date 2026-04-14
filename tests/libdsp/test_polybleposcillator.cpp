#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/synthesis/PolyBlepOscillator.h>

#include <cmath>
#include <vector>
#include <numeric>

using Catch::Matchers::WithinAbs;

namespace {

constexpr double kSampleRate = 44100.0;

// Compute RMS of a signal
float rms(const std::vector<float>& signal) {
    if (signal.empty()) return 0.0f;
    float sum = 0.0f;
    for (float s : signal) {
        sum += s * s;
    }
    return std::sqrt(sum / static_cast<float>(signal.size()));
}

// Generate N samples from an oscillator
std::vector<float> generateSamples(libdsp::PolyBlepOscillator& osc, int numSamples) {
    std::vector<float> output(static_cast<size_t>(numSamples));
    for (int i = 0; i < numSamples; ++i) {
        output[static_cast<size_t>(i)] = osc.process();
    }
    return output;
}

} // namespace

TEST_CASE("PolyBlepOscillator saw produces values in [-1, 1]", "[polybleposcillator]") {
    libdsp::PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequency(440.0f);
    osc.setWaveform(libdsp::PolyBlepOscillator::Waveform::Saw);

    for (int i = 0; i < 44100; ++i) {
        float val = osc.process();
        REQUIRE(val >= -1.5f); // Slight overshoot from PolyBLEP is acceptable
        REQUIRE(val <= 1.5f);
    }
}

TEST_CASE("PolyBlepOscillator square produces values near +/-1", "[polybleposcillator]") {
    libdsp::PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequency(440.0f);
    osc.setWaveform(libdsp::PolyBlepOscillator::Waveform::Square);

    int nearPosOne = 0;
    int nearNegOne = 0;
    int total = 44100;

    for (int i = 0; i < total; ++i) {
        float val = osc.process();
        if (std::fabs(val - 1.0f) < 0.2f) nearPosOne++;
        if (std::fabs(val + 1.0f) < 0.2f) nearNegOne++;
    }

    // Most samples should be near +1 or -1
    REQUIRE(nearPosOne + nearNegOne > total * 0.9);
}

TEST_CASE("PolyBlepOscillator saw has correct fundamental frequency", "[polybleposcillator]") {
    libdsp::PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequency(1000.0f);
    osc.setWaveform(libdsp::PolyBlepOscillator::Waveform::Saw);

    // Count zero-crossings over 1 second
    int zeroCrossings = 0;
    float prev = osc.process();
    for (int i = 1; i < 44100; ++i) {
        float curr = osc.process();
        if ((prev > 0.0f && curr <= 0.0f) || (prev < 0.0f && curr >= 0.0f)) {
            zeroCrossings++;
        }
        prev = curr;
    }

    // Saw has 2 zero crossings per cycle (but one is the discontinuity)
    // At 1000 Hz, expect ~1000 positive-to-negative crossings
    // Allow some tolerance
    REQUIRE(zeroCrossings >= 900);
    REQUIRE(zeroCrossings <= 2200);
}

TEST_CASE("PolyBlepOscillator triangle output is bounded", "[polybleposcillator]") {
    libdsp::PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequency(440.0f);
    osc.setWaveform(libdsp::PolyBlepOscillator::Waveform::Triangle);

    // Let the integrator settle
    for (int i = 0; i < 44100; ++i) {
        osc.process();
    }

    // After settling, check amplitude is reasonable
    float maxVal = 0.0f;
    for (int i = 0; i < 44100; ++i) {
        float val = osc.process();
        maxVal = std::max(maxVal, std::fabs(val));
    }

    // Triangle should be bounded (roughly [-1, 1] with some tolerance for integration)
    REQUIRE(maxVal < 2.0f);
    REQUIRE(maxVal > 0.1f); // Should have non-trivial amplitude
}

TEST_CASE("PolyBlepOscillator pulse width modifies square wave duty", "[polybleposcillator]") {
    libdsp::PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequency(100.0f);
    osc.setWaveform(libdsp::PolyBlepOscillator::Waveform::Square);
    osc.setPulseWidth(0.25f);

    int posCount = 0;
    int total = 44100;
    for (int i = 0; i < total; ++i) {
        if (osc.process() > 0.0f) posCount++;
    }

    // With 25% pulse width, about 25% of samples should be positive
    float posRatio = static_cast<float>(posCount) / static_cast<float>(total);
    REQUIRE(posRatio > 0.15f);
    REQUIRE(posRatio < 0.35f);
}

TEST_CASE("PolyBlepOscillator reset clears state", "[polybleposcillator]") {
    libdsp::PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequency(440.0f);
    osc.setWaveform(libdsp::PolyBlepOscillator::Waveform::Saw);

    // Generate some samples
    for (int i = 0; i < 1000; ++i) osc.process();

    osc.reset();

    // After reset, first sample should be same as a fresh oscillator
    float afterReset = osc.process();

    libdsp::PolyBlepOscillator fresh;
    fresh.prepare(kSampleRate);
    fresh.setFrequency(440.0f);
    fresh.setWaveform(libdsp::PolyBlepOscillator::Waveform::Saw);
    float freshFirst = fresh.process();

    REQUIRE_THAT(afterReset, WithinAbs(freshFirst, 0.001));
}

TEST_CASE("PolyBlepOscillator saw has non-zero RMS", "[polybleposcillator]") {
    libdsp::PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequency(440.0f);
    osc.setWaveform(libdsp::PolyBlepOscillator::Waveform::Saw);

    auto samples = generateSamples(osc, 44100);
    float rmsVal = rms(samples);

    // Saw RMS = 1/sqrt(3) ≈ 0.577
    REQUIRE(rmsVal > 0.4f);
    REQUIRE(rmsVal < 0.7f);
}

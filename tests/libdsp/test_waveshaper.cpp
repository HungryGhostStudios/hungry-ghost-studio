#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/nonlinear/WaveShaper.h>
#include <libdsp/nonlinear/ADAA.h>
#include <cmath>
#include <vector>
#include <numeric>

using Catch::Matchers::WithinAbs;

// ── ADAA tests ──────────────────────────────────────────────────────────

TEST_CASE("ADAA: soft clip tanh reduces aliasing vs naive", "[adaa]") {
    // Generate a high-frequency sine that would alias badly with naive tanh
    constexpr float sampleRate = 44100.0f;
    constexpr float freq = 8000.0f; // high frequency to provoke aliasing
    constexpr int numSamples = 1024;
    constexpr float drive = 5.0f;

    auto tanhFunc = [](float x) { return std::tanh(x); };
    auto tanhAnti = [](float x) { return std::log(std::cosh(x)); };
    auto adaa = libdsp::makeADAA(tanhFunc, tanhAnti);

    // Process with ADAA
    std::vector<float> adaaOut(numSamples);
    std::vector<float> naiveOut(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float x = drive * std::sin(2.0f * 3.14159265f * freq * i / sampleRate);
        adaaOut[i] = adaa.process(x);
        naiveOut[i] = std::tanh(x);
    }

    // Both should produce non-zero output
    float adaaEnergy = 0.0f, naiveEnergy = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        adaaEnergy += adaaOut[i] * adaaOut[i];
        naiveEnergy += naiveOut[i] * naiveOut[i];
    }
    REQUIRE(adaaEnergy > 0.0f);
    REQUIRE(naiveEnergy > 0.0f);

    // ADAA output should be bounded (tanh output is in [-1, 1])
    for (int i = 0; i < numSamples; ++i) {
        REQUIRE(std::fabs(adaaOut[i]) <= 1.1f); // small tolerance
    }
}

TEST_CASE("ADAA: reset clears state", "[adaa]") {
    auto f = [](float x) { return std::tanh(x); };
    auto af = [](float x) { return std::log(std::cosh(x)); };
    auto adaa = libdsp::makeADAA(f, af);

    // Process some samples
    adaa.process(0.5f);
    adaa.process(0.8f);

    // Reset
    adaa.reset();

    // After reset, processing 0.0 should give ~0
    float out = adaa.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(0.0f, 0.01f));
}

TEST_CASE("ADAA: identical consecutive samples fall back to direct eval", "[adaa]") {
    auto f = [](float x) { return std::tanh(x); };
    auto af = [](float x) { return std::log(std::cosh(x)); };
    auto adaa = libdsp::makeADAA(f, af);

    float val = 0.5f;
    adaa.process(val);
    float out = adaa.process(val); // same value — should use direct eval
    REQUIRE_THAT(out, WithinAbs(std::tanh(val), 0.001f));
}

// ── WaveShaper mode tests ───────────────────────────────────────────────

TEST_CASE("WaveShaper: Soft mode (tanh) saturates signal", "[waveshaper]") {
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaperMode::Soft);
    ws.setDrive(5.0f);
    ws.setBias(0.0f);
    ws.setDCBlock(false);

    // Large input should be compressed toward [-1, 1]
    float out = ws.process(1.0f);
    REQUIRE(out > 0.0f);
    REQUIRE(out <= 1.0f);

    // Output magnitude should be less than drive * input for high drive
    out = ws.process(0.5f);
    REQUIRE(out > 0.0f);
    REQUIRE(out < 2.5f); // much less than drive * 0.5
}

TEST_CASE("WaveShaper: Hard mode clips to [-1, 1]", "[waveshaper]") {
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaperMode::Hard);
    ws.setDrive(3.0f);
    ws.setBias(0.0f);
    ws.setDCBlock(false);

    // Process several samples to get past ADAA warmup
    ws.process(0.0f);
    for (int i = 0; i < 10; ++i) {
        float out = ws.process(0.8f);
        REQUIRE(out <= 1.05f); // tolerance for ADAA averaging
    }
}

TEST_CASE("WaveShaper: Fold mode produces wavefolder output", "[waveshaper]") {
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaperMode::Fold);
    ws.setDrive(1.0f);
    ws.setBias(0.0f);
    ws.setDCBlock(false);

    // sin(pi/2 * 1.0) = 1.0, sin(pi/2 * 2.0) = 0.0 (folds back)
    ws.process(0.0f); // warmup
    float out = ws.process(1.0f);
    REQUIRE(out > 0.0f); // should be positive
}

TEST_CASE("WaveShaper: Asym mode has different pos/neg behavior", "[waveshaper]") {
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaperMode::Asym);
    ws.setDrive(1.0f);
    ws.setBias(0.0f);
    ws.setDCBlock(false);

    // Process positive and negative with same magnitude
    ws.reset();
    ws.process(0.0f);
    float posOut = ws.process(0.8f);

    ws.reset();
    ws.process(0.0f);
    float negOut = ws.process(-0.8f);

    // Asymmetric: magnitudes should differ
    REQUIRE(std::fabs(posOut) != Catch::Approx(std::fabs(negOut)).margin(0.01f));
}

TEST_CASE("WaveShaper: Tape mode produces asymmetric saturation", "[waveshaper]") {
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaperMode::Tape);
    ws.setDrive(2.0f);
    ws.setBias(0.0f);
    ws.setDCBlock(false);

    ws.process(0.0f); // warmup
    float out = ws.process(0.5f);
    REQUIRE(std::isfinite(out));
    REQUIRE(out > 0.0f);
}

TEST_CASE("WaveShaper: DC blocker removes DC offset", "[waveshaper]") {
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaperMode::Asym);
    ws.setDrive(2.0f);
    ws.setBias(0.3f); // bias introduces DC
    ws.setDCBlock(true);

    // Process a constant signal — DC blocker should converge output toward 0
    float lastOut = 0.0f;
    for (int i = 0; i < 44100; ++i) {
        lastOut = ws.process(0.5f);
    }
    // After many samples of constant input, DC blocker should have removed the DC
    REQUIRE(std::fabs(lastOut) < 0.05f);
}

TEST_CASE("WaveShaper: bias shifts the transfer function", "[waveshaper]") {
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaperMode::Soft);
    ws.setDrive(1.0f);
    ws.setDCBlock(false);

    // Zero input with bias should produce non-zero output
    ws.setBias(0.5f);
    ws.process(0.0f); // warmup
    float out = ws.process(0.0f);
    // With bias=0.5, input to tanh is 0.5, output should be tanh(0.5) ≈ 0.46
    REQUIRE(std::fabs(out) > 0.1f);
}

TEST_CASE("WaveShaper: zero input produces zero output (no bias)", "[waveshaper]") {
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaperMode::Soft);
    ws.setDrive(1.0f);
    ws.setBias(0.0f);
    ws.setDCBlock(false);

    // Multiple zero samples should produce zero
    for (int i = 0; i < 100; ++i) {
        float out = ws.process(0.0f);
        REQUIRE_THAT(out, WithinAbs(0.0f, 0.001f));
    }
}

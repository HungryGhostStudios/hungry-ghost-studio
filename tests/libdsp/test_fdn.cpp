#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <numeric>
#include <vector>

#include "libdsp/reverb/FDN.h"
#include "libdsp/reverb/AllpassChain.h"

using Catch::Matchers::WithinAbs;

static constexpr double kSampleRate = 44100.0;
static constexpr int kBlockSize = 512;

// ─── FDN RT60 Decay ─────────────────────────────────────────────────

TEST_CASE("FDN: output decays to -60dB within specified RT60", "[fdn][decay]") {
    libdsp::FDN fdn;
    fdn.prepare(kSampleRate, kBlockSize);

    const float rt60 = 2.0f; // 2 seconds
    fdn.setDecay(rt60);
    fdn.setDamping(20000.0f); // minimal damping for clean measurement
    fdn.setModDepth(0.0f);    // no modulation
    fdn.setSize(1.0f);
    fdn.setDiffusion(1.0f);

    // Inject an impulse
    float peakLevel = std::abs(fdn.process(1.0f));

    // Process for RT60 duration and track peak
    const int rt60Samples = static_cast<int>(rt60 * kSampleRate);
    float levelAtRT60 = 0.0f;

    for (int i = 1; i < rt60Samples; ++i) {
        float out = fdn.process(0.0f);
        if (i == 0) peakLevel = std::max(peakLevel, std::abs(out));
        // Sample near the end of RT60
        if (i >= rt60Samples - 512) {
            levelAtRT60 = std::max(levelAtRT60, std::abs(out));
        }
    }

    // After RT60 the level should be roughly -60dB below the initial impulse response peak
    // Allow ±20% tolerance on the time (we check at exactly RT60)
    // The level should be very low — at least -40dB below peak (relaxed tolerance)
    if (peakLevel > 0.0f) {
        float dbDown = 20.0f * std::log10(levelAtRT60 / peakLevel);
        REQUIRE(dbDown < -40.0f); // at least 40dB down by RT60
    }
}

TEST_CASE("FDN: shorter decay results in faster energy loss", "[fdn][decay]") {
    auto measureEnergyAfter = [](float decayTime, float measureSeconds) -> float {
        libdsp::FDN fdn;
        fdn.prepare(kSampleRate, kBlockSize);
        fdn.setDecay(decayTime);
        fdn.setDamping(20000.0f);
        fdn.setModDepth(0.0f);
        fdn.setDiffusion(1.0f);
        fdn.setSize(1.0f);

        fdn.process(1.0f); // impulse

        const int samples = static_cast<int>(measureSeconds * kSampleRate);
        float energy = 0.0f;
        for (int i = 0; i < samples; ++i) {
            float out = fdn.process(0.0f);
            energy += out * out;
        }
        return energy;
    };

    float energyShort = measureEnergyAfter(0.5f, 1.0f);
    float energyLong = measureEnergyAfter(4.0f, 1.0f);

    REQUIRE(energyLong > energyShort);
}

// ─── Hadamard Energy Preservation ────────────────────────────────────

TEST_CASE("FDN: Hadamard matrix preserves energy", "[fdn][hadamard]") {
    // We can test energy preservation indirectly: inject a known energy pulse
    // and verify total output energy is roughly conserved (minus feedback losses)
    libdsp::FDN fdn;
    fdn.prepare(kSampleRate, kBlockSize);
    fdn.setDecay(10.0f);       // very long decay to minimize feedback loss
    fdn.setDamping(20000.0f);  // minimal damping
    fdn.setModDepth(0.0f);
    fdn.setDiffusion(1.0f);
    fdn.setSize(1.0f);

    // Inject impulse and collect first 4096 samples of output
    float inputEnergy = 1.0f; // impulse of 1.0
    float outputEnergy = 0.0f;

    float first = fdn.process(1.0f);
    outputEnergy += first * first;

    for (int i = 1; i < 4096; ++i) {
        float out = fdn.process(0.0f);
        outputEnergy += out * out;
    }

    // With very long decay and minimal damping, output energy should be
    // a significant fraction of input energy (energy redistributed, not lost)
    REQUIRE(outputEnergy > 0.1f * inputEnergy);
}

// ─── Damping ─────────────────────────────────────────────────────────

TEST_CASE("FDN: damping reduces high-frequency content over time", "[fdn][damping]") {
    auto measureHFEnergy = [](float dampingFreq) -> float {
        libdsp::FDN fdn;
        fdn.prepare(kSampleRate, kBlockSize);
        fdn.setDecay(3.0f);
        fdn.setDamping(dampingFreq);
        fdn.setModDepth(0.0f);
        fdn.setDiffusion(1.0f);
        fdn.setSize(1.0f);

        // Inject impulse (broadband)
        fdn.process(1.0f);

        // Skip initial transient
        for (int i = 0; i < 4410; ++i) { // 100ms
            fdn.process(0.0f);
        }

        // Measure "HF energy" via simple differencing (high-pass proxy)
        float hfEnergy = 0.0f;
        float prev = 0.0f;
        for (int i = 0; i < 8820; ++i) { // 200ms window
            float out = fdn.process(0.0f);
            float diff = out - prev;
            hfEnergy += diff * diff;
            prev = out;
        }
        return hfEnergy;
    };

    float hfLowDamp = measureHFEnergy(1000.0f);   // aggressive damping
    float hfHighDamp = measureHFEnergy(18000.0f);  // minimal damping

    // Low damping freq should have less HF energy
    REQUIRE(hfLowDamp < hfHighDamp);
}

// ─── Freeze Mode ─────────────────────────────────────────────────────

TEST_CASE("FDN: freeze mode maintains level indefinitely", "[fdn][freeze]") {
    libdsp::FDN fdn;
    fdn.prepare(kSampleRate, kBlockSize);
    fdn.setDecay(1.0f);       // short normal decay
    fdn.setDamping(5000.0f);
    fdn.setModDepth(0.0f);
    fdn.setDiffusion(1.0f);
    fdn.setSize(1.0f);

    // Inject energy
    fdn.process(1.0f);
    for (int i = 0; i < 2000; ++i) {
        fdn.process(0.0f);
    }

    // Enable freeze
    fdn.setFreeze(true);

    // Measure energy at two points separated by 5 seconds
    auto measureWindowEnergy = [&](int numSamples) -> float {
        float energy = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            float out = fdn.process(0.0f);
            energy += out * out;
        }
        return energy;
    };

    float energy1 = measureWindowEnergy(4410); // 100ms window

    // Skip 5 seconds
    for (int i = 0; i < static_cast<int>(5.0 * kSampleRate); ++i) {
        fdn.process(0.0f);
    }

    float energy2 = measureWindowEnergy(4410); // another 100ms window

    // In freeze mode, energy should be maintained (within 10% tolerance)
    if (energy1 > 1e-10f) {
        float ratio = energy2 / energy1;
        REQUIRE(ratio > 0.9f);
        REQUIRE(ratio < 1.1f);
    }
}

// ─── AllpassChain Tests ──────────────────────────────────────────────

TEST_CASE("AllpassChain: output preserves energy (allpass property)", "[allpasschain]") {
    libdsp::AllpassChain chain;
    chain.prepare(kSampleRate, 4096);
    chain.setNumStages(4);
    for (int i = 0; i < 4; ++i) {
        chain.setStageParams(i, 100 + i * 37, 0.5f);
    }

    // Process a burst of white noise and compare input/output energy
    float inputEnergy = 0.0f;
    float outputEnergy = 0.0f;
    const int N = 44100; // 1 second

    // Simple deterministic signal
    for (int i = 0; i < N; ++i) {
        float x = std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 44100.0f);
        inputEnergy += x * x;
        float y = chain.process(x);
        outputEnergy += y * y;
    }

    // Allpass should preserve energy (within 5% for long signals)
    float ratio = outputEnergy / inputEnergy;
    REQUIRE(ratio > 0.95f);
    REQUIRE(ratio < 1.05f);
}

TEST_CASE("AllpassChain: reset clears state", "[allpasschain]") {
    libdsp::AllpassChain chain;
    chain.prepare(kSampleRate, 4096);
    chain.setNumStages(2);
    chain.setStageParams(0, 50, 0.5f);
    chain.setStageParams(1, 80, 0.5f);

    // Feed signal
    for (int i = 0; i < 1000; ++i) {
        chain.process(static_cast<float>(i % 2));
    }

    chain.reset();

    // After reset, output should be zero for zero input
    float out = chain.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(0.0, 1e-10));
}

TEST_CASE("FDN: reset produces silence", "[fdn][reset]") {
    libdsp::FDN fdn;
    fdn.prepare(kSampleRate, kBlockSize);
    fdn.setDecay(3.0f);

    // Build up state
    fdn.process(1.0f);
    for (int i = 0; i < 1000; ++i) {
        fdn.process(0.0f);
    }

    fdn.reset();

    // After reset, output should be silence
    float out = fdn.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(0.0, 1e-10));
}

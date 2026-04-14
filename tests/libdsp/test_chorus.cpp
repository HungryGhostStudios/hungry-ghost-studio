#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/modulation/Chorus.h>
#include <cmath>
#include <vector>

using namespace libdsp;
using Catch::Matchers::WithinAbs;

static constexpr double kSampleRate = 44100.0;
static constexpr int kBlockSize = 512;

TEST_CASE("Chorus - prepare and process produce output", "[chorus]") {
    Chorus chorus;
    chorus.prepare(kSampleRate, kBlockSize);
    chorus.setVoices(2);
    chorus.setRate(1.0f);
    chorus.setDepth(3.0f);
    chorus.setDelay(7.0f);

    // Process a block of silence — should produce near-silence
    float maxOut = 0.0f;
    for (int i = 0; i < 1024; ++i) {
        auto out = chorus.process(0.0f, 0.0f);
        maxOut = std::max(maxOut, std::max(std::abs(out.left), std::abs(out.right)));
    }
    REQUIRE(maxOut < 0.01f);
}

TEST_CASE("Chorus - signal passes through with modulation", "[chorus]") {
    Chorus chorus;
    chorus.prepare(kSampleRate, kBlockSize);
    chorus.setVoices(2);
    chorus.setRate(0.5f);
    chorus.setDepth(2.0f);
    chorus.setDelay(5.0f);
    chorus.setFeedback(0.0f);
    chorus.setMode(Chorus::Mode::Clean);

    // Feed a constant signal and wait for delay to fill
    float sum = 0.0f;
    int numSamples = 4410; // 100ms worth
    for (int i = 0; i < numSamples; ++i) {
        auto out = chorus.process(1.0f, 1.0f);
        if (i > 1000) { // After delay settles
            sum += std::abs(out.left) + std::abs(out.right);
        }
    }
    // Should have non-trivial output
    float avgLevel = sum / (2.0f * static_cast<float>(numSamples - 1000));
    REQUIRE(avgLevel > 0.1f);
}

TEST_CASE("Chorus - voice count clamp", "[chorus]") {
    Chorus chorus;
    chorus.prepare(kSampleRate, kBlockSize);

    // Set voices below min (2), should clamp
    chorus.setVoices(1);
    chorus.setRate(1.0f);
    chorus.setDepth(2.0f);
    chorus.setDelay(5.0f);

    // Should still produce output without crash
    for (int i = 0; i < 512; ++i) {
        auto out = chorus.process(0.5f, 0.5f);
        REQUIRE(std::isfinite(out.left));
        REQUIRE(std::isfinite(out.right));
    }

    // Set above max (6), should clamp
    chorus.setVoices(10);
    for (int i = 0; i < 512; ++i) {
        auto out = chorus.process(0.5f, 0.5f);
        REQUIRE(std::isfinite(out.left));
        REQUIRE(std::isfinite(out.right));
    }
}

TEST_CASE("Chorus - feedback produces longer tail", "[chorus]") {
    // Without feedback
    Chorus chorusNoFb;
    chorusNoFb.prepare(kSampleRate, kBlockSize);
    chorusNoFb.setVoices(2);
    chorusNoFb.setRate(1.0f);
    chorusNoFb.setDepth(2.0f);
    chorusNoFb.setDelay(5.0f);
    chorusNoFb.setFeedback(0.0f);

    // With feedback
    Chorus chorusFb;
    chorusFb.prepare(kSampleRate, kBlockSize);
    chorusFb.setVoices(2);
    chorusFb.setRate(1.0f);
    chorusFb.setDepth(2.0f);
    chorusFb.setDelay(5.0f);
    chorusFb.setFeedback(0.7f);

    // Send an impulse
    chorusNoFb.process(1.0f, 1.0f);
    chorusFb.process(1.0f, 1.0f);

    // Process silence and measure tail energy
    float energyNoFb = 0.0f;
    float energyFb = 0.0f;
    for (int i = 0; i < 4410; ++i) {
        auto outNoFb = chorusNoFb.process(0.0f, 0.0f);
        auto outFb = chorusFb.process(0.0f, 0.0f);
        energyNoFb += outNoFb.left * outNoFb.left + outNoFb.right * outNoFb.right;
        energyFb += outFb.left * outFb.left + outFb.right * outFb.right;
    }

    REQUIRE(energyFb > energyNoFb);
}

TEST_CASE("Chorus - BBD mode adds noise", "[chorus]") {
    Chorus chorusClean;
    chorusClean.prepare(kSampleRate, kBlockSize);
    chorusClean.setVoices(2);
    chorusClean.setRate(1.0f);
    chorusClean.setDepth(2.0f);
    chorusClean.setDelay(7.0f);
    chorusClean.setMode(Chorus::Mode::Clean);

    Chorus chorusBBD;
    chorusBBD.prepare(kSampleRate, kBlockSize);
    chorusBBD.setVoices(2);
    chorusBBD.setRate(1.0f);
    chorusBBD.setDepth(2.0f);
    chorusBBD.setDelay(7.0f);
    chorusBBD.setMode(Chorus::Mode::BBD);

    // Process silence — BBD should have residual noise, Clean should not
    float cleanEnergy = 0.0f;
    float bbdEnergy = 0.0f;
    for (int i = 0; i < 4410; ++i) {
        auto cOut = chorusClean.process(0.0f, 0.0f);
        auto bOut = chorusBBD.process(0.0f, 0.0f);
        cleanEnergy += cOut.left * cOut.left + cOut.right * cOut.right;
        bbdEnergy += bOut.left * bOut.left + bOut.right * bOut.right;
    }

    // BBD mode should have more energy due to noise
    REQUIRE(bbdEnergy > cleanEnergy);
}

TEST_CASE("Chorus - stereo spread distributes across channels", "[chorus]") {
    Chorus chorus;
    chorus.prepare(kSampleRate, kBlockSize);
    chorus.setVoices(4);
    chorus.setRate(0.5f);
    chorus.setDepth(2.0f);
    chorus.setDelay(7.0f);
    chorus.setSpread(1.0f);
    chorus.setMode(Chorus::Mode::Clean);

    // Mono input
    float sumL = 0.0f;
    float sumR = 0.0f;
    for (int i = 0; i < 4410; ++i) {
        auto out = chorus.process(1.0f, 1.0f);
        if (i > 1000) {
            sumL += std::abs(out.left);
            sumR += std::abs(out.right);
        }
    }

    // Both channels should have energy when spread is applied
    REQUIRE(sumL > 0.0f);
    REQUIRE(sumR > 0.0f);
}

TEST_CASE("Chorus - reset clears state", "[chorus]") {
    Chorus chorus;
    chorus.prepare(kSampleRate, kBlockSize);
    chorus.setVoices(2);
    chorus.setRate(1.0f);
    chorus.setDepth(2.0f);
    chorus.setDelay(5.0f);
    chorus.setFeedback(0.5f);

    // Feed signal
    for (int i = 0; i < 2205; ++i) {
        chorus.process(1.0f, 1.0f);
    }

    chorus.reset();

    // After reset, processing silence should yield near-silence
    float maxOut = 0.0f;
    for (int i = 0; i < 512; ++i) {
        auto out = chorus.process(0.0f, 0.0f);
        maxOut = std::max(maxOut, std::max(std::abs(out.left), std::abs(out.right)));
    }
    REQUIRE(maxOut < 0.01f);
}

TEST_CASE("Chorus - all outputs are finite", "[chorus]") {
    Chorus chorus;
    chorus.prepare(kSampleRate, kBlockSize);
    chorus.setVoices(6);
    chorus.setRate(5.0f);
    chorus.setDepth(10.0f);
    chorus.setDelay(15.0f);
    chorus.setFeedback(0.9f);
    chorus.setSpread(1.0f);
    chorus.setMode(Chorus::Mode::BBD);

    // Stress test with large input
    for (int i = 0; i < 44100; ++i) {
        float input = std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * static_cast<float>(i) / 44100.0f);
        auto out = chorus.process(input, input);
        REQUIRE(std::isfinite(out.left));
        REQUIRE(std::isfinite(out.right));
    }
}

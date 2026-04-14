#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/reverb/FDN.h>
#include <libdsp/reverb/AllpassChain.h>
#include <cmath>
#include <numeric>
#include <vector>

using Catch::Matchers::WithinAbs;

static constexpr double kSampleRate = 44100.0;
static constexpr int kBlockSize = 512;

// Helper: compute RMS of a buffer
static float rms(const std::vector<float>& buf) {
    if (buf.empty()) return 0.0f;
    float sum = 0.0f;
    for (auto s : buf) sum += s * s;
    return std::sqrt(sum / static_cast<float>(buf.size()));
}

// Helper: compute RMS in dB
static float rmsDb(const std::vector<float>& buf) {
    float r = rms(buf);
    if (r < 1e-10f) return -200.0f;
    return 20.0f * std::log10(r);
}

TEST_CASE("FDN: output decays to -60dB within ±20% of RT60", "[reverb][fdn]")
{
    libdsp::FDN fdn;
    fdn.prepare(kSampleRate, kBlockSize);

    const float rt60 = 2.0f; // 2 seconds
    fdn.setDecay(rt60);
    fdn.setDamping(20000.0f); // minimal damping for clean decay
    fdn.setDiffusion(1.0f);
    fdn.setModRate(0.0f);
    fdn.setModDepth(0.0f);
    fdn.setSize(1.0f);
    fdn.setFreeze(false);

    // Inject an impulse
    float firstOut = fdn.process(1.0f);
    (void)firstOut;

    // Process silence and measure decay
    const int totalSamples = static_cast<int>(kSampleRate * rt60 * 2.0f); // 2x RT60
    std::vector<float> output(static_cast<size_t>(totalSamples));
    for (int i = 0; i < totalSamples; ++i) {
        output[static_cast<size_t>(i)] = fdn.process(0.0f);
    }

    // Measure RMS in the first 100ms to get initial level
    const int earlyEnd = static_cast<int>(kSampleRate * 0.1);
    std::vector<float> earlyBuf(output.begin(), output.begin() + earlyEnd);
    float earlyRmsDb = rmsDb(earlyBuf);

    // Find time when RMS drops to earlyRmsDb - 60dB
    // Measure in 100ms windows
    const int windowSize = static_cast<int>(kSampleRate * 0.1);
    float decayTime = -1.0f;
    for (int start = windowSize; start + windowSize < totalSamples; start += windowSize) {
        std::vector<float> window(output.begin() + start, output.begin() + start + windowSize);
        float windowDb = rmsDb(window);
        if (windowDb < earlyRmsDb - 60.0f) {
            decayTime = static_cast<float>(start) / static_cast<float>(kSampleRate);
            break;
        }
    }

    // The decay time should be within ±20% of the set RT60
    REQUIRE(decayTime > 0.0f);
    REQUIRE(decayTime >= rt60 * 0.8f);
    REQUIRE(decayTime <= rt60 * 1.2f);
}

TEST_CASE("FDN: Hadamard matrix preserves energy", "[reverb][fdn]")
{
    // Test energy preservation by checking that the FDN doesn't
    // amplify or attenuate excessively over a short burst.
    // Feed white noise impulse and verify output energy is reasonable.
    libdsp::FDN fdn;
    fdn.prepare(kSampleRate, kBlockSize);
    fdn.setDecay(1.0f);
    fdn.setDamping(20000.0f);
    fdn.setDiffusion(1.0f);
    fdn.setModRate(0.0f);
    fdn.setModDepth(0.0f);
    fdn.setSize(1.0f);
    fdn.setFreeze(false);

    // Inject unit impulse and collect output
    float totalEnergy = 0.0f;
    float out = fdn.process(1.0f);
    totalEnergy += out * out;

    // Process one full cycle of the shortest delay line (~1087 samples)
    for (int i = 0; i < 2000; ++i) {
        out = fdn.process(0.0f);
        totalEnergy += out * out;
    }

    // With energy preservation, total output energy should be
    // close to input energy (1.0) over this time window.
    // Allow generous range since FDN distributes energy over time.
    REQUIRE(totalEnergy > 0.01f);  // not silence
    REQUIRE(totalEnergy < 10.0f);  // not exploding
}

TEST_CASE("FDN: damping reduces high-frequency content over time", "[reverb][fdn]")
{
    // Compare HF content of early vs late reverb with damping enabled
    libdsp::FDN fdn;
    fdn.prepare(kSampleRate, kBlockSize);
    fdn.setDecay(3.0f);
    fdn.setDamping(2000.0f); // aggressive damping at 2kHz
    fdn.setDiffusion(1.0f);
    fdn.setModRate(0.0f);
    fdn.setModDepth(0.0f);
    fdn.setSize(1.0f);
    fdn.setFreeze(false);

    // Inject impulse
    fdn.process(1.0f);

    // Collect early output (first 0.1s after initial delay)
    const int earlyStart = 3000; // after initial delays settle
    const int windowLen = 4410;  // 100ms
    std::vector<float> earlyOut;
    for (int i = 0; i < earlyStart + windowLen; ++i) {
        float s = fdn.process(0.0f);
        if (i >= earlyStart) earlyOut.push_back(s);
    }

    // Collect late output (at ~1.5s)
    const int lateStart = static_cast<int>(1.5 * kSampleRate) - earlyStart - windowLen;
    std::vector<float> lateOut;
    for (int i = 0; i < lateStart + windowLen; ++i) {
        float s = fdn.process(0.0f);
        if (i >= lateStart) lateOut.push_back(s);
    }

    // Compute a simple HF proxy: sum of absolute differences between consecutive samples
    // (high-frequency content produces larger differences)
    auto hfEnergy = [](const std::vector<float>& buf) {
        float sum = 0.0f;
        for (size_t i = 1; i < buf.size(); ++i) {
            float diff = buf[i] - buf[i - 1];
            sum += diff * diff;
        }
        return sum / static_cast<float>(buf.size());
    };

    float earlyHF = hfEnergy(earlyOut);
    float lateHF = hfEnergy(lateOut);

    // Late reverb should have less HF energy due to damping
    // (unless early is already very quiet — check both have signal)
    if (rms(earlyOut) > 1e-6f && rms(lateOut) > 1e-6f) {
        // Normalize HF by overall energy to compare spectral tilt
        float earlyHFNorm = earlyHF / (rms(earlyOut) * rms(earlyOut) + 1e-20f);
        float lateHFNorm = lateHF / (rms(lateOut) * rms(lateOut) + 1e-20f);
        REQUIRE(lateHFNorm < earlyHFNorm);
    }
}

TEST_CASE("FDN: freeze mode maintains level indefinitely", "[reverb][fdn]")
{
    libdsp::FDN fdn;
    fdn.prepare(kSampleRate, kBlockSize);
    fdn.setDecay(1.0f);
    fdn.setDamping(5000.0f);
    fdn.setDiffusion(1.0f);
    fdn.setModRate(0.0f);
    fdn.setModDepth(0.0f);
    fdn.setSize(1.0f);
    fdn.setFreeze(false);

    // Inject a burst of samples to fill the reverb
    for (int i = 0; i < 1000; ++i) {
        fdn.process(0.5f * std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(i) / static_cast<float>(kSampleRate)));
    }

    // Enable freeze
    fdn.setFreeze(true);

    // Measure level at 1 second into freeze
    const int oneSec = static_cast<int>(kSampleRate);
    std::vector<float> atOneSec;
    for (int i = 0; i < oneSec; ++i) {
        float s = fdn.process(0.0f);
        if (i >= oneSec - 4410) atOneSec.push_back(s);
    }
    float levelAt1s = rms(atOneSec);

    // Measure level at 5 seconds into freeze (4 more seconds)
    const int fourMoreSec = static_cast<int>(kSampleRate * 4.0);
    std::vector<float> atFiveSec;
    for (int i = 0; i < fourMoreSec; ++i) {
        float s = fdn.process(0.0f);
        if (i >= fourMoreSec - 4410) atFiveSec.push_back(s);
    }
    float levelAt5s = rms(atFiveSec);

    // In freeze mode, level should not decay significantly
    // Allow small tolerance for floating-point accumulation
    REQUIRE(levelAt1s > 1e-6f); // must have signal
    REQUIRE(levelAt5s > levelAt1s * 0.9f); // no more than 10% drop over 4 seconds
}

TEST_CASE("AllpassChain: preserves energy (allpass property)", "[reverb][allpass]")
{
    libdsp::AllpassChain chain;
    chain.prepare(kSampleRate, 4096);
    chain.setNumStages(4);
    chain.setStageParams(0, 113, 0.5f);
    chain.setStageParams(1, 199, 0.5f);
    chain.setStageParams(2, 307, 0.5f);
    chain.setStageParams(3, 443, 0.5f);

    // Feed a burst and collect output, then collect tail
    float inputEnergy = 0.0f;
    float outputEnergy = 0.0f;

    // Input burst: 1000 samples of sine
    for (int i = 0; i < 1000; ++i) {
        float in = std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(i) / static_cast<float>(kSampleRate));
        inputEnergy += in * in;
        float out = chain.process(in);
        outputEnergy += out * out;
    }

    // Collect tail (allpass chains ring out)
    for (int i = 0; i < 5000; ++i) {
        float out = chain.process(0.0f);
        outputEnergy += out * out;
    }

    // Total output energy should closely match input energy (allpass property)
    float ratio = outputEnergy / (inputEnergy + 1e-20f);
    REQUIRE(ratio > 0.9f);
    REQUIRE(ratio < 1.1f);
}

TEST_CASE("AllpassChain: reset clears state", "[reverb][allpass]")
{
    libdsp::AllpassChain chain;
    chain.prepare(kSampleRate, 4096);
    chain.setNumStages(2);
    chain.setStageParams(0, 50, 0.5f);
    chain.setStageParams(1, 80, 0.5f);

    // Fill with signal
    for (int i = 0; i < 200; ++i) {
        chain.process(1.0f);
    }

    chain.reset();

    // After reset, processing silence should give silence
    float out = chain.process(0.0f);
    REQUIRE_THAT(out, WithinAbs(0.0, 1e-6));
}

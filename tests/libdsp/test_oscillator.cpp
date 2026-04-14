#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/synthesis/PolyBlepOscillator.h>

#include <cmath>
#include <vector>
#include <numeric>
#include <complex>

using namespace libdsp::synthesis;
using Catch::Matchers::WithinAbs;

namespace {

constexpr double kSampleRate = 44100.0;
constexpr float kTwoPi = 6.28318530718f;

// Compute THD (Total Harmonic Distortion) of a signal at a given fundamental frequency
// Returns THD in dB (negative means low distortion)
float computeTHD(const std::vector<float>& signal, float fundamentalHz, double sampleRate)
{
    const size_t N = signal.size();

    // DFT at fundamental and harmonics (up to 10th)
    float fundamentalPower = 0.f;
    float harmonicPower = 0.f;

    auto dftBin = [&](float freqHz) -> float {
        double re = 0.0, im = 0.0;
        for (size_t n = 0; n < N; ++n)
        {
            double angle = -kTwoPi * freqHz * n / sampleRate;
            re += signal[n] * std::cos(angle);
            im += signal[n] * std::sin(angle);
        }
        return static_cast<float>(re * re + im * im);
    };

    fundamentalPower = dftBin(fundamentalHz);

    for (int h = 2; h <= 10; ++h)
    {
        float hFreq = fundamentalHz * static_cast<float>(h);
        if (hFreq >= sampleRate / 2.0)
            break;
        harmonicPower += dftBin(hFreq);
    }

    if (fundamentalPower < 1e-20f)
        return 0.f;

    return 10.f * std::log10(harmonicPower / fundamentalPower);
}

} // anonymous namespace

TEST_CASE("PolyBlepOscillator: prepare resets state", "[oscillator]")
{
    PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequencyHz(440.f);
    osc.setWaveform(OscWaveform::Saw);

    // Generate some samples
    for (int i = 0; i < 100; ++i)
        osc.processSample();

    // Re-prepare should reset
    osc.prepare(kSampleRate);

    // First sample of saw should be near -1 (phase=0 → 2*0-1 = -1)
    float first = osc.processSample();
    REQUIRE_THAT(first, WithinAbs(-1.f, 0.1f));
}

TEST_CASE("PolyBlepOscillator: sine output is correct", "[oscillator]")
{
    PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequencyHz(440.f);
    osc.setWaveform(OscWaveform::Sine);

    // Sine at phase=0 should be 0
    float first = osc.processSample();
    REQUIRE_THAT(first, WithinAbs(0.f, 0.01f));

    // Generate a full cycle and check zero crossings
    int samplesPerCycle = static_cast<int>(kSampleRate / 440.0);
    std::vector<float> cycle(samplesPerCycle);
    cycle[0] = first;
    for (int i = 1; i < samplesPerCycle; ++i)
        cycle[i] = osc.processSample();

    // Find max — should be near 1.0
    float maxVal = *std::max_element(cycle.begin(), cycle.end());
    REQUIRE(maxVal > 0.95f);
    REQUIRE(maxVal <= 1.0f);

    // Find min — should be near -1.0
    float minVal = *std::min_element(cycle.begin(), cycle.end());
    REQUIRE(minVal < -0.95f);
    REQUIRE(minVal >= -1.0f);
}

TEST_CASE("PolyBlepOscillator: saw frequency is accurate", "[oscillator]")
{
    PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequencyHz(1000.f);
    osc.setWaveform(OscWaveform::Saw);

    // Let frequency smoother settle
    for (int i = 0; i < 4410; ++i)
        osc.processSample();

    // Count zero crossings (negative→positive) over 1 second
    int numSamples = static_cast<int>(kSampleRate);
    float prev = osc.processSample();
    int crossings = 0;
    for (int i = 1; i < numSamples; ++i)
    {
        float curr = osc.processSample();
        // Saw resets from +1 to -1, detect large negative step
        if (prev > 0.5f && curr < -0.5f)
            ++crossings;
        prev = curr;
    }

    // Should see ~1000 resets per second (±2 tolerance for edge cases)
    REQUIRE(crossings >= 998);
    REQUIRE(crossings <= 1002);
}

TEST_CASE("PolyBlepOscillator: polyBLEP saw has lower THD than naive saw", "[oscillator]")
{
    constexpr float testFreq = 1000.f;
    constexpr int numSamples = 8192;

    // Generate PolyBLEP saw
    PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequencyHz(testFreq);
    osc.setWaveform(OscWaveform::Saw);

    // Let smoother settle
    for (int i = 0; i < 4410; ++i)
        osc.processSample();

    std::vector<float> blepSaw(numSamples);
    for (int i = 0; i < numSamples; ++i)
        blepSaw[i] = osc.processSample();

    // Generate naive saw for comparison
    std::vector<float> naiveSaw(numSamples);
    float naivePhase = 0.f;
    float naiveDt = static_cast<float>(testFreq / kSampleRate);
    for (int i = 0; i < numSamples; ++i)
    {
        naiveSaw[i] = 2.f * naivePhase - 1.f;
        naivePhase += naiveDt;
        while (naivePhase >= 1.f)
            naivePhase -= 1.f;
    }

    float blepTHD = computeTHD(blepSaw, testFreq, kSampleRate);
    float naiveTHD = computeTHD(naiveSaw, testFreq, kSampleRate);

    // PolyBLEP should have lower THD than naive
    REQUIRE(blepTHD < naiveTHD);

    // PolyBLEP THD at 1kHz should be below -60dB (spec requirement)
    REQUIRE(blepTHD < -60.f);
}

TEST_CASE("PolyBlepOscillator: square wave pulse width", "[oscillator]")
{
    PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequencyHz(100.f);
    osc.setWaveform(OscWaveform::Square);
    osc.setPulseWidth(0.75f);

    // Let smoother settle
    for (int i = 0; i < 4410; ++i)
        osc.processSample();

    // Count positive vs negative samples over many cycles
    int positive = 0, total = 0;
    int numSamples = static_cast<int>(kSampleRate); // 1 second = 100 cycles at 100Hz
    for (int i = 0; i < numSamples; ++i)
    {
        float s = osc.processSample();
        if (s > 0.f) ++positive;
        ++total;
    }

    float ratio = static_cast<float>(positive) / static_cast<float>(total);
    // Should be approximately 75% positive (±2%)
    REQUIRE(ratio > 0.73f);
    REQUIRE(ratio < 0.77f);
}

TEST_CASE("PolyBlepOscillator: triangle output bounded", "[oscillator]")
{
    PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequencyHz(440.f);
    osc.setWaveform(OscWaveform::Triangle);

    // Let integrator stabilize
    for (int i = 0; i < 44100; ++i)
        osc.processSample();

    // Check output stays bounded
    float maxAbs = 0.f;
    for (int i = 0; i < 44100; ++i)
    {
        float s = osc.processSample();
        maxAbs = std::max(maxAbs, std::fabs(s));
    }

    // Triangle should stay bounded (not blow up)
    REQUIRE(maxAbs < 2.f);
    REQUIRE(maxAbs > 0.01f); // Should produce audible output
}

TEST_CASE("PolyBlepOscillator: waveform switching", "[oscillator]")
{
    PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setFrequencyHz(440.f);

    // Should be able to switch waveforms without crashing
    for (auto wf : {OscWaveform::Saw, OscWaveform::Square, OscWaveform::Triangle, OscWaveform::Sine})
    {
        osc.setWaveform(wf);
        for (int i = 0; i < 100; ++i)
        {
            float s = osc.processSample();
            REQUIRE(std::isfinite(s));
        }
    }
}

TEST_CASE("PolyBlepOscillator: output is finite at various frequencies", "[oscillator]")
{
    PolyBlepOscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(OscWaveform::Saw);

    for (float freq : {20.f, 100.f, 440.f, 1000.f, 5000.f, 10000.f, 20000.f})
    {
        osc.setFrequencyHz(freq);
        for (int i = 0; i < 1000; ++i)
        {
            float s = osc.processSample();
            REQUIRE(std::isfinite(s));
        }
    }
}

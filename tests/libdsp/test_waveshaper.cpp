#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/nonlinear/WaveShaper.h>
#include <libdsp/nonlinear/ADAA.h>
#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("WaveShaper Soft mode saturates", "[WaveShaper]")
{
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaper::Mode::Soft);
    ws.setDrive(5.0f);
    ws.setDCBlock(false);
    ws.setADAA(false);

    float output = ws.process(1.0f);
    // tanh(6.0) ≈ 1.0, should be saturated below 1.0
    REQUIRE(output < 1.0f);
    REQUIRE(output > 0.9f);
}

TEST_CASE("WaveShaper Hard mode clips exactly", "[WaveShaper]")
{
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaper::Mode::Hard);
    ws.setDrive(5.0f);
    ws.setDCBlock(false);
    ws.setADAA(false);

    float output = ws.process(1.0f);
    REQUIRE_THAT(output, WithinAbs(1.0f, 1e-6f));

    output = ws.process(-1.0f);
    REQUIRE_THAT(output, WithinAbs(-1.0f, 1e-6f));
}

TEST_CASE("WaveShaper Fold mode wraps signal", "[WaveShaper]")
{
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaper::Mode::Fold);
    ws.setDrive(3.0f);
    ws.setDCBlock(false);
    ws.setADAA(false);

    // With high drive, sin-based folder should produce values between -1 and 1
    float output = ws.process(0.5f);
    REQUIRE(output >= -1.0f);
    REQUIRE(output <= 1.0f);
}

TEST_CASE("WaveShaper ADAA reduces aliasing vs naive", "[WaveShaper]")
{
    // Compare ADAA on vs off with a high-frequency signal
    // ADAA should produce less high-frequency energy
    libdsp::WaveShaper wsNaive;
    wsNaive.prepare(44100.0);
    wsNaive.setMode(libdsp::WaveShaper::Mode::Soft);
    wsNaive.setDrive(4.0f);
    wsNaive.setDCBlock(false);
    wsNaive.setADAA(false);

    libdsp::WaveShaper wsADAA;
    wsADAA.prepare(44100.0);
    wsADAA.setMode(libdsp::WaveShaper::Mode::Soft);
    wsADAA.setDrive(4.0f);
    wsADAA.setDCBlock(false);
    wsADAA.setADAA(true);

    // Feed high-frequency sine and compare variance of output differences
    // (rough proxy for high-frequency content)
    const double freq = 8000.0;
    const double sr = 44100.0;
    float naiveDiffSum = 0.0f, adaaDiffSum = 0.0f;
    float prevNaive = 0.0f, prevADAA = 0.0f;

    for (int i = 0; i < 4096; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * freq * i / sr) * 0.8);
        float outNaive = wsNaive.process(input);
        float outADAA = wsADAA.process(input);

        if (i > 100) { // skip transient
            naiveDiffSum += std::abs(outNaive - prevNaive);
            adaaDiffSum += std::abs(outADAA - prevADAA);
        }
        prevNaive = outNaive;
        prevADAA = outADAA;
    }

    // ADAA output should be smoother (less sample-to-sample variation)
    // This is a soft check - ADAA smooths the output
    REQUIRE(adaaDiffSum <= naiveDiffSum * 1.1f); // allow small tolerance
}

TEST_CASE("WaveShaper DC blocker removes offset", "[WaveShaper]")
{
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaper::Mode::Asym);
    ws.setDrive(2.0f);
    ws.setBias(0.3f);
    ws.setDCBlock(true);
    ws.setADAA(false);

    // Process enough samples to let DC blocker settle
    float sum = 0.0f;
    const int N = 44100;
    for (int i = 0; i < N; ++i) {
        float input = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * i / 44100.0));
        float output = ws.process(input);
        if (i > N / 2) sum += output;
    }

    float avgDC = sum / static_cast<float>(N / 2);
    // DC component should be near zero
    REQUIRE(std::abs(avgDC) < 0.05f);
}

TEST_CASE("WaveShaper reset clears state", "[WaveShaper]")
{
    libdsp::WaveShaper ws;
    ws.prepare(44100.0);
    ws.setMode(libdsp::WaveShaper::Mode::Soft);

    for (int i = 0; i < 1000; ++i)
        ws.process(1.0f);

    ws.reset();
    float output = ws.process(0.0f);
    REQUIRE_THAT(output, WithinAbs(0.0f, 0.01f));
}

TEST_CASE("ADAA template helper works", "[ADAA]")
{
    auto adaa = libdsp::makeADAA(
        [](float x) { return std::tanh(x); },
        [](float x) { return std::log(std::cosh(x)); }
    );

    // Process a few samples - should not crash and produce reasonable output
    float out = adaa.process(0.5f);
    REQUIRE(std::isfinite(out));

    out = adaa.process(0.8f);
    REQUIRE(std::isfinite(out));
    REQUIRE(out > 0.0f);
    REQUIRE(out < 1.0f);
}

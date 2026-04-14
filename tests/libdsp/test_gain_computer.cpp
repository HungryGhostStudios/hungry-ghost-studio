#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/dynamics/GainComputer.h>

#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("GainComputer: no reduction below threshold", "[dynamics][gain_computer]")
{
    libdsp::dynamics::GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(4.f);
    gc.setKnee(0.f); // hard knee

    // Signal at -30 dB is 10 dB below threshold — no compression
    float gr = gc.process(-30.f);
    REQUIRE(gr == 0.f);
}

TEST_CASE("GainComputer: correct GR for known threshold/ratio", "[dynamics][gain_computer]")
{
    libdsp::dynamics::GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(4.f);
    gc.setKnee(0.f); // hard knee

    // Input at -10 dB → 10 dB over threshold
    // GR = overDb * (1/ratio - 1) = 10 * (0.25 - 1) = -7.5 dB
    float gr = gc.process(-10.f);
    REQUIRE_THAT(gr, WithinAbs(-7.5f, 0.01f));
}

TEST_CASE("GainComputer: soft knee interpolation", "[dynamics][gain_computer]")
{
    libdsp::dynamics::GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(4.f);
    gc.setKnee(6.f); // 6 dB knee

    // At threshold exactly (-20 dB), soft knee should give partial GR
    float grAtThreshold = gc.process(-20.f);
    // Should be between 0 and full hard-knee GR
    REQUIRE(grAtThreshold < 0.f);
    REQUIRE(grAtThreshold > -7.5f);

    // Well above knee (e.g., -10 dB, overDb=10 > halfKnee=3), should match hard knee
    float grAbove = gc.process(-10.f);
    REQUIRE_THAT(grAbove, WithinAbs(-7.5f, 0.01f));

    // Well below knee (e.g., -30 dB), no reduction
    float grBelow = gc.process(-30.f);
    REQUIRE(grBelow == 0.f);
}

TEST_CASE("GainComputer: infinite ratio acts as limiter", "[dynamics][gain_computer]")
{
    libdsp::dynamics::GainComputer gc;
    gc.setThreshold(-10.f);
    gc.setRatio(1000.f); // near-infinite
    gc.setKnee(0.f);

    // 20 dB over threshold → GR ≈ -20 dB (limiter behavior)
    float gr = gc.process(10.f);
    REQUIRE_THAT(gr, WithinAbs(-20.f, 0.1f));
}

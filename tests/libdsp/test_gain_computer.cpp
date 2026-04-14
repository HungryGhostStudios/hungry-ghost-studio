#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/dynamics/GainComputer.h>

#include <cmath>

using namespace libdsp::dynamics;
using Catch::Matchers::WithinAbs;

TEST_CASE("GainComputer: no reduction below threshold", "[dynamics][gain_computer]")
{
    GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(4.f);
    gc.setKnee(0.f); // Hard knee

    // Signal at -30 dB — well below -20 dB threshold
    float gr = gc.process(-30.f);
    REQUIRE_THAT(gr, WithinAbs(0.0, 1e-6));

    // Signal at -21 dB — still below threshold
    gr = gc.process(-21.f);
    REQUIRE_THAT(gr, WithinAbs(0.0, 1e-6));
}

TEST_CASE("GainComputer: correct GR above threshold with hard knee", "[dynamics][gain_computer]")
{
    GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(4.f);
    gc.setKnee(0.f);

    // Signal at 0 dB — 20 dB above threshold
    // GR = overDb * (1/ratio - 1) = 20 * (0.25 - 1) = 20 * -0.75 = -15 dB
    float gr = gc.process(0.f);
    REQUIRE_THAT(gr, WithinAbs(-15.0, 0.01));

    // Signal at -10 dB — 10 dB above threshold
    // GR = 10 * (0.25 - 1) = -7.5 dB
    gr = gc.process(-10.f);
    REQUIRE_THAT(gr, WithinAbs(-7.5, 0.01));
}

TEST_CASE("GainComputer: ratio of 1 means no compression", "[dynamics][gain_computer]")
{
    GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(1.f);
    gc.setKnee(0.f);

    // Even above threshold, ratio=1 means GR = overDb * (1/1 - 1) = 0
    float gr = gc.process(0.f);
    REQUIRE_THAT(gr, WithinAbs(0.0, 1e-6));
}

TEST_CASE("GainComputer: infinite ratio (limiter)", "[dynamics][gain_computer]")
{
    GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(1000.f); // Approximating infinity
    gc.setKnee(0.f);

    // Signal at 0 dB — GR = 20 * (1/1000 - 1) ≈ -20 dB
    float gr = gc.process(0.f);
    REQUIRE(gr < -19.9f);
    REQUIRE(gr > -20.1f);
}

TEST_CASE("GainComputer: soft knee blends smoothly", "[dynamics][gain_computer]")
{
    GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(4.f);
    gc.setKnee(6.f); // 6 dB soft knee

    // Below knee region (below threshold - halfKnee = -23 dB)
    float gr_below = gc.process(-24.f);
    REQUIRE_THAT(gr_below, WithinAbs(0.0, 1e-6));

    // In knee region (-23 to -17 dB)
    float gr_knee = gc.process(-20.f);
    REQUIRE(gr_knee < 0.f);
    REQUIRE(gr_knee > -15.f); // Less GR than hard knee at same level

    // Above knee region (above -17 dB)
    float gr_above = gc.process(0.f);
    REQUIRE(gr_above < -14.f); // Similar to hard knee for far-above-threshold signals
}

TEST_CASE("GainComputer: GR is always non-positive", "[dynamics][gain_computer]")
{
    GainComputer gc;
    gc.setThreshold(-20.f);
    gc.setRatio(4.f);
    gc.setKnee(6.f);

    for (float dB = -60.f; dB <= 20.f; dB += 1.f)
    {
        float gr = gc.process(dB);
        REQUIRE(gr <= 0.f);
    }
}

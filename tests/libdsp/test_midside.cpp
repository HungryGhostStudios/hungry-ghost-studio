#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/util/MidSide.h>

using Catch::Matchers::WithinAbs;

TEST_CASE("MidSide encode/decode round-trip preserves signal", "[MidSide]")
{
    const float left = 0.7f;
    const float right = -0.3f;

    auto [mid, side] = libdsp::MidSide::encode(left, right);
    auto [outLeft, outRight] = libdsp::MidSide::decode(mid, side);

    REQUIRE_THAT(outLeft, WithinAbs(left, 1e-6f));
    REQUIRE_THAT(outRight, WithinAbs(right, 1e-6f));
}

TEST_CASE("MidSide pure-left input produces equal M and S", "[MidSide]")
{
    auto [mid, side] = libdsp::MidSide::encode(1.0f, 0.0f);

    // mid = (1+0)/2 = 0.5, side = (1-0)/2 = 0.5
    REQUIRE_THAT(mid, WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(side, WithinAbs(0.5f, 1e-6f));
}

TEST_CASE("MidSide mono input produces zero side signal", "[MidSide]")
{
    const float mono = 0.6f;
    auto [mid, side] = libdsp::MidSide::encode(mono, mono);

    REQUIRE_THAT(side, WithinAbs(0.0f, 1e-6f));
    REQUIRE_THAT(mid, WithinAbs(mono, 1e-6f));
}

TEST_CASE("MidSide silence in produces silence out", "[MidSide]")
{
    auto [mid, side] = libdsp::MidSide::encode(0.0f, 0.0f);

    REQUIRE_THAT(mid, WithinAbs(0.0f, 1e-6f));
    REQUIRE_THAT(side, WithinAbs(0.0f, 1e-6f));
}

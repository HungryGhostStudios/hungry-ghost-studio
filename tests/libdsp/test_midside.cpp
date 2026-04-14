#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <libdsp/util/MidSide.h>

using Catch::Matchers::WithinAbs;

TEST_CASE("MidSide encode/decode round-trip preserves signal", "[MidSide]")
{
    const float left = 0.7f;
    const float right = -0.3f;

    auto [mid, side] = libdsp::MidSide::encode(left, right);
    auto [outL, outR] = libdsp::MidSide::decode(mid, side);

    REQUIRE_THAT(outL, WithinAbs(left, 1e-6f));
    REQUIRE_THAT(outR, WithinAbs(right, 1e-6f));
}

TEST_CASE("MidSide pure-left input produces equal mid and side", "[MidSide]")
{
    auto [mid, side] = libdsp::MidSide::encode(1.0f, 0.0f);

    // L=1, R=0 → M = 0.5, S = 0.5
    REQUIRE_THAT(mid, WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(side, WithinAbs(0.5f, 1e-6f));
}

TEST_CASE("MidSide mono input produces zero side signal", "[MidSide]")
{
    auto [mid, side] = libdsp::MidSide::encode(0.5f, 0.5f);

    REQUIRE_THAT(mid, WithinAbs(0.5f, 1e-6f));
    REQUIRE_THAT(side, WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("MidSide decode of zero side gives mono", "[MidSide]")
{
    auto [left, right] = libdsp::MidSide::decode(0.8f, 0.0f);

    REQUIRE_THAT(left, WithinAbs(right, 1e-6f));
    REQUIRE_THAT(left, WithinAbs(0.8f, 1e-6f));
}

#pragma once

namespace libdsp {

/// Cross-platform pi constant — avoids reliance on non-standard M_PI (undefined on MSVC by default).
static constexpr double kPi  = 3.14159265358979323846;
static constexpr float  kPiF = 3.14159265358979323846f;

} // namespace libdsp

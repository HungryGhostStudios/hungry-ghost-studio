#pragma once

#include <cmath>
#include <utility>

namespace libdsp {

class MidSide
{
public:
    static std::pair<float, float> encode(float left, float right) noexcept
    {
        const float mid  = (left + right) * 0.5f;
        const float side = (left - right) * 0.5f;
        return {mid, side};
    }

    static std::pair<float, float> decode(float mid, float side) noexcept
    {
        const float left  = mid + side;
        const float right = mid - side;
        return {left, right};
    }
};

} // namespace libdsp

#pragma once

#include <cmath>
#include <functional>

namespace libdsp {

/**
 * First-order Antiderivative Anti-Aliasing (ADAA) wrapper.
 * Takes a nonlinear function F(x) and its first antiderivative AD1(x).
 * Reduces aliasing from nonlinear processing by computing
 * (AD1(x) - AD1(x_prev)) / (x - x_prev) instead of F(x).
 * No JUCE dependency.
 */
template <typename FuncType, typename AntiderivType>
class ADAA {
public:
    ADAA(FuncType func, AntiderivType antideriv)
        : m_func(func), m_antideriv(antideriv) {}

    float process(float x) {
        float result;
        const float diff = x - m_xPrev;

        if (std::fabs(diff) < 1e-5f) {
            // When input barely changes, fall back to direct evaluation
            // to avoid division by near-zero
            result = m_func(0.5f * (x + m_xPrev));
        } else {
            result = (m_antideriv(x) - m_antideriv(m_xPrev)) / diff;
        }

        m_xPrev = x;
        return result;
    }

    void reset() {
        m_xPrev = 0.0f;
    }

private:
    FuncType m_func;
    AntiderivType m_antideriv;
    float m_xPrev = 0.0f;
};

/**
 * Helper to create ADAA instances with lambda functions.
 */
template <typename F, typename AF>
ADAA<F, AF> makeADAA(F func, AF antideriv) {
    return ADAA<F, AF>(func, antideriv);
}

} // namespace libdsp

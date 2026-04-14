#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace libdsp {

/**
 * Fractional delay line with linear and cubic (Hermite) interpolation.
 * Pre-allocates buffer in prepare(). No JUCE dependency.
 */
class DelayLine {
public:
    enum class Interpolation {
        Linear,
        Cubic
    };

    DelayLine() = default;

    void prepare(double sampleRate, int maxDelaySamples) {
        m_sampleRate = sampleRate;
        m_maxDelay = maxDelaySamples;
        // Add extra samples for cubic interpolation
        m_buffer.resize(static_cast<size_t>(maxDelaySamples + 4), 0.0f);
        m_writePos = 0;
        m_delaySamples = 0.0f;
    }

    /** Set delay in fractional samples. */
    void setDelay(float delaySamples) {
        m_delaySamples = (delaySamples < 0.0f) ? 0.0f :
                         (delaySamples > static_cast<float>(m_maxDelay)) ?
                         static_cast<float>(m_maxDelay) : delaySamples;
    }

    /** Set delay in milliseconds (convenience). */
    void setDelayMs(float delayMs) {
        setDelay(delayMs * 0.001f * static_cast<float>(m_sampleRate));
    }

    /** Set interpolation mode. */
    void setInterpolation(Interpolation mode) {
        m_interpolation = mode;
    }

    /** Push a sample into the delay line. */
    void push(float sample) {
        m_buffer[static_cast<size_t>(m_writePos)] = sample;
        m_writePos = (m_writePos + 1) % static_cast<int>(m_buffer.size());
    }

    /** Read from the delay line at the set delay time. */
    float read() const {
        return readAt(m_delaySamples);
    }

    /** Read from the delay line at a specific fractional delay. */
    float readAt(float delaySamples) const {
        if (m_buffer.empty()) return 0.0f;

        const int bufSize = static_cast<int>(m_buffer.size());
        const float readPos = static_cast<float>(m_writePos) - delaySamples - 1.0f;

        if (m_interpolation == Interpolation::Linear) {
            return readLinear(readPos, bufSize);
        } else {
            return readCubic(readPos, bufSize);
        }
    }

    /** Reset the delay line (clear buffer). */
    void reset() {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_writePos = 0;
    }

private:
    float readLinear(float readPos, int bufSize) const {
        const int idx0 = wrapIndex(static_cast<int>(std::floor(readPos)), bufSize);
        const int idx1 = wrapIndex(idx0 + 1, bufSize);
        const float frac = readPos - std::floor(readPos);

        return m_buffer[static_cast<size_t>(idx0)] * (1.0f - frac)
             + m_buffer[static_cast<size_t>(idx1)] * frac;
    }

    float readCubic(float readPos, int bufSize) const {
        const int idx1 = wrapIndex(static_cast<int>(std::floor(readPos)), bufSize);
        const int idx0 = wrapIndex(idx1 - 1, bufSize);
        const int idx2 = wrapIndex(idx1 + 1, bufSize);
        const int idx3 = wrapIndex(idx1 + 2, bufSize);
        const float frac = readPos - std::floor(readPos);

        const float y0 = m_buffer[static_cast<size_t>(idx0)];
        const float y1 = m_buffer[static_cast<size_t>(idx1)];
        const float y2 = m_buffer[static_cast<size_t>(idx2)];
        const float y3 = m_buffer[static_cast<size_t>(idx3)];

        // Hermite interpolation
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    int wrapIndex(int index, int bufSize) const {
        return ((index % bufSize) + bufSize) % bufSize;
    }

    double m_sampleRate = 44100.0;
    int m_maxDelay = 0;
    int m_writePos = 0;
    float m_delaySamples = 0.0f;
    Interpolation m_interpolation = Interpolation::Linear;
    std::vector<float> m_buffer;
};

} // namespace libdsp

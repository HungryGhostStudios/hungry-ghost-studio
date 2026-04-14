#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace libdsp {

/**
 * Delay line optimized for modulated (fractional) delays.
 * Uses cubic Hermite interpolation for artifact-free modulated delays.
 * No JUCE dependency.
 */
class FractionalDelay {
public:
    FractionalDelay() = default;

    void prepare(double sampleRate, float maxDelayMs) {
        m_sampleRate = sampleRate;
        int maxSamples = static_cast<int>(std::ceil(maxDelayMs * 0.001 * sampleRate));
        // +4 for cubic interpolation guard samples
        m_buffer.resize(static_cast<size_t>(maxSamples + 4), 0.0f);
        m_maxDelaySamples = static_cast<float>(maxSamples);
        m_writePos = 0;
        m_delaySamples = 0.0f;
    }

    /** Set delay in fractional samples. */
    void setDelay(float delaySamples) {
        m_delaySamples = (delaySamples < 0.0f) ? 0.0f :
                         (delaySamples > m_maxDelaySamples) ?
                         m_maxDelaySamples : delaySamples;
    }

    /** Set delay in milliseconds (convenience). */
    void setDelayMs(float delayMs) {
        setDelay(static_cast<float>(delayMs * 0.001 * m_sampleRate));
    }

    /** Write a sample and read the delayed output using cubic interpolation. */
    float process(float sample) {
        // Write input
        m_buffer[static_cast<size_t>(m_writePos)] = sample;

        // Read with cubic Hermite interpolation
        float output = readCubic(m_delaySamples);

        // Advance write position
        m_writePos = (m_writePos + 1) % static_cast<int>(m_buffer.size());

        return output;
    }

    void reset() {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_writePos = 0;
    }

private:
    float readCubic(float delaySamples) const {
        if (m_buffer.empty()) return 0.0f;

        const int bufSize = static_cast<int>(m_buffer.size());
        const float readPos = static_cast<float>(m_writePos) - delaySamples;

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
    int m_writePos = 0;
    float m_delaySamples = 0.0f;
    float m_maxDelaySamples = 0.0f;
    std::vector<float> m_buffer;
};

} // namespace libdsp

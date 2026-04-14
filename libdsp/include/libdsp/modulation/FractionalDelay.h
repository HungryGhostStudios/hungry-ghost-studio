#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace libdsp {

/**
 * Fractional delay line with cubic Hermite interpolation.
 * Designed for modulation effects (chorus, flanger, vibrato).
 * Pure C++17, no JUCE dependency.
 */
class FractionalDelay {
public:
    FractionalDelay() = default;

    /**
     * Prepare the delay line.
     * @param sampleRate Current sample rate in Hz.
     * @param maxDelaySamples Maximum delay in samples.
     */
    void prepare(double sampleRate, int maxDelaySamples) {
        m_sampleRate = sampleRate;
        m_maxDelay = maxDelaySamples;
        // Extra samples for cubic interpolation
        m_buffer.resize(static_cast<size_t>(maxDelaySamples + 4), 0.0f);
        m_writePos = 0;
        m_delaySamples = 0.0f;
    }

    /** Set delay in fractional samples. Clamped to [0, maxDelay]. */
    void setDelay(float delaySamples) {
        m_delaySamples = (delaySamples < 0.0f) ? 0.0f :
                         (delaySamples > static_cast<float>(m_maxDelay)) ?
                         static_cast<float>(m_maxDelay) : delaySamples;
    }

    /** Set delay in milliseconds. */
    void setDelayMs(float delayMs) {
        setDelay(delayMs * 0.001f * static_cast<float>(m_sampleRate));
    }

    /**
     * Process one sample through the fractional delay line.
     * Pushes the input sample and reads the delayed output using cubic Hermite interpolation.
     * @param sample Input sample.
     * @return Delayed output sample.
     */
    float process(float sample) {
        // Write input to buffer
        m_buffer[static_cast<size_t>(m_writePos)] = sample;

        // Read with cubic Hermite interpolation
        const int bufSize = static_cast<int>(m_buffer.size());
        const float readPos = static_cast<float>(m_writePos) - m_delaySamples;

        const int idx1 = wrapIndex(static_cast<int>(std::floor(readPos)), bufSize);
        const int idx0 = wrapIndex(idx1 - 1, bufSize);
        const int idx2 = wrapIndex(idx1 + 1, bufSize);
        const int idx3 = wrapIndex(idx1 + 2, bufSize);
        const float frac = readPos - std::floor(readPos);

        const float y0 = m_buffer[static_cast<size_t>(idx0)];
        const float y1 = m_buffer[static_cast<size_t>(idx1)];
        const float y2 = m_buffer[static_cast<size_t>(idx2)];
        const float y3 = m_buffer[static_cast<size_t>(idx3)];

        // Hermite interpolation coefficients
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        const float output = ((c3 * frac + c2) * frac + c1) * frac + c0;

        // Advance write position
        m_writePos = (m_writePos + 1) % bufSize;

        return output;
    }

    /** Reset the delay line (clear buffer). */
    void reset() {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_writePos = 0;
    }

private:
    int wrapIndex(int index, int bufSize) const {
        return ((index % bufSize) + bufSize) % bufSize;
    }

    double m_sampleRate = 44100.0;
    int m_maxDelay = 0;
    int m_writePos = 0;
    float m_delaySamples = 0.0f;
    std::vector<float> m_buffer;
};

} // namespace libdsp

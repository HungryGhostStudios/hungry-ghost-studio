#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace libdsp {

/**
 * Oversampler supporting 1x, 2x, 4x, 8x oversampling with half-band FIR
 * filters for up/downsampling. No JUCE dependency.
 */
class Oversampler {
public:
    Oversampler() = default;

    void prepare(double sampleRate, int blockSize, int factor) {
        m_sampleRate = sampleRate;
        m_factor = (factor < 2) ? 1 : (factor <= 2 ? 2 : (factor <= 4 ? 4 : 8));
        m_blockSize = blockSize;
        m_oversampledBuffer.resize(static_cast<size_t>(blockSize * m_factor), 0.0f);
        initFilters();
    }

    /** Upsample input buffer (blockSize samples) into internal oversampled buffer. */
    float* upsample(const float* input, int numSamples) {
        if (m_factor == 1) {
            for (int i = 0; i < numSamples; ++i)
                m_oversampledBuffer[static_cast<size_t>(i)] = input[i];
            return m_oversampledBuffer.data();
        }

        // Zero-stuff
        const auto osSize = static_cast<size_t>(numSamples * m_factor);
        for (size_t i = 0; i < osSize; ++i)
            m_oversampledBuffer[i] = 0.0f;

        for (int i = 0; i < numSamples; ++i)
            m_oversampledBuffer[static_cast<size_t>(i * m_factor)] = input[i] * static_cast<float>(m_factor);

        // Apply half-band lowpass filter
        applyFilter(m_oversampledBuffer.data(), static_cast<int>(osSize));
        return m_oversampledBuffer.data();
    }

    /** Downsample internal oversampled buffer back to original sample rate. */
    void downsample(float* output, int numSamples) {
        if (m_factor == 1) {
            for (int i = 0; i < numSamples; ++i)
                output[i] = m_oversampledBuffer[static_cast<size_t>(i)];
            return;
        }

        // Apply half-band lowpass filter before decimation
        applyFilter(m_oversampledBuffer.data(), numSamples * m_factor);

        // Decimate
        for (int i = 0; i < numSamples; ++i)
            output[i] = m_oversampledBuffer[static_cast<size_t>(i * m_factor)];
    }

    /** Get pointer to internal oversampled buffer for in-place processing. */
    float* getOversampledBuffer() { return m_oversampledBuffer.data(); }

    int getOversampledBlockSize(int inputSize) const { return inputSize * m_factor; }
    int getFactor() const { return m_factor; }
    double getOversampledSampleRate() const { return m_sampleRate * m_factor; }

private:
    // Half-band FIR filter coefficients (7-tap)
    static constexpr int kFilterOrder = 7;
    static constexpr std::array<float, kFilterOrder> kHalfBandCoeffs = {
        0.007568359375f, 0.0f, -0.068359375f, 0.5703125f,
        -0.068359375f, 0.0f, 0.007568359375f
    };

    void initFilters() {
        m_filterState.resize(kFilterOrder, 0.0f);
    }

    void applyFilter(float* buffer, int numSamples) {
        // Simple FIR convolution
        for (int i = 0; i < numSamples; ++i) {
            // Shift delay line
            for (int j = kFilterOrder - 1; j > 0; --j)
                m_filterState[static_cast<size_t>(j)] = m_filterState[static_cast<size_t>(j - 1)];
            m_filterState[0] = buffer[i];

            // Compute output
            float sum = 0.0f;
            for (int j = 0; j < kFilterOrder; ++j)
                sum += kHalfBandCoeffs[static_cast<size_t>(j)] * m_filterState[static_cast<size_t>(j)];
            buffer[i] = sum;
        }
    }

    double m_sampleRate = 44100.0;
    int m_factor = 1;
    int m_blockSize = 512;
    std::vector<float> m_oversampledBuffer;
    std::vector<float> m_filterState;
};

} // namespace libdsp

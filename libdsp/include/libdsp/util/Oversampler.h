#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cassert>

namespace libdsp {

class Oversampler {
public:
    // factor: 2, 4, or 8
    void prepare(int oversamplingFactor, int blockSize, double sampleRate)
    {
        assert(oversamplingFactor == 2 || oversamplingFactor == 4 || oversamplingFactor == 8);
        factor = oversamplingFactor;
        maxBlockSize = blockSize;
        fs = sampleRate;

        upsampledBuffer.resize(static_cast<size_t>(blockSize * factor), 0.f);
        downsampledBuffer.resize(static_cast<size_t>(blockSize), 0.f);

        // Half-band FIR filter coefficients (7-tap)
        // Designed for ~-60dB stopband attenuation
        halfBandCoeffs = { 0.00390625f, 0.f, -0.0703125f, 0.f,
                           0.5703125f, 1.f, 0.5703125f, 0.f,
                           -0.0703125f, 0.f, 0.00390625f };
        halfBandLen = static_cast<int>(halfBandCoeffs.size());
        halfBandCenter = halfBandLen / 2;

        // Filter state for each stage (max 3 stages for 8x)
        int numStages = 0;
        int f = factor;
        while (f > 1) { ++numStages; f /= 2; }

        filterStatesUp.resize(static_cast<size_t>(numStages));
        filterStatesDown.resize(static_cast<size_t>(numStages));
        for (auto& s : filterStatesUp)
            s.assign(static_cast<size_t>(halfBandLen), 0.f);
        for (auto& s : filterStatesDown)
            s.assign(static_cast<size_t>(halfBandLen), 0.f);
    }

    // Upsample input and return pointer to upsampled buffer
    float* getUpsampledBuffer(const float* input, int numSamples)
    {
        assert(numSamples <= maxBlockSize);

        // Stage 1: zero-stuff by 2, then filter
        int currentLen = numSamples;
        const float* currentInput = input;

        // Copy input to temp
        std::vector<float> temp(input, input + numSamples);

        int f = factor;
        while (f > 1)
        {
            int newLen = currentLen * 2;
            std::vector<float> stuffed(static_cast<size_t>(newLen), 0.f);

            // Zero-stuff: insert zeros between samples
            for (int i = 0; i < currentLen; ++i)
                stuffed[static_cast<size_t>(i * 2)] = temp[static_cast<size_t>(i)] * 2.f;

            // Apply half-band FIR filter
            temp.resize(static_cast<size_t>(newLen));
            for (int i = 0; i < newLen; ++i)
            {
                float sum = 0.f;
                for (int k = 0; k < halfBandLen; ++k)
                {
                    int idx = i - halfBandCenter + k;
                    if (idx >= 0 && idx < newLen)
                        sum += stuffed[static_cast<size_t>(idx)] * halfBandCoeffs[static_cast<size_t>(k)];
                }
                temp[static_cast<size_t>(i)] = sum;
            }

            currentLen = newLen;
            f /= 2;
        }

        std::copy(temp.begin(), temp.begin() + currentLen, upsampledBuffer.begin());
        return upsampledBuffer.data();
    }

    // Downsample processed buffer back to original rate
    void downsample(const float* processed, float* output, int numSamples)
    {
        int currentLen = numSamples * factor;
        std::vector<float> temp(processed, processed + currentLen);

        int f = factor;
        while (f > 1)
        {
            int newLen = currentLen / 2;

            // Apply half-band FIR filter
            std::vector<float> filtered(static_cast<size_t>(currentLen));
            for (int i = 0; i < currentLen; ++i)
            {
                float sum = 0.f;
                for (int k = 0; k < halfBandLen; ++k)
                {
                    int idx = i - halfBandCenter + k;
                    if (idx >= 0 && idx < currentLen)
                        sum += temp[static_cast<size_t>(idx)] * halfBandCoeffs[static_cast<size_t>(k)];
                }
                filtered[static_cast<size_t>(i)] = sum;
            }

            // Decimate: take every other sample
            temp.resize(static_cast<size_t>(newLen));
            for (int i = 0; i < newLen; ++i)
                temp[static_cast<size_t>(i)] = filtered[static_cast<size_t>(i * 2)];

            currentLen = newLen;
            f /= 2;
        }

        std::copy(temp.begin(), temp.begin() + numSamples, output);
    }

    int getLatencySamples() const
    {
        // Each stage adds halfBandCenter samples of latency
        int stages = 0;
        int f = factor;
        while (f > 1) { ++stages; f /= 2; }
        return halfBandCenter * stages;
    }

    int getFactor() const { return factor; }

private:
    int factor = 1;
    int maxBlockSize = 0;
    double fs = 44100.0;

    std::vector<float> upsampledBuffer;
    std::vector<float> downsampledBuffer;

    std::vector<float> halfBandCoeffs;
    int halfBandLen = 0;
    int halfBandCenter = 0;

    std::vector<std::vector<float>> filterStatesUp;
    std::vector<std::vector<float>> filterStatesDown;
};

} // namespace libdsp

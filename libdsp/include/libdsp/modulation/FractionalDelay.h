#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace libdsp {

class FractionalDelay {
public:
    FractionalDelay() = default;

    void prepare(double sampleRate, float maxDelayMs) {
        sampleRate_ = sampleRate;
        int maxSamples = static_cast<int>(std::ceil(maxDelayMs * 0.001 * sampleRate)) + 4;
        buffer_.resize(static_cast<size_t>(maxSamples), 0.0f);
        bufferSize_ = maxSamples;
        writePos_ = 0;
        delaySamples_ = 0.0f;
    }

    /// Set delay in fractional samples
    void setDelay(float delaySamples) {
        delaySamples_ = std::max(0.0f, delaySamples);
    }

    /// Set delay in milliseconds
    void setDelayMs(float ms) {
        setDelay(static_cast<float>(ms * 0.001 * sampleRate_));
    }

    /// Process one sample: writes input to buffer, returns delayed output using cubic interpolation
    float process(float input) {
        // Write input to circular buffer
        buffer_[static_cast<size_t>(writePos_)] = input;

        // Read with cubic Hermite interpolation
        float output = readInterpolated(delaySamples_);

        // Advance write position
        writePos_++;
        if (writePos_ >= bufferSize_)
            writePos_ = 0;

        return output;
    }

    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        writePos_ = 0;
    }

private:
    float readInterpolated(float delaySamples) const {
        float readPos = static_cast<float>(writePos_) - delaySamples;

        int index = static_cast<int>(std::floor(readPos));
        float frac = readPos - static_cast<float>(index);

        // Get 4 surrounding samples for cubic interpolation (y0, y1, y2, y3)
        float y0 = readBuffer(index - 1);
        float y1 = readBuffer(index);
        float y2 = readBuffer(index + 1);
        float y3 = readBuffer(index + 2);

        // Cubic Hermite interpolation
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    float readBuffer(int index) const {
        // Wrap index into valid range
        while (index < 0)
            index += bufferSize_;
        while (index >= bufferSize_)
            index -= bufferSize_;
        return buffer_[static_cast<size_t>(index)];
    }

    double sampleRate_ = 44100.0;
    std::vector<float> buffer_;
    int bufferSize_ = 0;
    int writePos_ = 0;
    float delaySamples_ = 0.0f;
};

} // namespace libdsp

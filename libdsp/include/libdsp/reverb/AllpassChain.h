#pragma once

#include <array>
#include <cmath>
#include <vector>

namespace libdsp {

/**
 * Series allpass sections for early reflection diffusion.
 * Each stage is a Schroeder allpass: y[n] = -g*x[n] + x[n-d] + g*y[n-d]
 * No JUCE dependency.
 */
class AllpassChain {
public:
    static constexpr int kMaxStages = 8;

    AllpassChain() = default;

    void prepare(double sampleRate, int maxDelaySamples = 4096) {
        m_sampleRate = sampleRate;
        for (auto& stage : m_stages) {
            stage.buffer.assign(static_cast<size_t>(maxDelaySamples), 0.0f);
            stage.writePos = 0;
        }
    }

    /** Set the number of active allpass stages (1..kMaxStages). */
    void setNumStages(int n) {
        m_numStages = (n < 1) ? 1 : (n > kMaxStages) ? kMaxStages : n;
    }

    /** Set delay (in samples) and feedback gain for a specific stage. */
    void setStageParams(int stageIndex, int delaySamples, float gain) {
        if (stageIndex < 0 || stageIndex >= kMaxStages) return;
        m_stages[static_cast<size_t>(stageIndex)].delay = delaySamples;
        m_stages[static_cast<size_t>(stageIndex)].gain = gain;
    }

    /** Process one sample through the allpass chain. */
    float process(float input) {
        float x = input;
        for (int i = 0; i < m_numStages; ++i) {
            x = processStage(m_stages[static_cast<size_t>(i)], x);
        }
        return x;
    }

    void reset() {
        for (auto& stage : m_stages) {
            std::fill(stage.buffer.begin(), stage.buffer.end(), 0.0f);
            stage.writePos = 0;
        }
    }

private:
    struct Stage {
        std::vector<float> buffer;
        int writePos = 0;
        int delay = 113;    // prime default
        float gain = 0.5f;
    };

    float processStage(Stage& s, float input) {
        if (s.buffer.empty()) return input;

        const int bufSize = static_cast<int>(s.buffer.size());
        const int readPos = ((s.writePos - s.delay) % bufSize + bufSize) % bufSize;
        const float delayed = s.buffer[static_cast<size_t>(readPos)];

        // Schroeder allpass: y = -g*x + delayed + g*delayed_output
        // Using the nested form: v = x + g * delayed; y = -g * v + delayed
        const float v = input + s.gain * delayed;
        const float output = -s.gain * v + delayed;

        // Denormal protection
        constexpr float kDenormalGuard = 1.0e-20f;
        s.buffer[static_cast<size_t>(s.writePos)] = v + kDenormalGuard - kDenormalGuard;

        s.writePos = (s.writePos + 1) % bufSize;
        return output;
    }

    double m_sampleRate = 44100.0;
    int m_numStages = 4;
    std::array<Stage, kMaxStages> m_stages;
};

} // namespace libdsp

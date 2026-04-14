#pragma once

#include <libdsp/modulation/LFO.h>
#include <libdsp/modulation/FractionalDelay.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace libdsp {

class Chorus {
public:
    enum class Mode { Clean, BBD };

    Chorus() = default;

    void prepare(double sampleRate, int blockSize) {
        m_sampleRate = sampleRate;
        m_blockSize = blockSize;

        // Max delay = base delay + max depth + margin
        const float maxDelayMs = 50.0f;

        for (int i = 0; i < kMaxVoices; ++i) {
            m_delayL[i].prepare(sampleRate, maxDelayMs);
            m_delayR[i].prepare(sampleRate, maxDelayMs);
            m_lfoL[i].prepare(sampleRate);
            m_lfoR[i].prepare(sampleRate);
        }

        m_rng.seed(42);
        m_feedbackL = 0.0f;
        m_feedbackR = 0.0f;
    }

    void setVoices(int voices) {
        m_numVoices = std::clamp(voices, 2, kMaxVoices);
    }

    void setRate(float hz) {
        m_rateHz = hz;
    }

    void setDepth(float ms) {
        m_depthMs = ms;
    }

    void setDelay(float ms) {
        m_delayMs = ms;
    }

    void setFeedback(float amount) {
        m_feedback = std::clamp(amount, -0.95f, 0.95f);
    }

    void setMode(Mode mode) {
        m_mode = mode;
    }

    void setSpread(float spread) {
        m_spread = std::clamp(spread, 0.0f, 1.0f);
    }

    struct StereoSample {
        float left = 0.0f;
        float right = 0.0f;
    };

    StereoSample process(float left, float right) {
        // Update LFO rates and phase offsets
        for (int v = 0; v < m_numVoices; ++v) {
            float phaseOffset = static_cast<float>(v) / static_cast<float>(m_numVoices);
            m_lfoL[v].setFrequency(m_rateHz);
            m_lfoL[v].setWaveform(LFO::Waveform::Sine);
            m_lfoL[v].setPhase(phaseOffset);

            m_lfoR[v].setFrequency(m_rateHz);
            m_lfoR[v].setWaveform(LFO::Waveform::Sine);
            m_lfoR[v].setPhase(phaseOffset);
        }

        // Add feedback with denormal protection
        float inputL = left + m_feedbackL * m_feedback + kDenormalDC;
        float inputR = right + m_feedbackR * m_feedback + kDenormalDC;

        float outL = 0.0f;
        float outR = 0.0f;

        for (int v = 0; v < m_numVoices; ++v) {
            // LFO modulates delay time
            float lfoValL = m_lfoL[v].process();
            float lfoValR = m_lfoR[v].process();

            float delayL = m_delayMs + lfoValL * m_depthMs;
            float delayR = m_delayMs + lfoValR * m_depthMs;

            // Clamp to positive values
            delayL = std::max(delayL, 0.1f);
            delayR = std::max(delayR, 0.1f);

            // Convert ms to samples
            float delaySamplesL = static_cast<float>(delayL * 0.001 * m_sampleRate);
            float delaySamplesR = static_cast<float>(delayR * 0.001 * m_sampleRate);

            m_delayL[v].setDelay(delaySamplesL);
            m_delayR[v].setDelay(delaySamplesR);

            float wetL = m_delayL[v].process(inputL);
            float wetR = m_delayR[v].process(inputR);

            // BBD mode: add noise and sample-rate reduction
            if (m_mode == Mode::BBD) {
                // Sample-rate reduction based on "stage count"
                // BBD chips have limited bandwidth; simulate with sample-hold
                int stageCount = 256 + v * 256; // More stages per voice
                float bbdRate = static_cast<float>(m_sampleRate) / static_cast<float>(stageCount);
                float holdSamples = static_cast<float>(m_sampleRate) / std::max(bbdRate, 1.0f);

                m_bbdCounterL[v] += 1.0f;
                if (m_bbdCounterL[v] >= holdSamples) {
                    m_bbdHoldL[v] = wetL;
                    m_bbdCounterL[v] = 0.0f;
                }
                wetL = m_bbdHoldL[v];

                m_bbdCounterR[v] += 1.0f;
                if (m_bbdCounterR[v] >= holdSamples) {
                    m_bbdHoldR[v] = wetR;
                    m_bbdCounterR[v] = 0.0f;
                }
                wetR = m_bbdHoldR[v];

                // Add subtle noise
                float noise = m_noiseDist(m_rng) * m_bbdNoiseLevel;
                wetL += noise;
                wetR += noise;
            }

            // Stereo spread: distribute voices across L/R
            float pan = 0.5f; // center
            if (m_numVoices > 1 && m_spread > 0.0f) {
                // Spread voices from left to right
                float voicePos = static_cast<float>(v) / static_cast<float>(m_numVoices - 1);
                pan = 0.5f + (voicePos - 0.5f) * m_spread;
            }

            float gainL = std::cos(pan * static_cast<float>(M_PI) * 0.5f);
            float gainR = std::sin(pan * static_cast<float>(M_PI) * 0.5f);

            outL += wetL * gainL;
            outR += wetR * gainR;
        }

        // Normalize by voice count
        float norm = 1.0f / static_cast<float>(m_numVoices);
        outL *= norm;
        outR *= norm;

        // Store feedback (remove denormal DC)
        m_feedbackL = outL - kDenormalDC;
        m_feedbackR = outR - kDenormalDC;

        return { outL, outR };
    }

    void reset() {
        for (int i = 0; i < kMaxVoices; ++i) {
            m_delayL[i].reset();
            m_delayR[i].reset();
            m_lfoL[i].reset();
            m_lfoR[i].reset();
            m_bbdCounterL[i] = 0.0f;
            m_bbdCounterR[i] = 0.0f;
            m_bbdHoldL[i] = 0.0f;
            m_bbdHoldR[i] = 0.0f;
        }
        m_feedbackL = 0.0f;
        m_feedbackR = 0.0f;
    }

private:
    static constexpr int kMaxVoices = 6;
    static constexpr float kDenormalDC = 1e-25f;

    double m_sampleRate = 44100.0;
    int m_blockSize = 512;

    int m_numVoices = 2;
    float m_rateHz = 0.5f;
    float m_depthMs = 3.0f;
    float m_delayMs = 7.0f;
    float m_feedback = 0.0f;
    float m_spread = 1.0f;
    Mode m_mode = Mode::Clean;

    std::array<FractionalDelay, kMaxVoices> m_delayL;
    std::array<FractionalDelay, kMaxVoices> m_delayR;
    std::array<LFO, kMaxVoices> m_lfoL;
    std::array<LFO, kMaxVoices> m_lfoR;

    float m_feedbackL = 0.0f;
    float m_feedbackR = 0.0f;

    // BBD mode state
    float m_bbdNoiseLevel = 0.0005f;
    std::array<float, kMaxVoices> m_bbdCounterL = {};
    std::array<float, kMaxVoices> m_bbdCounterR = {};
    std::array<float, kMaxVoices> m_bbdHoldL = {};
    std::array<float, kMaxVoices> m_bbdHoldR = {};
    std::mt19937 m_rng{42};
    std::uniform_real_distribution<float> m_noiseDist{-1.0f, 1.0f};
};

} // namespace libdsp

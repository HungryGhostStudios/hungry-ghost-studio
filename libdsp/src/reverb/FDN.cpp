#include "libdsp/reverb/FDN.h"
#include <algorithm>
#include <cmath>

namespace libdsp {

// Mutually prime delay times (in samples at 44.1kHz baseline).
// These produce a dense, coloration-free reverb tail.
static constexpr std::array<int, FDN::kNumLines> kPrimeDelays = {
    1087, 1283, 1481, 1693, 1879, 2089, 2293, 2503
};

void FDN::prepare(double sampleRate, int /*blockSize*/) {
    m_sampleRate = sampleRate;

    // Scale prime delays by sample rate ratio
    const double srRatio = sampleRate / 44100.0;
    for (int i = 0; i < kNumLines; ++i) {
        m_baseDelays[static_cast<size_t>(i)] =
            std::max(1, static_cast<int>(std::round(kPrimeDelays[static_cast<size_t>(i)] * srRatio)));
    }

    // Prepare delay lines with enough headroom for modulation
    const int maxMod = static_cast<int>(0.01 * sampleRate); // 10ms max mod
    for (int i = 0; i < kNumLines; ++i) {
        m_delayLines[static_cast<size_t>(i)].prepare(
            sampleRate,
            m_baseDelays[static_cast<size_t>(i)] + maxMod + 4
        );
        m_delayLines[static_cast<size_t>(i)].setInterpolation(DelayLine::Interpolation::Linear);
    }

    updateFeedbackGains();
    updateDampingCoeffs();
    updateModPhaseInc();

    m_modPhase = 0.0f;
}

void FDN::setDecay(float seconds) {
    m_decay = std::max(0.1f, seconds);
    updateFeedbackGains();
}

void FDN::setDamping(float freqHz) {
    m_damping = std::max(200.0f, std::min(freqHz, 20000.0f));
    updateDampingCoeffs();
}

void FDN::setDiffusion(float amount) {
    m_diffusion = std::max(0.0f, std::min(amount, 1.0f));
}

void FDN::setModRate(float rateHz) {
    m_modRate = std::max(0.0f, std::min(rateHz, 5.0f));
    updateModPhaseInc();
}

void FDN::setModDepth(float depthMs) {
    m_modDepth = std::max(0.0f, std::min(depthMs, 5.0f)) *
                 0.001f * static_cast<float>(m_sampleRate);
}

void FDN::setSize(float size) {
    m_size = std::max(0.0f, std::min(size, 1.0f));
}

void FDN::setFreeze(bool freeze) {
    m_freeze = freeze;
}

float FDN::process(float input) {
    constexpr float kDenormalGuard = 1.0e-20f;

    // Read from all delay lines
    std::array<float, kNumLines> delayOuts{};
    for (int i = 0; i < kNumLines; ++i) {
        // Calculate modulated delay time
        const float baseDelay = static_cast<float>(m_baseDelays[static_cast<size_t>(i)]) * m_size;
        float modOffset = 0.0f;
        if (m_modDepth > 0.0f) {
            // Each line gets a phase-offset LFO
            const float phase = m_modPhase +
                static_cast<float>(i) / static_cast<float>(kNumLines);
            const float wrappedPhase = phase - std::floor(phase);
            modOffset = std::sin(wrappedPhase * 6.283185307f) * m_modDepth;
        }
        const float totalDelay = std::max(1.0f, baseDelay + modOffset);
        m_delayLines[static_cast<size_t>(i)].setDelay(totalDelay);
        delayOuts[static_cast<size_t>(i)] = m_delayLines[static_cast<size_t>(i)].read();
    }

    // Apply damping filters
    for (int i = 0; i < kNumLines; ++i) {
        if (!m_freeze) {
            delayOuts[static_cast<size_t>(i)] =
                m_dampingFilters[static_cast<size_t>(i)].process(delayOuts[static_cast<size_t>(i)]);
        }
    }

    // Apply feedback gains
    std::array<float, kNumLines> feedback{};
    for (int i = 0; i < kNumLines; ++i) {
        const float gain = m_freeze ? 1.0f : m_feedbackGains[static_cast<size_t>(i)];
        feedback[static_cast<size_t>(i)] = delayOuts[static_cast<size_t>(i)] * gain;
    }

    // Mix with Hadamard matrix (scaled by diffusion)
    hadamardMix(feedback);

    // Scale by diffusion amount (blend between mixed and direct)
    for (int i = 0; i < kNumLines; ++i) {
        feedback[static_cast<size_t>(i)] =
            delayOuts[static_cast<size_t>(i)] * (1.0f - m_diffusion) +
            feedback[static_cast<size_t>(i)] * m_diffusion;
    }

    // Feed back into delay lines with input injection
    const float inputScaled = input / std::sqrt(static_cast<float>(kNumLines));
    for (int i = 0; i < kNumLines; ++i) {
        const float sample = feedback[static_cast<size_t>(i)] + inputScaled + kDenormalGuard - kDenormalGuard;
        m_delayLines[static_cast<size_t>(i)].push(sample);
    }

    // Advance modulation phase
    m_modPhase += m_modPhaseInc;
    if (m_modPhase >= 1.0f) m_modPhase -= 1.0f;

    // Output: sum all delay outputs, scaled
    float output = 0.0f;
    for (int i = 0; i < kNumLines; ++i) {
        output += delayOuts[static_cast<size_t>(i)];
    }
    return output / std::sqrt(static_cast<float>(kNumLines));
}

void FDN::reset() {
    for (auto& dl : m_delayLines) dl.reset();
    for (auto& df : m_dampingFilters) df.reset();
    m_modPhase = 0.0f;
}

void FDN::hadamardMix(std::array<float, kNumLines>& data) {
    // In-place Hadamard transform for N=8 via butterfly stages
    // This is an energy-preserving orthogonal mixing matrix
    // Three stages of butterfly operations (log2(8) = 3)
    constexpr float kScale = 1.0f / 2.828427f; // 1/sqrt(8)

    // Stage 1: pairs (0,1), (2,3), (4,5), (6,7)
    for (int i = 0; i < kNumLines; i += 2) {
        const float a = data[static_cast<size_t>(i)];
        const float b = data[static_cast<size_t>(i + 1)];
        data[static_cast<size_t>(i)]     = a + b;
        data[static_cast<size_t>(i + 1)] = a - b;
    }

    // Stage 2: pairs (0,2), (1,3), (4,6), (5,7)
    for (int i = 0; i < kNumLines; i += 4) {
        for (int j = 0; j < 2; ++j) {
            const float a = data[static_cast<size_t>(i + j)];
            const float b = data[static_cast<size_t>(i + j + 2)];
            data[static_cast<size_t>(i + j)]     = a + b;
            data[static_cast<size_t>(i + j + 2)] = a - b;
        }
    }

    // Stage 3: pairs (0,4), (1,5), (2,6), (3,7)
    for (int j = 0; j < 4; ++j) {
        const float a = data[static_cast<size_t>(j)];
        const float b = data[static_cast<size_t>(j + 4)];
        data[static_cast<size_t>(j)]     = a + b;
        data[static_cast<size_t>(j + 4)] = a - b;
    }

    // Normalize for energy preservation
    for (int i = 0; i < kNumLines; ++i) {
        data[static_cast<size_t>(i)] *= kScale;
    }
}

void FDN::updateFeedbackGains() {
    // Calculate per-line feedback gain from desired RT60 (decay time)
    // g = 10^(-3 * delayTime / RT60) — derived from -60dB target
    for (int i = 0; i < kNumLines; ++i) {
        const float delaySeconds =
            static_cast<float>(m_baseDelays[static_cast<size_t>(i)]) /
            static_cast<float>(m_sampleRate);
        m_feedbackGains[static_cast<size_t>(i)] =
            std::pow(10.0f, -3.0f * delaySeconds / m_decay);
    }
}

void FDN::updateDampingCoeffs() {
    // One-pole lowpass coefficient from cutoff frequency
    // coeff = 1 - e^(-2*pi*fc/fs)
    const float fc = m_damping / static_cast<float>(m_sampleRate);
    const float coeff = 1.0f - std::exp(-6.283185307f * fc);
    for (auto& df : m_dampingFilters) {
        df.coeff = coeff;
    }
}

void FDN::updateModPhaseInc() {
    m_modPhaseInc = m_modRate / static_cast<float>(m_sampleRate);
}

} // namespace libdsp

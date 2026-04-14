# Hungry Ghost — Open Source VST Suite — Project Specification
**Target agent**: Claude Code  
**Stack**: C++17, JUCE 8, CMake 3.24+, CLAP + VST3 + AU + Standalone  
**License**: GPL-3.0  

---

## 1. Project Overview

Build a suite of open-source audio effect and instrument VST/AU/CLAP plugins sharing a common DSP library. All plugins are built from a single monorepo using CMake. The shared library (`libdsp`) contains all signal processing code; individual plugins are thin wrappers that expose parameters and route audio.

**Plugin targets (Phase 1):**
1. `hg-compressor` — Feed-forward/feedback stereo compressor with analog emulation modes
2. `hg-eq` — 8-band parametric EQ with linear-phase option and M/S processing
3. `hg-saturator` — Waveshaping saturation with ADAA anti-aliasing and multiple modes
4. `hg-reverb` — FDN algorithmic reverb with multiple room characters
5. `hg-chorus` — Multi-voice chorus with BBD and clean modes
6. `hg-synth` — Virtual analog synthesizer (polyBLEP oscillators, ladder filter, FM)

---

## 2. Repository Structure

```
hungry-ghost/
├── CMakeLists.txt                  # Root CMake — configures all targets
├── CMakePresets.json               # Debug/Release/CI presets
├── .github/
│   └── workflows/
│       └── build.yml               # CI: builds all formats on Mac/Win/Linux
├── modules/
│   └── JUCE/                       # JUCE as git submodule (tag: 8.0.x)
├── libdsp/
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── libdsp/
│   │       ├── dynamics/
│   │       │   ├── Compressor.h
│   │       │   ├── LevelDetector.h
│   │       │   └── GainComputer.h
│   │       ├── filters/
│   │       │   ├── SVFFilter.h
│   │       │   ├── LadderFilter.h
│   │       │   ├── BiquadCascade.h
│   │       │   └── Coefficients.h
│   │       ├── nonlinear/
│   │       │   ├── WaveShaper.h
│   │       │   ├── ADAA.h
│   │       │   └── TapeModel.h
│   │       ├── reverb/
│   │       │   ├── FDN.h
│   │       │   ├── DelayLine.h
│   │       │   └── AllpassChain.h
│   │       ├── modulation/
│   │       │   ├── Chorus.h
│   │       │   ├── LFO.h
│   │       │   └── FractionalDelay.h
│   │       ├── synthesis/
│   │       │   ├── PolyBlepOscillator.h
│   │       │   ├── WavetableOscillator.h
│   │       │   ├── FMOperator.h
│   │       │   └── KarplusStrong.h
│   │       ├── util/
│   │       │   ├── SmoothParameter.h
│   │       │   ├── MidSide.h
│   │       │   ├── Oversampler.h
│   │       │   └── LevelMeter.h
│   │       └── libdsp.h            # Umbrella header
│   └── src/
│       └── [.cpp implementations]
├── plugins/
│   ├── hg-compressor/
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── PluginProcessor.h/.cpp
│   │   │   └── PluginEditor.h/.cpp
│   │   └── resources/
│   ├── hg-eq/
│   ├── hg-saturator/
│   ├── hg-reverb/
│   ├── hg-chorus/
│   └── hg-synth/
└── tests/
    ├── CMakeLists.txt
    └── libdsp/
        ├── test_compressor.cpp
        ├── test_filters.cpp
        └── test_waveshaper.cpp
```

---

## 3. CMake Configuration

### 3.1 Root `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(HungryGhost VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE as submodule
add_subdirectory(modules/JUCE)

# Shared DSP library (no JUCE dependency)
add_subdirectory(libdsp)

# Plugins
add_subdirectory(plugins/hg-compressor)
add_subdirectory(plugins/hg-eq)
add_subdirectory(plugins/hg-saturator)
add_subdirectory(plugins/hg-reverb)
add_subdirectory(plugins/hg-chorus)
add_subdirectory(plugins/hg-synth)

# Tests
enable_testing()
add_subdirectory(tests)
```

### 3.2 Plugin `CMakeLists.txt` (template — repeat per plugin)

```cmake
juce_add_plugin(hg-compressor
    COMPANY_NAME "Hungry Ghost"
    PLUGIN_MANUFACTURER_CODE HGPl
    PLUGIN_CODE HGCp
    FORMATS VST3 AU Standalone            # Add CLAP via juce_clap_extensions
    PRODUCT_NAME "JV Compressor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(hg-compressor PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_compile_definitions(hg-compressor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0         # GPL exemption removes splash
)

target_link_libraries(hg-compressor PRIVATE
    libdsp
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
```

### 3.3 CLAP Support (add to each plugin)

Use `clap-juce-extensions` as a submodule:
```
modules/clap-juce-extensions/  (github.com/free-audio/clap-juce-extensions)
```

Add to plugin CMake:
```cmake
clap_juce_extensions_plugin(TARGET hg-compressor
    CLAP_ID "com.hungry-ghost.compressor"
    CLAP_FEATURES audio-effect dynamics compressor
)
```

### 3.4 `CMakePresets.json`

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
    },
    {
      "name": "release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
    }
  ]
}
```

---

## 4. libdsp — Shared DSP Library

**Rules:**
- Zero JUCE dependency. Pure C++17 + STL.
- All classes are header-only OR have explicit `.cpp` translations.
- All DSP classes accept `double sampleRate` in `prepare(double sampleRate, int blockSize)`.
- All parameters are smoothed via `SmoothParameter<float>` internally.
- No dynamic allocation after `prepare()` is called.

### 4.1 `SmoothParameter<T>`

```cpp
template<typename T>
class SmoothParameter {
public:
    void prepare(double sampleRate, double smoothingTimeMs = 20.0);
    void setTarget(T target);
    T next();          // call once per sample
    T getCurrentValue() const;
    bool isSmoothing() const;
private:
    T currentValue{}, targetValue{};
    T coeff{};
};
// Implementation: one-pole LP: coeff = exp(-2π * 20Hz / sampleRate)
// currentValue = coeff * currentValue + (1-coeff) * targetValue
```

### 4.2 `MidSide`

```cpp
namespace MidSide {
    inline void encode(float L, float R, float& M, float& S) {
        M = (L + R) * 0.5f;
        S = (L - R) * 0.5f;
    }
    inline void decode(float M, float S, float& L, float& R) {
        L = M + S;
        R = M - S;
    }
}
```

### 4.3 `LevelDetector`

```cpp
enum class DetectorMode { Peak, RMS, RMSLog };

class LevelDetector {
public:
    void prepare(double sampleRate);
    void setAttackMs(float ms);
    void setReleaseMs(float ms);
    void setMode(DetectorMode mode);
    float processSample(float x);   // returns envelope value
private:
    float attackCoeff{}, releaseCoeff{};
    float state{};
    DetectorMode mode{ DetectorMode::RMS };
    float rmsState{};
};
```

### 4.4 `GainComputer`

```cpp
struct CompressorParams {
    float threshold_dB = -20.f;
    float ratio        = 4.f;
    float kneeWidth_dB = 6.f;    // 0 = hard knee
    float makeupGain_dB = 0.f;
};

class GainComputer {
public:
    // Returns gain reduction in dB (always <= 0)
    float computeGainReduction(float level_dB, const CompressorParams& p) const;
};
```

### 4.5 `Compressor`

```cpp
class Compressor {
public:
    void prepare(double sampleRate);
    void setParams(const CompressorParams& p);
    void setAttackMs(float ms);
    void setReleaseMs(float ms);
    void setDetectorMode(DetectorMode mode);
    void setStereoLink(bool linked);    // linked = max(L,R) for both channels

    // Process stereo pair
    void process(float* left, float* right, int numSamples);

private:
    LevelDetector detector;
    GainComputer gainComputer;
    SmoothParameter<float> gainSmooth;
    CompressorParams params;
};
```

### 4.6 `SVFFilter` (State Variable Filter — TPT/ZDF)

```cpp
enum class SVFMode { LowPass, HighPass, BandPass, Notch, AllPass };

class SVFFilter {
public:
    void prepare(double sampleRate);
    void setCutoffHz(float hz);
    void setQ(float q);
    void setMode(SVFMode mode);
    float processSample(float x);

    // Direct coefficient access for ZDF:
    // g = tan(π * fc / fs)
    // k = 1/Q
private:
    float g{}, k{}, a1{}, a2{}, a3{};
    float ic1eq{}, ic2eq{};   // integrator states (TDF2)
    double sampleRate{44100};
    SmoothParameter<float> smoothCutoff, smoothQ;
};
```

Implementation follows Zavalishin TPT SVF (Art of VA Filter Design, p.77):
```cpp
float SVFFilter::processSample(float x) {
    float v3 = x - ic2eq;
    float v1 = a1 * ic1eq + a2 * v3;
    float v2 = ic2eq + a2 * ic1eq + a3 * v3;
    ic1eq = 2.0f * v1 - ic1eq;
    ic2eq = 2.0f * v2 - ic2eq;
    // return LP/HP/BP/notch based on mode
    // LP = v2, HP = v3, BP = v1, Notch = v3+v2, AP = v3+v2-k*v1
}
```

### 4.7 `LadderFilter` (Moog — TPT 4-pole)

```cpp
class LadderFilter {
public:
    void prepare(double sampleRate);
    void setCutoffHz(float hz);
    void setResonance(float r);    // 0..1, self-oscillates at ~0.99
    void setDrive(float drive);    // Nonlinear drive pre-filter
    float processSample(float x);

private:
    SVFFilter stages[4];           // 4 cascaded 1-pole LP
    float resonance{}, drive{};
    float state[4]{};
    SmoothParameter<float> smoothCutoff, smoothRes;
};
```

### 4.8 `BiquadCascade`

```cpp
struct BiquadCoeffs { float b0, b1, b2, a1, a2; };

class BiquadCascade {
public:
    BiquadCascade() = default;
    explicit BiquadCascade(int numStages);
    void setCoeffs(int stage, const BiquadCoeffs& c);
    float processSample(float x);
    void reset();
private:
    std::vector<BiquadCoeffs> coeffs;
    std::vector<std::array<float,2>> states;  // TDF2 state pairs
};

// Coefficient factory (RBJ Audio EQ Cookbook):
namespace EQCoeffs {
    BiquadCoeffs peakingEQ(double fc, double fs, double gainDB, double Q);
    BiquadCoeffs lowShelf(double fc, double fs, double gainDB, double S = 1.0);
    BiquadCoeffs highShelf(double fc, double fs, double gainDB, double S = 1.0);
    BiquadCoeffs highPass(double fc, double fs, double Q);
    BiquadCoeffs lowPass(double fc, double fs, double Q);
    BiquadCoeffs notch(double fc, double fs, double Q);
    BiquadCoeffs allPass(double fc, double fs, double Q);
}
```

### 4.9 `WaveShaper` + `ADAA`

```cpp
enum class ShaperMode { Tanh, SoftClip, HardClip, Asymmetric, Fold };

// Pure waveshaper functions (memoryless)
namespace Shapers {
    inline float tanh_shaper(float x, float drive) {
        return std::tanh(x * drive) / std::tanh(drive);
    }
    inline float softclip(float x) {
        x = std::clamp(x, -1.0f, 1.0f);
        return 1.5f*x - 0.5f*x*x*x;
    }
    // Antiderivatives F(x) for ADAA:
    inline float tanh_antideriv(float x) { return std::log(std::cosh(x)); }
    inline float softclip_antideriv(float x) {
        x = std::clamp(x, -1.0f, 1.0f);
        return 0.75f*x*x - 0.125f*x*x*x*x;
    }
}

// First-order ADAA wrapper
template<typename ShaperFn, typename AntiderivFn>
class ADAA {
public:
    float processSample(float x, ShaperFn f, AntiderivFn F) {
        float out;
        float diff = x - xPrev;
        if (std::abs(diff) < 1e-5f)
            out = f(0.5f*(x + xPrev));         // fallback at near-zero diff
        else
            out = (F(x) - F(xPrev)) / diff;    // antiderivative formula
        xPrev = x;
        return out;
    }
private:
    float xPrev{};
};
```

### 4.10 `Oversampler`

Thin wrapper around `juce::dsp::Oversampling<float>` (since libdsp can optionally depend on JUCE DSP module, or use a standalone polyphase FIR resampler from scratch):

```cpp
class Oversampler {
public:
    // factor: 2, 4, 8
    void prepare(int factor, int blockSize, double sampleRate);
    float* getUpsampledBuffer(const float* input, int numSamples);
    void downsample(const float* processed, float* output, int numSamples);
    int getLatencySamples() const;
private:
    int factor{};
    // Uses juce::dsp::Oversampling internally OR custom polyphase FIR
};
```

### 4.11 `DelayLine<T>`

```cpp
template<typename T>
class DelayLine {
public:
    void prepare(int maxDelaySamples);
    void reset();
    void write(T x);
    T read(int delaySamples) const;
    T readFrac(float delaySamples) const;   // Hermite 4-point interpolation

private:
    std::vector<T> buffer;
    int writePos{};
    int mask{};   // power-of-2 size for fast modulo
};
```

### 4.12 `FDN` (Feedback Delay Network — Jot 8-line)

```cpp
class FDN {
public:
    static constexpr int N = 8;

    void prepare(double sampleRate);
    void setDecayTime(float rt60_seconds);
    void setDamping(float cutoffHz);     // per-line lowpass fc
    void setModulation(float rateHz, float depthMs);
    void setPredelay(float ms);

    // Stereo: first 4 lines → L, last 4 → R
    void process(float inL, float inR, float& outL, float& outR);

private:
    static constexpr int DELAY_LENGTHS[N] = {1181, 1499, 1811, 2039,
                                              2311, 2591, 2999, 3407};
    DelayLine<float> lines[N];
    SVFFilter dampingFilters[N];    // 1-pole LP per line
    float feedbackMatrix[N][N]{};  // Hadamard (normalized)
    LFO lfos[N];
    DelayLine<float> predelay;
    SmoothParameter<float> smoothRT60, smoothDamping;

    void buildHadamardMatrix();
    float computeLoopGain(int lineIdx, float rt60) const;
};
```

### 4.13 `PolyBlepOscillator`

```cpp
enum class OscWaveform { Saw, Square, Triangle, Sine };

class PolyBlepOscillator {
public:
    void prepare(double sampleRate);
    void setFrequencyHz(float hz);
    void setPulseWidth(float pw);   // 0.0..1.0 for square
    void setWaveform(OscWaveform w);
    float processSample();

private:
    float phase{}, dt{};
    double sampleRate{};
    OscWaveform waveform{ OscWaveform::Saw };
    float pulseWidth{ 0.5f };
    float triIntegrator{};
    SmoothParameter<float> smoothFreq;

    float polyBlep(float t) const;
};
```

### 4.14 `LFO`

```cpp
enum class LFOWaveform { Sine, Triangle, Square, SampleAndHold };

class LFO {
public:
    void prepare(double sampleRate);
    void setRateHz(float hz);
    void setWaveform(LFOWaveform w);
    void setPhaseOffset(float radians);  // for multi-voice stagger
    float tick();                         // returns -1..+1

private:
    float phase{}, dt{};
    LFOWaveform waveform{ LFOWaveform::Sine };
    float shValue{};     // for S&H
};
```

---

## 5. Plugin Specifications

### 5.1 `hg-compressor`

**Parameters** (all APVTS-exposed):

| ID | Name | Range | Default | Units |
|----|------|--------|---------|-------|
| `threshold` | Threshold | -60..0 | -20 | dB |
| `ratio` | Ratio | 1..20 | 4 | :1 |
| `attack` | Attack | 0.1..200 | 10 | ms |
| `release` | Release | 10..2000 | 100 | ms |
| `knee` | Knee | 0..12 | 6 | dB |
| `makeup` | Makeup | -12..24 | 0 | dB |
| `input_gain` | Input Gain | -12..12 | 0 | dB |
| `mix` | Dry/Wet | 0..100 | 100 | % |
| `mode` | Mode | 0..2 (FF/FB/Opto) | 0 | enum |
| `detector` | Detector | 0..1 (Peak/RMS) | 1 | enum |
| `ms_mode` | M/S Mode | 0..2 (Stereo/Mid/Side) | 0 | enum |
| `stereo_link` | Stereo Link | 0..1 | 1 | bool |
| `lookahead` | Lookahead | 0..10 | 0 | ms |

**DSP chain:**
```
Input Gain → [M/S Encode] → Level Detector → Gain Computer →
Gain Smoother (attack/release) → VCA → Makeup Gain → [M/S Decode] → Dry/Wet Mix
```

**Modes:**
- `FF` (feed-forward): detector reads input
- `FB` (feedback): detector reads output
- `Opto`: feedback mode + program-dependent release (dual time-constant: 50ms fast + 400ms slow, whichever is larger drives release)

**Display:**
- GR meter (gain reduction, real-time, -30dB to 0dB range)
- Input/output level meters (peak + RMS)
- Transfer curve (static visualization of threshold/ratio/knee)

---

### 5.2 `hg-eq`

**Parameters:**

8 bands. Each band:

| ID pattern | Range | Notes |
|-----------|-------|-------|
| `band{n}_type` | LP/HP/LS/HS/Peak/Notch/AP | Band type enum |
| `band{n}_freq` | 20..20000 | Hz, log scale |
| `band{n}_gain` | -24..24 | dB (disabled for LP/HP) |
| `band{n}_q` | 0.1..18 | Q / bandwidth |
| `band{n}_enabled` | bool | bypass per band |

Global:

| ID | Range | Notes |
|----|-------|-------|
| `ms_mode` | 0..2 (Stereo/Mid/Side) | Which channel is processed |
| `linear_phase` | bool | Switch to linear-phase FIR mode (adds latency) |
| `output_gain` | -12..12 dB | Output trim |

**DSP chain:**
```
[M/S Encode] → 8 × BiquadCascade (1 biquad each) → [M/S Decode] → Output Gain
Linear-phase mode: replace biquads with FIR (windowed-sinc or park-McClellan via JUCE dsp::FilterDesign)
```

**Display:**
- Frequency response curve (sum of all bands, 200-point magnitude response computed in repaintTimer)
- Phase response overlay (toggleable)
- Per-band draggable nodes on the frequency curve
- FFT spectrum analyzer behind the EQ curve (real-time, 4096-point FFT, 30fps update)

---

### 5.3 `hg-saturator`

**Parameters:**

| ID | Name | Range | Default |
|----|------|--------|---------|
| `input_gain` | Input | -12..24 dB | 0 |
| `mode` | Mode | Soft/Hard/Tape/Fold/Asym | Soft |
| `drive` | Drive | 0..100 % | 50 |
| `bias` | Bias | -0.5..0.5 | 0 | (asymmetry)
| `output_gain` | Output | -24..12 dB | 0 |
| `mix` | Dry/Wet | 0..100 % | 100 |
| `oversample` | Oversample | 1x/2x/4x/8x | 4x |
| `dc_block` | DC Block | bool | true |
| `pre_filter_freq` | Pre LPF | 1k..20k Hz | 20k |
| `post_filter_freq` | Post LPF | 1k..20k Hz | 20k |

**Waveshaper modes:**
- `Soft`: first-order ADAA with `tanh(x * drive)` shaper
- `Hard`: first-order ADAA with hard clip + polyBLAMP correction
- `Tape`: simplified Jiles-Atherton (3-coefficient model), per-sample ODE integration
- `Fold`: wavefolding — `y = asin(sin(x * drive * π))` normalized
- `Asym`: tanh with DC bias: `y = tanh((x + bias) * drive) - tanh(bias * drive)`

**DSP chain:**
```
Input Gain → Pre-LPF → Oversample Up →
    [WaveShaper + ADAA] (processed at Nx rate) →
Oversample Down → Post-LPF → DC Block → Output Gain → Dry/Wet
```

**Display:**
- Oscilloscope showing input and output waveform (real-time, triggered)
- Static transfer curve (x=input, y=output, mode-dependent shape)
- THD meter (estimated from fundamental vs harmonics)

---

### 5.4 `hg-reverb`

**Parameters:**

| ID | Name | Range | Default |
|----|------|--------|---------|
| `predelay` | Pre-Delay | 0..100 ms | 10 |
| `size` | Room Size | 0..100 % | 50 |
| `decay` | Decay (RT60) | 0.1..30 s | 2.0 |
| `damping` | Damping | 200..10000 Hz | 4000 |
| `diffusion` | Diffusion | 0..100 % | 80 |
| `mod_rate` | Mod Rate | 0..5 Hz | 0.5 |
| `mod_depth` | Mod Depth | 0..5 ms | 1.0 |
| `early_level` | Early Level | -inf..0 dB | -6 |
| `late_level` | Late Level | -inf..0 dB | 0 |
| `width` | Stereo Width | 0..100 % | 100 |
| `mix` | Dry/Wet | 0..100 % | 30 |
| `freeze` | Freeze | bool | false |

**DSP chain:**
```
Input → Predelay → Early Reflections (8-tap delay network) →
         → 8-line FDN (Jot topology, Hadamard matrix) → Stereo Width → Mix
```

**FDN implementation requirements:**
- 8 delay lines with mutually-prime lengths scaled by `size` parameter
- Hadamard feedback matrix (normalized, butterfly implementation)
- Per-line 1-pole LP damping filter (cutoff = `damping` parameter)
- Per-line modulated allpass (LFO-driven, depth = `mod_depth`)
- Frequency-dependent RT60: bass slightly longer than treble (fixed 1.3x bass multiplier)
- `freeze` mode: set all loop gains to 1.0 (lossless → infinite sustain)
- DC blocking HP per line at 5 Hz

**Display:**
- Impulse response visualization (triggered mono click, 4s window)
- RT60 readout
- Wet level meter

---

### 5.5 `hg-chorus`

**Parameters:**

| ID | Name | Range | Default |
|----|------|--------|---------|
| `voices` | Voices | 2..6 | 4 |
| `rate` | Rate | 0.05..8 Hz | 0.8 |
| `depth` | Depth | 0.1..15 ms | 4.0 |
| `delay` | Center Delay | 5..30 ms | 15 |
| `feedback` | Feedback | -90..90 % | 0 |
| `mode` | Mode | Clean/BBD | Clean |
| `spread` | Stereo Spread | 0..100 % | 100 |
| `mix` | Dry/Wet | 0..100 % | 50 |
| `bbd_stages` | BBD Stages | 512/1024/2048/4096 | 1024 |
| `bbd_noise` | BBD Noise | 0..100 % | 30 |

**DSP chain:**
```
Input → [Voices: N × (DelayLine + LFO modulation)] → sum + pan L/R → Feedback → Mix
BBD mode: add per-tap: input LPF(clock/2) → delay → output LPF(clock/2) + clock noise
```

**Voice architecture:**
- Voices have LFO phases evenly spread: `phaseOffset[i] = i * (2π / numVoices)`
- Even voices pan left, odd voices pan right, weight = `spread` parameter
- All voices share same rate LFO; phases are fixed offsets
- Hermite 4-point interpolation for fractional delay

**BBD mode extras:**
- Clock frequency: `f_clock = numStages / (2 * delayTime)`
- Anti-aliasing LPF: 2-pole Butterworth at `f_clock / 2` on input
- Reconstruction LPF: same on output
- Clock noise: band-pass filtered white noise at `f_clock * 0.5`, level = `bbd_noise`

**Display:**
- LFO phase wheel visualization (per voice position on circle)
- Stereo correlation meter

---

### 5.6 `hg-synth`

**Architecture:** Polyphonic virtual analog synthesizer, up to 16 voices.

**Voice structure:**
```
MIDI → VoiceManager → (per voice):
    [OSC1: PolyBlepOscillator] ──┐
    [OSC2: PolyBlepOscillator] ──┤── [Mix] → [Filter: LadderFilter] → [VCA] → [Output]
    [Noise: WhiteNoise]        ──┘       ↑               ↑              ↑
                                       [FENV]          [ENV2]         [ENV1 (AMP)]
                                         ↑               ↑
                                       [LFO1]          [LFO2]
```

**Parameters:**

*Oscillator 1 & 2:*

| ID | Range | Notes |
|----|-------|-------|
| `osc{n}_wave` | Saw/Square/Tri/Sine | Waveform |
| `osc{n}_tune_semi` | -24..24 | Semitones |
| `osc{n}_tune_cents` | -100..100 | Fine tune |
| `osc{n}_pw` | 0.0..1.0 | Pulse width (square only) |
| `osc{n}_level` | 0..100 % | Level in mix |
| `osc_sync` | bool | OSC2 hard-synced to OSC1 |
| `osc_detune` | 0..100 cents | Unison detune |

*Filter:*

| ID | Range | Notes |
|----|-------|-------|
| `filt_cutoff` | 20..20000 Hz | |
| `filt_res` | 0..0.98 | Resonance (self-oscillates) |
| `filt_env_amount` | -100..100 % | Filter envelope depth |
| `filt_lfo_amount` | -100..100 % | LFO1 → filter |
| `filt_kb_track` | 0..100 % | Key tracking |
| `filt_drive` | 0..100 % | Pre-filter saturation |
| `filt_type` | LP12/LP24/HP12/BP12 | Filter mode |

*Envelopes (2 × ADSR):*

| ID | Range |
|----|-------|
| `env{n}_attack` | 0.5..5000 ms |
| `env{n}_decay` | 1..5000 ms |
| `env{n}_sustain` | 0..100 % |
| `env{n}_release` | 1..8000 ms |
| `env{n}_vel_sens` | 0..100 % |

*LFOs (2 × LFO):*

| ID | Range |
|----|-------|
| `lfo{n}_rate` | 0.05..30 Hz |
| `lfo{n}_wave` | Sine/Tri/Square/S&H |
| `lfo{n}_depth` | 0..100 % |
| `lfo{n}_dest` | Pitch/PW/Cutoff/Amp |
| `lfo{n}_sync` | bool (tempo sync) |

*Master:*

| ID | Range |
|----|-------|
| `voices` | 1..16 |
| `glide_time` | 0..2000 ms |
| `glide_mode` | Off/Always/Legato |
| `pitch_bend_range` | 1..12 semitones |
| `master_tune` | -100..100 cents |
| `unison_voices` | 1..8 |
| `unison_detune` | 0..100 cents |

**Voice management (VoiceManager):**
- Tracks active voices, note-on/off, MIDI channel
- Steal oldest voice in RELEASE when all voices busy
- For legato glide: keep voice alive, retrigger envelope in legato mode
- Unison: stack N voices per note with detuning

**MIDI handling (in PluginProcessor):**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    for (auto msg : midi) {
        if (msg.getMessage().isNoteOn())  voiceManager.noteOn(note, velocity);
        if (msg.getMessage().isNoteOff()) voiceManager.noteOff(note);
        if (msg.getMessage().isPitchWheel()) ...
        if (msg.getMessage().isController()) ...
    }
    for (auto& voice : voiceManager.activeVoices())
        voice.process(buffer);
}
```

**Display:**
- 2-oscillator block diagram with live parameter display
- Filter cutoff + resonance XY pad (drag to set both)
- ADSR envelope shape visualization (per envelope)
- Keyboard with active note highlighting

---

## 6. Plugin Processor Template

All plugins follow this `PluginProcessor` pattern:

```cpp
class HungryGhostCompressorProcessor : public juce::AudioProcessor {
public:
    HungryGhostCompressorProcessor();

    //=== APVTS parameter layout ===
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //=== Core overrides ===
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //=== State save/restore ===
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //=== Editor ===
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //=== APVTS (all parameters live here) ===
    juce::AudioProcessorValueTreeState apvts;

private:
    // DSP objects from libdsp
    libdsp::Compressor compressor;

    // Cached parameter pointers (avoid map lookup in processBlock)
    std::atomic<float>* thresholdParam{};
    std::atomic<float>* ratioParam{};
    // ... all params cached at prepareToPlay

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HungryGhostCompressorProcessor)
};
```

**Parameter attachment in Editor:**
```cpp
// In PluginEditor constructor:
thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    processor.apvts, "threshold", thresholdSlider);
```

---

## 7. UI Guidelines

**Framework**: JUCE's native `juce::Component` hierarchy. No external UI libraries.

**Design principles:**
- Dark theme, matte charcoal background (`#1E1E1E`)
- Accent color per plugin family (compressor = amber, EQ = blue, saturation = orange, reverb = teal)
- All knobs: `juce::Slider` with `RotaryVerticalDrag`, custom `LookAndFeel`
- Meters: custom `juce::Component` subclass, updates via `juce::Timer` at 30fps
- Frequency display: `juce::Path` drawn in `paint()`, recomputed when parameters change

**Per-plugin editor size:**
- Compressor: 600×400
- EQ: 900×500 (spectrum analyzer needs width)
- Saturator: 650×380
- Reverb: 700×420
- Chorus: 580×360
- Synth: 900×600

**Custom LookAndFeel** (`HGLookAndFeel`) subclasses `juce::LookAndFeel_V4`:
- Override `drawRotarySlider()` — flat knob with notch indicator
- Override `drawLinearSlider()` — minimal flat fader
- Override `drawButtonBackground()` — flat toggle buttons

---

## 8. CI / GitHub Actions

`.github/workflows/build.yml`:

```yaml
name: Build All Plugins

on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install Linux deps
        if: runner.os == 'Linux'
        run: sudo apt-get install -y libasound2-dev libx11-dev libxcomposite-dev
             libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev

      - name: Configure
        run: cmake --preset release

      - name: Build
        run: cmake --build build/release --parallel

      - name: Test
        run: ctest --test-dir build/release --output-on-failure

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: plugins-${{ matrix.os }}
          path: build/release/**/*.vst3
```

---

## 9. Testing Strategy

All libdsp classes have unit tests using **Catch2** (fetched via CMake FetchContent).

**Test structure:**

```cpp
// tests/libdsp/test_compressor.cpp
TEST_CASE("Compressor gain reduction", "[dynamics]") {
    libdsp::Compressor comp;
    comp.prepare(44100.0);
    comp.setParams({.threshold_dB = -20, .ratio = 4, .kneeWidth_dB = 0});
    comp.setAttackMs(1.0f);
    comp.setReleaseMs(100.0f);

    // Feed a tone at -10dBFS (10dB above threshold)
    // Expected GR: (10 - 10/4) = 7.5 dB
    // Run until settled (200ms of signal)
    float signal[8820] = {}; // 200ms
    for (auto& s : signal) s = 0.316f; // -10dBFS = 0.316 linear
    comp.process(signal, signal, 8820);
    // GR should be ~7.5 dB → output ≈ 0.316 * 10^(-7.5/20) ≈ 0.0999
    REQUIRE(std::abs(signal[8819]) == Approx(0.0999f).margin(0.01f));
}

TEST_CASE("SVF filter frequency response", "[filters]") {
    libdsp::SVFFilter svf;
    svf.prepare(44100.0);
    svf.setCutoffHz(1000.0f);
    svf.setQ(0.707f);
    svf.setMode(libdsp::SVFMode::LowPass);
    // At fc: -3dB. At 2*fc: -12dB (2-pole)
    // Measure via impulse response DFT
}
```

**Tests to implement:**
- `test_compressor.cpp`: gain reduction accuracy, attack/release timing, stereo linking, makeup gain
- `test_filters.cpp`: SVF LP/HP/BP frequency response, ladder self-oscillation threshold, biquad RBJ formula accuracy
- `test_waveshaper.cpp`: ADAA vs oversampled output comparison, THD measurement, DC offset post-asymmetric shaper
- `test_oscillator.cpp`: polyBLEP THD vs naive oscillator at several frequencies
- `test_fdn.cpp`: RT60 accuracy, decay linearity in dB, stereo decorrelation

---

## 10. Git Submodules

```bash
git submodule add https://github.com/juce-framework/JUCE.git modules/JUCE
git submodule add https://github.com/free-audio/clap-juce-extensions.git modules/clap-juce-extensions
# Pin JUCE to a specific tag:
cd modules/JUCE && git checkout 8.0.0
```

`.gitmodules`:
```
[submodule "modules/JUCE"]
    path = modules/JUCE
    url = https://github.com/juce-framework/JUCE.git
    branch = main
[submodule "modules/clap-juce-extensions"]
    path = modules/clap-juce-extensions
    url = https://github.com/free-audio/clap-juce-extensions.git
```

---

## 11. Build & Run Instructions (for README)

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/your-org/hungry-ghost.git
cd hungry-ghost

# Configure + build (release)
cmake --preset release
cmake --build build/release --parallel

# Run tests
ctest --test-dir build/release

# Built plugins land in build/release/plugins/jv-*/jv-*.vst3 (and .component on Mac)
```

---

## 12. Phase 2 Backlog (post Phase 1)

- `hg-tape-delay` — tape delay emulation (Jiles-Atherton saturation + wow/flutter)
- `hg-phaser` — allpass-based phaser with TPT topology (Zavalishin ch.6)
- `hg-transient` — transient shaper (dual-envelope derivative)
- `hg-limiter` — true-peak lookahead limiter with ITU-R BS.1770 loudness metering
- `hg-grain` — granular pitch shifter / time stretcher (phase vocoder mode)
- `hg-physical` — physical modeling instrument (Karplus-Strong + waveguide)
- CLAP-native parameter modulation support
- Preset browser (APVTS state serialized to XML files in user app data dir)
- Dark/light theme toggle
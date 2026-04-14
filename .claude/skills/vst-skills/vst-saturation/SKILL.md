---
name: vst-saturation
description: >
  Complete reference for building saturation, distortion, overdrive, and waveshaping
  VST plugins. Use when implementing any nonlinear processor — tube amp emulation,
  tape saturation, transistor overdrive, diode clipping, bitcrushing, fuzz, guitar
  amp simulation, console saturation, or transformer modeling. Covers memoryless
  waveshaping functions, aliasing and polyBLAMP/BLEP solutions, oversampling
  strategies, circuit modeling approaches, and neural network (LSTM/GRU) audio
  modeling.
---

# VST Saturation & Distortion

## Core Concept: Memoryless Waveshaping

The simplest nonlinear model: output is a function of *instantaneous* input only.

```
y[n] = f(x[n])
```

Where `f` is a nonlinear function (hard clip, tanh, polynomial, etc.).

**Problem**: generates harmonics above Nyquist → aliasing distortion. Always treat aliasing.

---

## Common Waveshaping Functions

### Hard Clipper
```cpp
y = std::clamp(x, -threshold, threshold);
```
Generates all harmonics (odd + even for asymmetric, odd-only for symmetric). Maximum aliasing. Needs heavy oversampling (8x–32x) or polyBLAMP.

### Soft Clipper (tanh)
```cpp
y = std::tanh(x * drive) / std::tanh(drive);
// Normalized so unity gain at low amplitudes
```
Smooth transition. Near-linear at low levels, approaches hard clip at high gain. Great for tubes and console preamps.

### Cubic Soft Clip (fast polynomial)
```cpp
// Only valid for |x| ≤ 1 after clamping
x = std::clamp(x, -1.0f, 1.0f);
y = 1.5f * x - 0.5f * x * x * x;  // = 1.5x - 0.5x³
// Derivative is 0 at ±1 — smooth transition into clipping
```

### Asymmetric Soft Clip (even harmonics = warmth)
```cpp
// Tube triode-like (generates 2nd harmonic)
if (x >= 0)
    y = 1.0f - std::exp(-x);
else
    y = -1.0f + std::exp(x);
```
Or simply `y = tanh(a*x + b*x²)` for tunable asymmetry.

### Sigmoid-based (parameterized hardness)
```cpp
// Adjustable: k=1 → nearly linear, k=∞ → hard clip
float hard = k * x / (1.0f + std::abs(k * x));  // algebraic sigmoid
```

### Chebyshev Polynomial Waveshaper
Dial in specific harmonic content. `T_n(cos(θ)) = cos(nθ)` — Chebyshev polynomials map a sine wave of frequency f to a sine of frequency n*f:
```
T2(x) = 2x² - 1     → adds 2nd harmonic
T3(x) = 4x³ - 3x   → adds 3rd harmonic
```
Sum these with coefficients to target specific harmonic ratios.

---

## Aliasing Problem & Solutions

Any nonlinearity creates harmonics. If those harmonics exceed Nyquist, they fold back (alias).

**Perception**: aliasing adds inharmonic, scratchy overtones that worsen with high frequencies.

### Solution 1: Oversampling (Standard)
1. Upsample signal by factor N (4x, 8x, 16x typical) using high-quality resampler
2. Apply waveshaper at oversampled rate
3. Downsample with brick-wall anti-aliasing filter

```cpp
// In JUCE/CLAP: use dsp::Oversampling<float> module
// N = 4x for soft saturation, 8–16x for hard distortion
```

Pros: simple, universal. Cons: CPU cost, phase shift from resampling filters.

### Solution 2: Anti-Derivative Anti-Aliasing (ADAA)
Apply the *antiderivative* of the waveshaper to analytically compute the average between samples.

For a waveshaper `f(x)`, let `F(x)` = antiderivative of `f(x)`:
```
y[n] ≈ (F(x[n]) - F(x[n-1])) / (x[n] - x[n-1])
```

For hard clipper, `F(x) = x²/2` in linear region, `F(x) = x` in clipped region.

Highly efficient. Single-order ADAA eliminates aliasing almost entirely for smooth signals.
Reference: Bilbao, DAFx 2017 — "Antiderivative Antialiasing for Memoryless Nonlinearities"

### Solution 3: polyBLAMP / polyBLEP
Apply correction at discontinuity points (where the derivative jumps).
BLEP = Bandlimited Step (corrects discontinuities in signal)
BLAMP = Bandlimited Ramp (corrects discontinuities in first derivative)
polyBLAMP = polynomial approximation — 2-point corrects by ~12dB, 4-point by ~20dB.

Best for hard clipping specifically. Reference: Esqueda, Välimäki, Bilbao (EUSIPCO 2015, DAFx 2016).

---

## Tube Amp Emulation

A tube amp stage consists of:
1. **Input transformer** (mild saturation, phase rotation)
2. **Preamp gain stage** (soft saturation from tube triode curves)
3. **Tone stack** (passive RC filter — highly interactive controls)
4. **Power amp stage** (harder saturation, transformer coupling)
5. **Speaker cabinet** (convolution IR or parametric minimum-phase EQ)

### Triode Model
The Koren model for triode plate current:
```
Ip = K * max(0, Vgk + Vpk/μ)^(3/2)
```
Where `K`, `μ` are tube parameters. Full SPICE-level modeling is computationally expensive; approximate with tuned tanh curves derived from measured tube curves.

### Tone Stack Emulation
The classic Fender/Marshall/Mesa tone stack is a passive RC network:
- Not straightforward to simulate with separate biquads
- Model as an RLC ladder → derive transfer function analytically → implement as IIR sections
- Online: `Tone Stack Calculator` by Duncan Munro — computes exact frequency response

### Cabinet Simulation
- **Convolution IR**: most accurate. Capture with swept sine or MLS. CPU cost scales with IR length.
- **Parametric EQ**: approximate the cabinet EQ shape with 4–8 biquad sections. Less accurate but low CPU.
- Real cab IRs vary enormously — provide IR loading as a feature.

---

## Tape Saturation

Three components: **saturation**, **hysteresis**, and **frequency response**.

### Saturation / Hysteresis
The Jiles-Atherton model is the physics-accurate hysteresis model:
```
dM/dH = (irreversible_component + reversible_component)
```
This is an ODE — expensive to integrate in real time.
Simplified implementation: use a modified tanh with frequency-dependent behavior (high frequencies clip harder).

ChowTape (ChowDSP) is open-source and uses the Jiles-Atherton model — reference implementation:
`github.com/Chowdhury-DSP/AnalogTapeModel`

### Tape Head Response
- Azimuth error → comb filtering
- Head bump → low-frequency resonance (typically 60–120 Hz)
- High-frequency rolloff from head geometry: `H(f) = sin(πf/f_c) / (πf/f_c)`

### Bias
- Underbias: bright, distorted
- Overbias: dull, over-smooth

---

## Transformer & Console Saturation

Transformers saturate at very low harmonics — primarily 2nd harmonic (even-order distortion), gentle slope.

Model: `y = x + k * x^2` for very subtle even-order, or `y = tanh(x * (1 + eps * x^2))` for asymmetric saturation.

Console channel saturation (API, SSL, Neve): mostly occurs in transformers and op-amps at high levels. Very low THD (<1%) but adds air and glue. Implement as very mild asymmetric tanh with ~0.1–0.5% THD at 0dBu input.

---

## Neural Network Modeling (Grey-Box / Black-Box)

Modern trend: use RNNs (LSTM, GRU) trained on input/output audio pairs to model hardware.

**RTNeural** — inference library for real-time audio neural networks:
`github.com/jatinchowdhury18/RTNeural`

**Chow Centaur** (Chowdhury DSP) — LSTM model of the Klon Centaur pedal:
- Trained with guitar signal in / hardware output pairs
- Running at 44.1 kHz, real-time

**Grey-box approach**: combine structural DSP (filters, compressor shell) with learned parameters. More interpretable than black-box.

Reference: Wright et al., DAFx 2020 — "Real-Time Guitar Amplifier Emulation with Deep Learning"

---

## Bitcrushing & Sample Rate Reduction

```cpp
// Bit depth reduction (quantization distortion)
float quantize(float x, int bits) {
    float levels = std::pow(2.0f, bits - 1);
    return std::round(x * levels) / levels;
}

// Sample rate reduction (hold-mode aliasing)
float sampleRateReduce(float x, int hold) {
    static float held = 0;
    static int counter = 0;
    if (++counter >= hold) { held = x; counter = 0; }
    return held;
}
```

Add dithering before quantization to shape quantization noise spectrally.

---

## Key Papers & Resources

| Source | Topic | URL/Reference |
|--------|--------|----------------|
| ADAA Paper — Bilbao et al. DAFx 2017 | Antiderivative antialiasing | `dafx.de` search "ADAA" |
| polyBLAMP — Esqueda, Välimäki | Soft-clipper antialiasing | EUSIPCO 2015 |
| ChowTape source code | Jiles-Atherton tape model | `github.com/Chowdhury-DSP/AnalogTapeModel` |
| Pakarinen & Yeh 2009 (Computer Music Journal) | Review of vacuum tube guitar amp modeling | CMJ vol.33 no.2 |
| Zölzer DAFX Book Ch. 4 | Nonlinear processing overview | Wiley 2nd ed |
| KVR DSP Forum — aliasing threads | Practitioner knowledge | `kvraudio.com/forum` |
| RTNeural | Real-time neural audio | `github.com/jatinchowdhury18/RTNeural` |
| BYOD (ChowDSP) | Full open-source distortion VST | `github.com/Chowdhury-DSP/BYOD` |
| Geofex — RG Keen | Guitar circuit analysis | `geofex.com` |
| Valve Wizard | Tube amp design | `valvewizard.co.uk` |
| Elliott Sound Products | Circuit analysis articles | `sound-au.com` |

---

## Implementation Gotchas

1. **Oversampling latency**: adds plugin latency. Report it via `getLatencySamples()`. Use PDC (plugin delay compensation) in the DAW.
2. **ADAA numerical edge case**: when `x[n] ≈ x[n-1]`, the division-by-difference blows up. Threshold: if `|x[n] - x[n-1]| < 1e-5`, fall back to `f(0.5*(x[n]+x[n-1]))`.
3. **Gain staging**: distortion amount perceived by ear is relative to the saturation threshold. Provide input gain and output level separately.
4. **DC offset from asymmetric clippers**: always high-pass filter at 5–20 Hz after nonlinearity.
5. **Stereo**: run saturation *per channel* independently. Linked stereo can create unnatural pumping.

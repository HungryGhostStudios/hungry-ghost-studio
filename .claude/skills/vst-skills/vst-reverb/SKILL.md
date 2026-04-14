---
name: vst-reverb
description: >
  Comprehensive reference for building reverb VST plugins. Use when implementing
  any reverberation algorithm — Schroeder/Moorer comb-allpass, Feedback Delay Networks
  (FDN), convolution reverb (IR), plate reverb, spring reverb emulation, room
  simulation, diffusion networks, or modern algorithmic reverb. Covers Jot FDN,
  Hadamard/Householder matrices, modal reverb, velvet noise, Dattorro plate,
  early reflections design, and decay time control.
---

# VST Reverb

## Physical Model of Reverberation

Real room reverberation has three phases:
1. **Direct sound** — dry signal, arrives first
2. **Early reflections** — discrete echoes, first 50–100ms, encode room geometry
3. **Late reverberation** — exponentially decaying diffuse sound field

Key parameters: **RT60** (time for decay by 60dB), room size, diffusion, pre-delay, early/late ratio.

---

## Algorithm Families

### 1. Schroeder / Moorer (1960s–70s)

Original digital reverb structures. Still used in simple reverbs.

**Schroeder parallel comb filters + series allpasses:**
```
input → [N parallel comb filters] → summed → [M series allpass filters] → output
```

- Comb filter: `y[n] = x[n] + g * y[n - D]`
  (D = delay samples, g = feedback gain for decay)
- Allpass: `y[n] = -g*x[n] + x[n-D] + g*y[n-D]`
  (All-pass response adds diffusion without changing frequency content)

**Freeverb** (Jezar at Dreampoint) — free, well-tuned Schroeder-Moorer implementation:
8 parallel comb + 4 series allpass per channel.
Code: widely available via web search "freeverb source code".

**Drawbacks**: metallic, resonant coloration at long decay times. The "modes" (eigenfrequencies) of the comb filter bank ring audibly.

**Fix**: use mutually prime delay lengths, add per-comb lowpass filters for frequency-dependent decay.

---

### 2. Feedback Delay Network (FDN) — Jot 1992

The industry standard for quality algorithmic reverb.

**Structure:**
```
x → [input gains b] → [delay lines D₁...Dₙ] → [N×N feedback matrix A] → [loop back] → [output gains c] → y
```

**Key equation:** output of delay line i feeds back as: `s_i[n+D_i] = Σⱼ A_ij * s_j[n] + b_i * x[n]`

**For lossless (infinite decay):** A must be **unitary** (orthogonal for real-valued).

**Common feedback matrices:**
- **Hadamard matrix**: efficient butterfly implementation. 8×8 = 3 butterfly stages.
  ```
  H₂ = 1/√2 * [1  1; 1 -1]
  H₂ₙ = H₂ ⊗ Hₙ  (Kronecker product)
  ```
- **Householder reflection**: `A = I - 2vvᵀ` for unit vector v. Single computation.
- **Random orthogonal**: Gram-Schmidt orthogonalize a random matrix.

**For finite decay:** multiply delay-line outputs by attenuation filters:
```
g_i(z) = 10^(-3*D_i / (RT60 * fs)) at DC
```
Use per-frequency RT60 with first-order IIR filters in the feedback loop to get frequency-dependent decay.

**Delay line lengths**: choose mutually prime values. Mean free path ≈ 4V/S (Sabine formula, V=volume, S=surface area). Typical: 149, 283, 401, 521, 641, 769, 919, 1087 samples @44.1kHz.

**Jot's tonal correction filter**: add a global correction filter in series to compensate for coloration introduced by absorptive filters.

---

### 3. Dattorro Plate Reverb (JAES 1997)

Jon Dattorro's "Effect Design Part 1: Reverberator and Other Filters" — JAES January 1997.
A specific network using two "tank loops" (figure-8 topology with modulated allpasses).

Structure:
```
input diffusion (4 allpasses) → 2 parallel reverb tanks (figure-8 loop)
```

Each tank contains:
- Allpass (modulated delay → chorus-like diffusion)
- Long delay line
- Lowpass filter (air absorption)
- Allpass
- Another long delay

Multiple taps from the tanks provide the output. The modulation prevents metallic modes.

This is the basis for many classic hardware reverbs and several popular plugins (Valhalla Room is inspired by this topology).

---

### 4. Convolution (IR) Reverb

Captures the actual impulse response of a physical space and convolves with the audio.

**Perfect realism**. Disadvantages: static (no real-time parameter change), CPU scales with IR length.

**Efficient convolution**: use **Overlap-Add** or **Overlap-Save** with FFT:
```
Split IR into uniform partitions of length B:
H = [H₀, H₁, H₂, ... H_N]
For each block of input, convolve with current partition, accumulate
```

**Non-uniform partitioning**: first partition small (low latency), later partitions large (efficient). Used in commercial convolution reverbs to balance latency and CPU.

**IR capture**: sweep sine method (`y(t) = sin(K*(e^(t/T) - 1))`). Divide the captured response by the swept sine's spectrum to get the impulse response.

---

### 5. Velvet Noise Reverb

Modern approach: approximate diffuse reverberation with sparse random sequences (velvet noise).
Efficient, good sound, adjustable density.

Reference: Välimäki et al., IEEE Signal Processing Letters 2009.

---

## Early Reflections Design

Early reflections are critical for perceived room size and sense of space.

1. **Image source method**: for a rectangular room, place virtual mirror-image sources at reflections. Compute delay and gain for each path.
2. **IIR early reflection network**: ladder delay network with geometrically spaced taps.
3. **FIR early reflection filter**: convolve input with a sparse FIR encoding the first N reflections.

Typical early reflection density: 10–30 discrete echoes in the first 80ms.

---

## Spring Reverb Emulation

- Physical model: spring is a dispersive waveguide
- Dispersion: `φ(ω) = c₀ + c₁*ω^0.5` — low frequencies travel slower
- Model as a digital waveguide with allpass chain providing group delay dispersion
- Chirp artifact (boing): excited by large input transients; add saturation + instability feedback
- Reference: DAFx proceedings — search "spring reverb model"

---

## Parameter Control

| Parameter | Implementation |
|-----------|---------------|
| Pre-delay | Simple delay line before algorithm input |
| RT60 / Decay | Scale loop gain filters `g ∝ 10^(-3/RT60*τ)` |
| Diffusion | Depth of modulation in allpass sections |
| Room size | Scale all delay line lengths by the same factor |
| Damping | Cutoff frequency of in-loop lowpass filters |
| Early/late balance | Mix of early reflection network and late FDN |
| Modulation rate/depth | LFO controlling allpass delay modulation depth |

---

## Modulation (Preventing Metallic Modes)

Modulate allpass delay lengths with low-frequency LFOs (<5 Hz) to break up resonant modes.
- Amount: 0.1–2ms modulation depth
- Different LFO phases for each delay line
- Keep modulation subtle — too much → pitch wobble (chorus effect)

---

## Key Papers & Resources

| Source | Topic | URL/Reference |
|--------|--------|----------------|
| Schroeder 1962 | Original comb/allpass reverb | "Natural-Sounding Artificial Reverberation", JAES |
| Moorer 1979 | "On the Reverberation Algorithm" | JAES |
| Jot & Chaigne 1991 | FDN reverb | AES Convention Paper |
| Dattorro 1997 | Plate algorithm (must-read) | JAES Jan/Apr 1997 — "Effect Design Part 1" |
| Gardner 1992 | MIT thesis — room reverb | Media Lab thesis |
| JOS CCRMA — Artificial Reverberation | Full online textbook chapter | `ccrma.stanford.edu/~jos/pasp/Artificial_Reverberation.html` |
| Välimäki et al. 2012 | "Fifty Years of Artificial Reverberation" | IEEE Trans. Audio, Speech, Lang. |
| FDNTB Toolbox (MATLAB) | FDN toolbox — research tool | DAFx 2020 paper |
| Freeverb source | Schroeder-Moorer reference | Search "Freeverb source code" |
| Valhalla DSP blog | Sean Costello's algorithm discussions | `valhalladsp.com/blog` |
| Zita-rev1 (Fons Adriaensen) | High-quality open-source FDN C++ | `kokkinizita.linuxaudio.org` |

---

## Implementation Gotchas

1. **Delay line buffer management**: use circular (ring) buffers with power-of-2 sizes for efficiency. Wrap with bitmasking: `idx & (size-1)`.
2. **Denormals in feedback loops**: add a tiny dither signal (1e-30) to prevent CPU slowdown from subnormal numbers in the long decay tail.
3. **Sample rate independence**: all delay line lengths are in samples. Scale by `fs / 44100.0` for SR-independent reverb time.
4. **Stereo**: typically run an 8-delay FDN with the first 4 taps → left and second 4 → right. Don't just duplicate mono with different delays (too correlated).
5. **DC blocking**: add DC-blocking high-pass (very low fc, ~5 Hz) in the feedback loop of each delay line to prevent DC build-up.
6. **Tail truncation**: FIR convolution reverbs have a sharp cut at the end of the IR. Crossfade to silence over the last 20–50ms.

---
name: vst-eq-filters
description: >
  Complete reference for building EQ and filter VST plugins. Use when implementing
  any EQ type — shelving, bell/peak, high/low pass, notch, allpass — or any filter
  design: biquad, state-variable, ladder (Moog), Sallen-Key, graphic EQ, linear
  phase FIR EQ, minimum-phase IIR, dynamic EQ, or vintage console EQ emulation
  (Neve, SSL, API, Pultec). Covers bilinear transform, topology-preserving
  transforms (TPT/ZDF), matched-Z, and filter cookbook coefficients.
---

# VST EQ & Filters

## The Biquad — Universal EQ Building Block

All parametric EQ bands are built from the **Direct Form II transposed biquad**:

```
H(z) = (b0 + b1*z⁻¹ + b2*z⁻²) / (1 + a1*z⁻¹ + a2*z⁻²)

// TDF2 implementation (numerically preferred):
y[n] = b0*x[n] + s1[n-1]
s1[n] = b1*x[n] - a1*y[n] + s2[n-1]
s2[n] = b2*x[n] - a2*y[n]
```

Use **TDF2 over DF1/DF2** — better numerical behavior and the natural storage for VA/ZDF filter states.

---

## Analog Prototype → Digital Mapping Methods

### Bilinear Transform (BLT)
Maps `s = (2/T) * (z-1)/(z+1)`.
- Pros: no aliasing, stable
- Cons: frequency warping — pre-warp cutoff: `ωc_analog = (2/T) * tan(ωc_digital * T/2)`

### Matched-Z Transform
Maps poles/zeros directly: `z = e^(s*T)`.
- Better high-frequency response match
- May be unstable for some prototypes

### Topology-Preserving Transform (TPT / ZDF — Zavalishin)
Models the *circuit topology* itself in discrete time using trapezoidal integration.
- Zero delay feedback resolved analytically
- Correct time-varying behavior (smooth modulation of cutoff)
- **Mandatory for any modulated filter** (synth filters, wah, envelope filters)
- Reference: *The Art of VA Filter Design* — `native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf`

---

## Cookbook Filter Coefficients (Audio EQ Cookbook — RBJ)

Robert Bristow-Johnson's coefficients — the standard reference for digital EQ.
URL: `https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html`

### Peak/Bell Filter
```
A = sqrt(10^(dBgain/20))   // or: A = 10^(dBgain/40)
w0 = 2*pi*f0/Fs
alpha = sin(w0)/(2*Q)

b0 =   1 + alpha*A
b1 =  -2*cos(w0)
b2 =   1 - alpha*A
a0 =   1 + alpha/A
a1 =  -2*cos(w0)
a2 =   1 - alpha/A
```

### Low Shelf
```
b0 =    A*[ (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha ]
b1 =  2*A*[ (A-1) - (A+1)*cos(w0)                   ]
b2 =    A*[ (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha ]
a0 =        (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha
a1 =   -2*[ (A-1) + (A+1)*cos(w0)                   ]
a2 =        (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha
```

### High-Pass 2nd Order
```
b0 =  (1 + cos(w0))/2
b1 = -(1 + cos(w0))
b2 =  (1 + cos(w0))/2
a0 =   1 + alpha
a1 =  -2*cos(w0)
a2 =   1 - alpha
```

*Always normalize: divide all b/a coefficients by a0.*

---

## State Variable Filter (SVF)

The SVF is a 2-pole filter with simultaneous LP, BP, HP, notch, allpass outputs.
Ideal for synths and dynamic EQ (smooth cutoff modulation).

**TPT/ZDF SVF (Zavalishin p.77):**
```
// Per-sample:
v1 = (x - g*s2 - s1) / (1 + g*(g + k))  // k = 1/Q
v2 = g*v1

HP = v1
BP = s1 + g*v1  (update: s1 += 2*v2)
LP = s2 + g*v2  (update: s2 += 2*v2)
Notch = x - k*BP
AP    = x - 2*k*BP
```
Where `g = tan(π*fc/fs)` and `k = 1/Q`.

---

## The Moog Ladder Filter

4-pole (24dB/oct) LPF with resonance feedback — the foundation of subtractive synthesis.

**Key characteristics:**
- Self-oscillates at resonance = 1.0 (Q → ∞)
- Resonance reduces passband level; compensate with `sqrt(resonance + 1)` makeup
- Nonlinear version: tanh() saturation in each stage loop and/or feedback path

**TPT Ladder (Zavalishin ch.4):**
```
u = tanh((input - 4*resonance*output) / saturation_level)
for each of 4 cascaded TPT 1-pole LPFs: run u through each stage
output = last stage output
```
Zavalishin covers both linear and nonlinear models extensively.

**Stilson & Smith 1996 paper** on the Moog VCF — free at CCRMA.

---

## Classic Console EQ Emulation

### Pultec EQP-1A
- Passive LC-based shelving EQ with unique boost+cut interaction
- When you boost AND cut the same frequency, you get a "tilt" around the corner with a resonant bump
- Model with passive RLC ladder network analysis, then implement as parallel biquad sections
- White papers: search "Pultec passive EQ circuit analysis"

### Neve 1073
- Transformer-coupled, op-amp-based EQ
- Distinct sound from transformer saturation and limited high-frequency headroom
- 3 bands: HF shelf, mid bell, HPF
- Input/output transformer models: add mild saturation + phase rotation (allpass shelf) to signal path

### SSL 4000 E/G Channel
- HF/LF shelves switchable between shelf and bell
- Dynamic EQ can be implemented as two compressors per band
- "Black knob" vs "Brown knob" — different Q behaviors in each era

### API 550A/550B
- Proportional-Q design: Q increases with boost/cut amount
- 5-band, stepped controls (±12dB in 2dB steps)
- `Q_effective = Q_base / (1 + boost_dB / some_normalization)`

---

## Filter Types Summary

| Type | Order | Roll-off | Use case |
|------|-------|----------|----------|
| Butterworth | N | -20N dB/oct | Maximally flat, smooth rolloff |
| Chebyshev I | N | -20N dB/oct | Equiripple passband, steeper rolloff |
| Chebyshev II | N | -20N dB/oct | Equiripple stopband |
| Elliptic (Cauer) | N | Steepest | Equiripple both bands, phase nonlinearity |
| Bessel | N | -20N dB/oct | Maximally flat group delay, gentle rolloff |
| Linkwitz-Riley | 2N | -40N dB/oct (LR4) | Crossovers — flat summed response |

---

## Linear Phase EQ

For mastering EQ or transparent processing:
- FIR filter with symmetric coefficients → linear phase → no phase distortion
- Cost: **latency** = N/2 samples (N = filter length)
- Design: park-McClellan (Remez) algorithm or windowed sinc
- Drawback vs IIR: much higher CPU for equivalent frequency resolution
- Mixed-phase option: linear-phase for subtle cuts, minimum-phase for boosting (less pre-ringing)

---

## Dynamic EQ

Parametric EQ band where gain is controlled by a sidechain detector (like a compressor per band).
- Linear-phase FIR dynamic EQ: computationally expensive but transparent
- IIR dynamic EQ: low latency, more phase artifacts
- Per-band detector: RMS or peak with attack/release
- Sidechain can be internal (same signal) or external

---

## Key Resources

| Source | URL / Reference |
|--------|----------------|
| Audio EQ Cookbook (RBJ) | `webaudio.github.io/Audio-EQ-Cookbook/` |
| Zavalishin VA Filter Design (free PDF) | `native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf` |
| JOS — Intro to Digital Filters | `ccrma.stanford.edu/~jos/filters/` |
| JOS — Spectral Audio Signal Processing | `ccrma.stanford.edu/~jos/sasp/` |
| Zölzer DAFX Book Ch. 2-3 | Filter design chapters, Wiley |
| Will Pirkle — Designing Audio Effect Plugins in C++ | Chapter on EQ/filters |
| Signalsmith Blog — EQ interpolation | `signalsmith-audio.co.uk/writing` |
| KVR DSP Forum | `kvraudio.com/forum/viewforum.php?f=33` |
| Stilson & Smith 1996 — Moog VCF | `ccrma.stanford.edu/~stilti/papers/moogvcf.pdf` |

---

## Implementation Gotchas

1. **Coefficient smoothing**: never jump filter coefficients — interpolate smoothly per block or per sample to avoid clicks. Use parameter smoothing (1-pole lowpass on each coefficient or cutoff/Q).
2. **Denormals**: add a tiny DC offset (`1e-25`) to state variables or use `FTZ` CPU flags to prevent subnormal slowdowns.
3. **High resonance instability**: at very high Q, TDF2 biquads can go unstable. Use SVF topology instead for high-Q synth filters.
4. **Oversampling**: not usually needed for linear EQ; needed for nonlinear elements in the signal path.
5. **Graphic EQ**: constant-Q vs proportional-Q graphic EQ; use Regalia-Mitra or Orfanidis equalization for minimum-overlap graphic EQ.

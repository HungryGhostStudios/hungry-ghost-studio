---
name: vst-synthesis
description: >
  Complete reference for building synthesizer VST plugins. Use when implementing
  any synthesis engine: virtual analog (subtractive), FM/PM synthesis, wavetable,
  additive, physical modeling (Karplus-Strong, digital waveguides, modal), granular,
  AM/RM, or hybrid engines. Covers bandlimited oscillator design (BLIT, polyBLEP,
  minBLEP), analog oscillator emulation (Moog, Roland, ARP), envelope generators,
  voice allocation/polyphony, and MIDI handling.
---

# VST Synthesis Engines

## Oscillator Fundamentals

### The Aliasing Problem
A naive sawtooth generates harmonics at all multiples of the fundamental. Above Nyquist, these fold back as aliasing distortion. A 440 Hz saw at 44.1 kHz has 50 harmonics before Nyquist — but any higher frequencies (from waveshape non-idealities) alias horribly.

---

## Bandlimited Oscillator Methods

### polyBLEP (Production Standard)
Polynomial Bandlimited Step — correct the discontinuity points of classic waveforms with a small correction function.

```cpp
// polyBLEP correction for a rising discontinuity at fractional position t (0..1)
float polyBlep(float t, float dt) {
    if (t < dt) {        // just after the discontinuity
        t /= dt;
        return t+t - t*t - 1.0f;
    } else if (t > 1.0f - dt) {  // just before
        t = (t - 1.0f) / dt;
        return t*t + t+t + 1.0f;
    }
    return 0.0f;
}

// Sawtooth oscillator with polyBLEP:
float phase = 0;  // 0..1
float dt = freq / sampleRate;
float saw = 2.0f * phase - 1.0f;  // naive saw
saw -= polyBlep(phase, dt);         // correction at wrap-around
phase += dt;
if (phase >= 1.0f) phase -= 1.0f;
```

**polyBLEP quality**: excellent for frequencies < ~8kHz. At very high frequencies, small residual aliasing remains — use 2x oversampling for critical applications.

### minBLEP (High Quality, More CPU)
Pre-computed minimum-phase step correction. True alias-free for all audio frequencies.
Reference: Müller, Pirkle — "Bandwidth Limited Impulse Train" papers.

### BLIT (Bandlimited Impulse Train)
Synthesize the derivative of the waveform (a train of impulses), then integrate.
`saw(t) = integrate(BLIT(t))`.
Good quality, some complexity in implementation.
Reference: Stilson & Smith 1996 — CCRMA technical report.

---

## Classic Waveform Generation (polyBLEP)

### Pulse / Square Wave
```cpp
float pulse = (phase < pulseWidth) ? 1.0f : -1.0f;
// Apply polyBLEP at both discontinuities (rising at 0, falling at pulseWidth)
pulse += polyBlep(phase, dt);
pulse -= polyBlep(fmod(phase + 1.0f - pulseWidth, 1.0f), dt);
```

### Triangle Wave
Integrate the square wave:
```cpp
float tri = dt * square + (1.0f - dt) * tri_prev;  // leaky integrator
```

Or derive analytically from the square with a proper integration constant.

### Hard Sync
When oscillator 2 resets oscillator 1 (regardless of oscillator 1's phase):
```cpp
if (osc2_reset_this_sample) {
    // Apply polyBLEP correction at the forced discontinuity
    osc1_phase = 0;
}
```
Hard sync creates rich, formant-like spectra — classic Roland Juno / SH-101 sound.

---

## FM / PM Synthesis

### Basic FM (Chowning 1973)
```
output = A * sin(ωc * t + I * sin(ωm * t))
```
- `ωc` = carrier frequency
- `ωm` = modulator frequency
- `I` = modulation index (depth)

Spectrum: sidebands at `fc ± n*fm` for all integers n, with amplitudes given by Bessel functions Jn(I).

**Harmonic spectra**: when `fc/fm = integer ratio`, all sidebands are harmonically related.
**Metallic/inharmonic spectra**: when `fc/fm` is irrational or complex ratios.

**In practice (DX7-style)**: use *phase modulation* (PM) instead — more stable for musical use:
```
output = sin(ωc * t + PM_from_modulator)
```
PM and FM are mathematically equivalent for sinusoidal modulators but PM scales more intuitively.

### DX7 Algorithm System
The DX7 has 6 operators arranged in 32 fixed algorithms. Carriers are audible; modulators shape the timbre.

**Operator**: oscillator + envelope + level. Can be in carrier or modulator role depending on position in algorithm.

**Feedback**: one operator can modulate itself (adds noise/buzzy character).

**C-ratio / C:M ratio**: the most musically useful FM textures come from specific ratios:
- 1:1 → adds odd + even harmonics
- 1:2 → doubles the fundamental, adds harmonics
- 1:7 → harsh metallic character
- N:1.414 (irrational) → bell-like inharmonic partials

Reference: Chowning 1973 CMSJ — "The Synthesis of Complex Audio Spectra by Means of Frequency Modulation"

---

## Wavetable Synthesis

Store one cycle of a waveform in a table; read at different rates for different pitches.

**Anti-aliasing**: different wavetables for different pitch ranges (each contains only harmonics below Nyquist for that octave band):
```
Table 0: all harmonics  (used for octave 0-1)
Table 1: harmonics 1..N/2 (octave 1-2)
Table k: harmonics 1..N/2^k (octave k to k+1)
```

**Mipmap-style**: choose the appropriate table based on playback speed:
```cpp
int tableIndex = (int)log2(playbackSpeed);
float* table = tables[clamp(tableIndex, 0, numTables-1)];
```

**Interpolation between tables**: crossfade based on fractional position between table bands to avoid spectral discontinuities.

**Wavetable reading** (fractional index):
```cpp
float read(float* table, int size, float pos) {
    int i0 = (int)pos % size;
    int i1 = (i0 + 1) % size;
    float frac = pos - (int)pos;
    return table[i0] + frac * (table[i1] - table[i0]);
}
```

---

## Additive Synthesis

Sum of sinusoidal partials:
```
output(t) = Σ aₙ(t) * sin(n*ω₀*t + φₙ(t))
```

Computationally expensive for many partials but highly controllable.

**Resynthesis pipeline**: analyze audio with STFT/sinusoidal analysis → track partials → resynthesize.

**Efficient implementation**: use a single accumulator per partial (phase accumulator oscillator):
```cpp
phase[k] += 2*π * freqs[k] / fs;
output += amps[k] * sin(phase[k]);
```

For N > 100 partials, consider IFFT-based resynthesis (OLA method).

---

## Physical Modeling

### Karplus-Strong (Plucked String)
```
// Fill delay line with noise (the pluck)
// Continuously average adjacent samples + feedback
y[n] = 0.5 * (y[n-N] + y[n-N-1])  // where N = fs/pitch
```

The 0.5 averaging acts as a lowpass filter, causing higher harmonics to decay faster (like a real string).

**Extended Karplus-Strong** (dynamic stiffness, pick position, etc.):
- Pick position: comb filter on initial noise
- Stiffness: allpass filter in the loop
- Loss: adjust the loop filter pole

### Digital Waveguide (JOS — Smith 1992)
Model waves traveling in both directions along a physical medium (string, tube, etc.):
```
// Right-traveling wave: y+(t, x) propagated as delay
// Left-traveling wave:  y-(t, x) propagated as delay (reverse direction)
// Scattering junctions connect multiple waveguides
```

Used for: strings (bowed, plucked, struck), wind instruments (clarinet, flute, trumpet), percussion.

Key insight: a 1D acoustic medium with wave equation is perfectly simulated by two delay lines (one per direction) plus appropriate boundary conditions.

**JOS books**: `ccrma.stanford.edu/~jos/pasp/` — covers strings, winds, brasses, percussion in detail.

### Modal Synthesis
Decompose an instrument into its resonant modes. Each mode is a second-order resonator:
```
H_k(z) = A_k / (1 - 2*R_k*cos(ω_k)*z⁻¹ + R_k²*z⁻²)
```
Where `R_k` = decay rate for mode k, `ω_k` = modal frequency, `A_k` = modal amplitude.

Drive these modes with an impulse or excitation signal. Very efficient for percussive instruments.

---

## Envelope Generators

### ADSR
```cpp
// Simple state machine
enum State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };

float tick() {
    switch(state) {
    case ATTACK:
        level += attackIncrement;
        if (level >= 1.0f) { level = 1.0f; state = DECAY; }
        break;
    case DECAY:
        level -= decayDecrement;
        if (level <= sustainLevel) { level = sustainLevel; state = SUSTAIN; }
        break;
    case SUSTAIN: break;  // hold at sustain level
    case RELEASE:
        level -= releaseDecrement;
        if (level <= 0.0f) { level = 0.0f; state = IDLE; }
        break;
    }
    return level;
}
```

**Exponential ADSR** (more musical — matches human perception):
Use `level *= factor` instead of `level += increment`. Converges asymptotically; use a small offset to ensure it actually reaches target:
```cpp
level = target + (level - target) * decayCoeff;  // decayCoeff < 1
```

**DX7-style ADSR**: 4-rate, 4-level curve segments. Each segment has independent rate + target level. More expressive than simple ADSR.

---

## Voice Architecture & Polyphony

### Voice Stealing
When all voices are busy and a new note arrives, steal the quietest or oldest active voice:
```cpp
int findVoiceToSteal() {
    // Prefer voices in RELEASE, then DECAY
    // Prefer quietest level
    // Prefer oldest note-on time
}
```

### Unison
Multiple voices playing the same note, detuned slightly:
```
Voice 1: pitch * 2^(+spread/1200)
Voice 2: pitch (center)
Voice N: pitch * 2^(-spread/1200)
```

Sum all voices, compensate gain by `1/sqrt(N)`.

---

## Key Resources

| Source | Topic | URL/Reference |
|--------|--------|----------------|
| Chowning 1973 CMSJ | FM synthesis original paper | Stanford CS archive |
| JOS — Physical Audio Signal Processing | Digital waveguides, physical modeling | `ccrma.stanford.edu/~jos/pasp/` |
| Stilson & Smith 1996 | Bandlimited oscillators, Moog VCF | CCRMA Tech Report |
| Välimäki et al. 2010 | polyBLEP wavetable oscillators | AES paper |
| DX7 RE blog series — Ken Shirriff | DX7 chip reverse engineering | `righto.com/2021/12/yamaha-dx7...` |
| Dexed source code | Open-source DX7 emulator | `github.com/asb2m10/dexed` |
| Surge XT source code | Open-source, huge range of synthesis | `github.com/surge-synthesizer/surge` |
| ZynAddSubFX | Open-source additive/subtractive synth | `github.com/zynaddsubfx/zynaddsubfx` |
| Pirkle — Designing Software Synthesizer Plugins | Book: all synthesis types | Focal Press |
| Zölzer DAFX — Ch. 7-8 | Synthesis and virtual analog | Wiley 2nd ed |

---

## Implementation Gotchas

1. **Pitch quantization**: compute oscillator phase increment as `freq / sampleRate`, not `freq * T`. For precise pitch, derive freq from MIDI note: `f = 440 * 2^((note - 69) / 12)`.
2. **Phase accumulator overflow**: modulo 1.0f per sample. `if (phase >= 1.0f) phase -= 1.0f;` (avoid fmod — slow).
3. **Bandlimited sum in additive**: for efficiency, compute only harmonics below `fs/2`. `N_max = floor(fs / (2 * f0))`.
4. **Karplus-Strong delay line length**: `N = round(fs / f0)`. The rounding introduces tuning error at low frequencies. Use allpass interpolation to tune precisely.
5. **FM ratio feedback**: DX7 operators with self-feedback create noise at high feedback amounts. Clip and scale appropriately.

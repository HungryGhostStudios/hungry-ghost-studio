---
name: vst-pitch-time
description: >
  Complete reference for building pitch shifting, time stretching, pitch correction,
  and formant manipulation VST plugins. Use when implementing any pitch or time
  manipulation — phase vocoder, PSOLA, granular time stretch, harmonic pitch
  shifting, auto-tune-style correction, vibrato, microtuning, formant shifting,
  or robotization effects. Covers STFT overlap-add, phase propagation, transient
  handling, and elastique/zplane algorithm approaches.
---

# VST Pitch & Time Processing

## Core Problem

**Time stretching**: change duration without changing pitch.
**Pitch shifting**: change pitch without changing duration.
These are inherently coupled — naive approaches affect both simultaneously.

---

## The Phase Vocoder (STFT-Based)

The phase vocoder is the most flexible approach. It separates analysis (decompose into frequency bins) from synthesis (resynthesize at different time/pitch).

### Analysis Stage
```
1. Buffer audio frames of length N (e.g., 2048 samples)
2. Apply analysis window (Hann: w[n] = 0.5*(1-cos(2π*n/N)))
3. Compute FFT: X[k] = FFT(x[n] * w[n])
4. Extract magnitude and phase: |X[k]|, ∠X[k]
5. Advance by H_a samples (analysis hop size, e.g., 512)
```

### Phase Difference (Instantaneous Frequency)
Between frames, each bin k accumulates an expected phase advance of `2π*k*H_a/N`.
The *actual* phase advance tells us the true instantaneous frequency:

```cpp
float expected_phase_advance = 2*π * k * hopSize / fftSize;
float delta_phase = angle[k] - prev_angle[k] - expected_phase_advance;
// Wrap to [-π, π]
delta_phase -= 2*π * round(delta_phase / (2*π));
// True frequency:
float true_freq = (expected_phase_advance + delta_phase) * sampleRate / (2*π * hopSize);
```

### Time Stretching
For time stretch by factor `α` (α=2.0 → twice as long):
- Analysis hop: `H_a`
- Synthesis hop: `H_s = α * H_a`

Compute synthesis phases by accumulating scaled phase increments:
```cpp
synth_phase[k] += true_freq[k] * 2*π * H_s / sampleRate;
```

### Pitch Shifting
Pitch shift by factor `β` (β=2.0 → one octave up):
- First time-stretch by `1/β` (make it shorter)
- Then resample by `β` (restore duration)

Or in one step: `H_s = H_a * (1/β)`, then resample output by `β`.

### Synthesis Stage (Overlap-Add)
```
1. For each output frame: reconstruct X_out from synth magnitude + synth phase
2. IFFT → x_out[n]
3. Apply synthesis window
4. Overlap-add at synthesis hop positions
5. Normalize by sum of window²
```

---

## PSOLA (Pitch Synchronous Overlap-Add)

Works in the time domain. Requires pitch detection.

**TD-PSOLA (Time-Domain PSOLA):**
1. Detect pitch periods (fundamental period T₀)
2. Extract "pitch marks" (one per period)
3. Window each period with a Hann window of length 2*T₀
4. For pitch shifting by β: output the same segments but spaced T₀/β apart

**Advantages**: very natural for speech and monophonic instruments, low artifacts.
**Disadvantages**: requires clean pitch detection; struggles with polyphony, noise, transients.

**Implementation pattern:**
```
analysis pitch marks: [t₀, t₀+T₀, t₀+2T₀, ...]
synthesis pitch marks: [t₀, t₀+T₀/β, t₀+2T₀/β, ...]
OLA the analysis frames at synthesis marks
```

---

## Granular Time Stretching

Divide audio into small overlapping grains (20–100ms each):
```
grain[k] = windowed(audio[pos_k..pos_k+grain_len])
```

For time stretching: output grains at different positions than analysis.
For pitch shifting: resample each grain at rate `1/β`.

**Grain scheduling:**
- Regular: grains extracted and placed at constant intervals
- Randomized position: adds texture, hides grain artifacts
- Transient-locked: detect onsets and lock grains to transients

**Grain windows**: Hann or Tukey (tapered rectangle). Overlap must be ≥ 50% for smooth OLA.

---

## Transient Handling — The Key Quality Differentiator

Time stretching is worst on transients (attacks of drums, plucked strings).
The transient "smears" when grains overlap.

**Solutions:**
1. **Transient detection**: find onset frames (spectral flux, HFC, phase deviation)
2. **Lock mode**: on transient frames, set analysis hop = synthesis hop (no stretching) to preserve transient sharpness
3. **Transient extraction + stretch residual**: separate transient and tonal components, stretch only the tonal part, recombine

**Transient detection (spectral flux):**
```cpp
float spectralFlux = 0;
for (int k = 0; k < N/2; k++) {
    float diff = mag[k] - prev_mag[k];
    spectralFlux += std::max(0.0f, diff);  // positive-only = onset
}
if (spectralFlux > threshold) // → transient frame
```

---

## Pitch Detection

Required for PSOLA and pitch correction.

### Autocorrelation (McLeod Pitch Method / YIN)
```cpp
// YIN: compute difference function
float d[tau] = sum_{n=0}^{N-1} (x[n] - x[n+tau])^2
// Cumulative mean normalized difference (CMNDF)
float d'[tau] = d[tau] / (1/tau * sum_{j=1}^{tau} d[j])
// Find first minimum below threshold (e.g. 0.1)
```

YIN is the standard for monophonic pitch detection.
Reference: de Cheveigné & Kawahara, JASA 2002.

### HPS (Harmonic Product Spectrum)
```cpp
// Compute spectrum magnitude
// Down-sample and multiply spectra at 1x, 2x, 3x etc.
// Peak of product corresponds to fundamental
```

### PYIN (Probabilistic YIN)
Probabilistic extension of YIN. Much more robust, handles octave errors.
Free reference implementation in librosa (Python).

---

## Pitch Correction (Auto-Tune Style)

1. **Detect pitch** in each analysis frame (YIN, pYIN)
2. **Compute correction** = target pitch / detected pitch
3. **Smooth correction** over time to avoid artifacts (one-pole LP on correction amount)
4. **Apply correction**: multiply synthesis frequency or resample grain

**Retune speed**: controls how fast the pitch correction snaps. Slow = gentle, natural. Fast = robotic.

**"Hard tune" / robotic effect**: set retune speed to maximum (instantaneous correction).

---

## Formant Preservation

Pitch shifting without formant preservation causes the "chipmunk" or "giant" effect.

**Formants** = resonant peaks in the spectral envelope of a voice (or instrument) that are fixed regardless of pitch. The vowel sound of a voice depends on formant positions.

**Cepstral formant extraction:**
```
1. Compute log magnitude spectrum: log|X[k]|
2. IFFT (cepstrum)
3. Apply liftering window (retain only slow-varying quefrency terms)
4. Forward FFT to get spectral envelope
5. Subtract envelope from fine spectrum to get harmonic structure
```

Then shift the harmonics (pitch shift) but NOT the envelope (formants stay).

**Simpler approach**: all-pole model (LPC). Extract LPC coefficients representing the formant envelope, then shift the excitation signal independently.

---

## Robotization & Whisperization

**Robotization**: replace the excitation phase with zeros (set all phases to 0 before synthesis). Sounds like a monotone robot voice.

**Whisperization**: replace the magnitude spectrum with noise. Sounds like whispering.

Both are phase vocoder effects — trivial to implement once you have a working phase vocoder.

---

## Vibrato in Pitch Shifters

Apply a slow LFO (<8 Hz) to the pitch shift ratio:
```
pitch_ratio = center * 2^(lfo_depth * sin(2π * lfo_rate * t) / 1200.0f)
```
(depth in cents)

---

## Key Resources

| Source | Topic | URL/Reference |
|--------|--------|----------------|
| Dolson 1986 — Computer Music Journal | Phase vocoder tutorial | Classic CMJ paper, widely available |
| Zölzer DAFX Ch. 7 | Time-frequency effects | Wiley |
| JOS CCRMA — Spectral Audio Signal Processing | STFT, overlap-add | `ccrma.stanford.edu/~jos/sasp/` |
| YIN Algorithm — de Cheveigné & Kawahara 2002 | Pitch detection | JASA 2002 |
| Moulines & Charpentier 1990 | PSOLA (original paper) | ICASSP |
| Elastique SDK (zplane) | Commercial time-stretch SDK | `zplane.de` (for reference) |
| Rubber Band Library | Open-source time stretch/pitch shift | `github.com/breakfastquay/rubberband` |
| SoundTouch Library | Lightweight open-source | `surina.net/soundtouch/` |
| librosa (Python) | Reference implementations, pYIN | `librosa.org` |

---

## Implementation Gotchas

1. **Phase vocoder latency**: analysis hop + frame length = minimum latency. A 2048-sample FFT at 512-hop = ~46ms latency @44.1kHz. Report this.
2. **Phase continuity on pitch shift change**: when ratio changes, smoothly interpolate or you'll hear a click. Interpolate synthesis hop or true frequency across a transition window.
3. **Sibilance smearing**: the "s" and "t" sounds are hard transients. Without transient handling, they smear. Detect high-frequency transients separately.
4. **Phase vocoder phasiness**: the classic "phasiness" artifact comes from bin-phase cancellation when two signals occupy the same bin. For polyphonic time stretching, use more sophisticated methods (REPET, MDP, HPSS separation first).
5. **Stretch ratio limits**: most algorithms break down beyond 2–3x stretch. Warn users / document limits.
6. **Bin assignment for pitch shift by resample**: when shifting by resample, the output sample rate is different from input — correct by applying a rational SRC filter at the synthesis output.

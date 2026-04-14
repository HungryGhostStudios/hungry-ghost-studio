---
name: vst-imaging
description: >
  Reference for building stereo imaging, mid-side processing, spatial enhancement,
  binaural, and panning VST plugins. Use when implementing any stereo width plugin,
  M/S encoder/decoder, stereo bus tool, Haas effect widener, Blumlein difference,
  panning laws, HRTF binaural spatialization, ambisonics, mono compatibility checks,
  correlation meters, or the Dolby/spatial audio context for plugin design.
---

# VST Imaging & Stereo Processing

## Mid-Side (M/S) Encoding / Decoding

The most fundamental stereo processing tool:

```cpp
// Encode L/R → M/S
mid  = (L + R) * 0.5f;
side = (L - R) * 0.5f;

// Decode M/S → L/R
L = mid + side;
R = mid - side;
```

**Processing in M/S domain:**
- Process `mid` independently from `side`
- Narrow: reduce side gain
- Widen: boost side gain
- EQ only the mid (mono-compatible, affects center image)
- EQ only the side (affects stereo width, mono-invisible)

**Widener control (width ∈ [0..2]):**
```cpp
float mid  = (L + R) * 0.5f;
float side = (L - R) * 0.5f;
side *= width;   // width=1.0 → original, 0.0 → full mono, 2.0 → double width
L = mid + side;
R = mid - side;
```

---

## Panning Laws

The gain of left and right channels as a function of pan position θ ∈ [-1, 1]:

### Linear Pan
```cpp
R_gain = (1 + pan) * 0.5f;
L_gain = 1.0f - R_gain;
```
Center: `L = R = 0.5` → -6dB. Common but sounds incorrect — center appears quiet.

### Equal Power Pan (Constant Power)
```cpp
float angle = pan * (π / 4.0f);   // -45° to +45°
L_gain = std::cos(angle + π/4);
R_gain = std::sin(angle + π/4);
```
Center: `L = R = 1/√2 ≈ -3dB`. Most natural for headphone listening.

### -4.5dB Compromise
Many DAWs use a law between -3dB and -6dB at center. Implement by blending linear and equal-power.

---

## Haas Effect Widening (Inter-Aural Time Difference)

Delay one channel by 1–35ms. The ear perceives the sound as coming from the earlier side.
```cpp
R[n] = x[n - delay_samples];   // delay right channel
L[n] = x[n];
```

**Caution**: sounds wide on stereo speakers, but destroys mono compatibility (comb filtering). Always provide a mono check.

**Better alternative**: use a very short unmodulated delay (0.5–1.5ms) + complementary allpass filter on one channel to create psychoacoustic width without comb filtering.

---

## Stereo Correlation Meter

Measures how similar L and R signals are:
```
correlation = Σ(L*R) / sqrt(Σ(L²) * Σ(R²))
```
- `+1.0` = mono (L = R)
- `0.0` = uncorrelated
- `-1.0` = inverse mono (L = -R) — cancels in mono = BAD

Essential for mastering plugins. Display as a "phase scope" (Lissajous figure): plot L vs R on X/Y axes.

**Circular buffer moving correlation**: compute over N samples (window ~300ms for visual display).

---

## HRTF Binaural Processing

Head-Related Transfer Function: models how sound from a specific direction in 3D space is modified by the head and ears before reaching the eardrums.

**Basic binaural spatialization:**
1. Measure or simulate HRTF for target angle (azimuth, elevation)
2. Convolve mono input with left and right HRTF impulse responses
3. Output to headphones

**Free HRTF datasets:**
- **CIPIC database** (UC Davis): `cipic.ucdavis.edu`
- **SOFA format** (AES69): standardized HRTF file format
- **OpenDAFF**: `opendaff.org`

**Simplifications for real-time:**
- Interaural Time Difference (ITD): delay between L and R channels = `sin(azimuth) * head_diameter / c`
- Interaural Level Difference (ILD): amplitude difference between channels
- Pinna spectral shaping: IIR filter approximation of elevation-dependent spectral coloration

---

## Stereo Width Enhancement Techniques

### Spectral Stereo (Multiband Width)
Apply different widths to different frequency bands. Common approach: widen highs (creates "air"), keep lows mono (focused bass).

### Allpass Stereo Decorrelation
```cpp
// Apply first-order allpass to one channel with different coefficient than the other
// Creates decorrelation without tonal change
```
Used in reverb outputs and stereo enhancement. Subtle, relatively mono-compatible.

### Spatial Unfolding / Shuffle
From stereo microphone theory (Blumlein):
```
// For XY coincident stereo:
// Low frequencies: use sum for both channels (LF are essentially mono)
// High frequencies: use full stereo
// Implement with crossover: LF → M/S decode with reduced width, HF → full width
```

---

## Mid-Side EQ Gotchas

- **Phase coherence**: any phase difference between mid and side processing leaks into cross-channel bleed
- **Level matching**: boost side → perceived widening; attenuate side → narrowing/mono
- **Frequency-dependent width**: bass usually kept narrow (≤ 1.0), high mids/highs can be wider

---

## Mono Compatibility Checking

Side signal cancels in mono: `mono = L + R = 2 * mid`. Side = 0 in mono.

**Test**: compare sum `(L+R)` with expected mono version. Any signal that's phase-inverted between L and R will cancel.

**Goniometer/Phase Scope**: standard display. A vertical line = mono, a horizontal line = out-of-phase, a circle = uncorrelated stereo.

---

## Ambisonics (First-Order B-Format)

A 4-channel format for spatial audio: W (omni), X (front-back), Y (left-right), Z (up-down).

**Encode a source at angle θ, elevation φ:**
```
W = 0.707 * source
X = source * cos(φ) * cos(θ)
Y = source * cos(φ) * sin(θ)
Z = source * sin(φ)
```

**Decode B-format to stereo:**
```
L = W + Y
R = W - Y
```

Higher-order ambisonics (HOA) uses more channels for better spatial resolution.

---

## Key Resources

| Source | Topic | URL/Reference |
|--------|--------|----------------|
| AES69-2015 SOFA | HRTF file format standard | `sofaconventions.org` |
| CIPIC HRTF Database | Free measured HRTFs | `cipic.ucdavis.edu/hrtf.html` |
| Michael Gerzon papers | Surround, ambisonics theory | AES preprints (search Gerzon) |
| Zölzer DAFX Ch. 10 | Spatial effects | Wiley |
| Blumlein 1931 patent | Original stereo theory | British Patent 394,325 |
| Reiss & McPherson — "Audio Effects: Theory, Implementation and Application" | Imaging chapter | CRC Press |

---

## Implementation Gotchas

1. **Phase scope display**: Lissajous is drawn by plotting `L[n]` on X axis vs `R[n]` on Y axis, rotated 45°. The visual correlation corresponds to `(L - R)` vs `(L + R)`.
2. **Mid-side decode normalization**: `L = mid + side` (not `* 0.5`). The 0.5 in encode is compensated by the integer addition in decode. Don't double-normalize.
3. **Stereo widening and loudness**: wider = lower perceived loudness (side energy increases but doesn't add loudness linearly). Compensate with makeup gain.
4. **Binaural HRTF for loudspeakers**: don't use headphone HRTFs for speaker-output plugins without speaker virtualization (cross-talk cancellation XTC).
5. **LUFS loudness in stereo**: ITU-R BS.1770 meter sums L and R differently based on channel position — account for this in any loudness metering in stereo bus tools.

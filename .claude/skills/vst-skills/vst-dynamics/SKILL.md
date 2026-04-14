---
name: vst-dynamics
description: >
  Comprehensive reference for building dynamics processing VST plugins including
  compressors, limiters, expanders, gates, and transient shapers. Use this skill
  whenever designing or implementing any dynamic range processor — feed-forward vs
  feedback topologies, RMS vs peak detection, analog hardware emulation, optical
  and FET-style compressors, look-ahead limiting, and sidechain design. Covers
  both clean digital dynamics and analog circuit modeling (LA-2A, 1176, SSL G-Bus,
  Fairchild, dbx 160).
---

# VST Dynamics: Compressors, Limiters, Expanders & Gates

## Foundational Architecture

Every dynamic range processor shares a common signal flow:

```
Input → Level Detector → Gain Computer → Gain Smoother → VCA → Output
                                           ↑
                              (attack/release envelope)
```

**Feed-forward** (modern): detector reads the input signal. Predictable, controllable.
**Feedback** (vintage): detector reads the output signal. Creates the "soft knee" auto-release characteristic of classic opto/vari-mu designs.

---

## Level Detection

### Peak Detection
```
y[n] = |x[n]|
```
Captures transients exactly. Fast. Used in limiters and FET compressors.

### RMS Detection
```
y[n] = sqrt( (1-α) * y[n-1]² + α * x[n]² )
```
Where `α = exp(-1 / (τ * fs))` and `τ` is the RMS window in seconds.
Averages energy. Perceptually correlates better with loudness. Typical for bus compressors.

### Log-Domain Detection
Work in dB throughout: `x_dB = 20 * log10(|x[n]|)`.
Simplifies gain computer math; threshold and ratio are natural in dB.

---

## Gain Computer (Static Characteristic)

**Hard Knee:**
```
if x_dB < T:       y_dB = x_dB              (below threshold, unity gain)
else:              y_dB = T + (x_dB - T) / R (compressed)
```

**Soft Knee** (W = knee width in dB):
```
if x_dB < T - W/2:  y_dB = x_dB
if x_dB > T + W/2:  y_dB = T + (x_dB - T) / R
else:               y_dB = x_dB + ((1/R - 1) * (x_dB - T + W/2)²) / (2*W)
```

**Gain Reduction:** `GR_dB = y_dB - x_dB` (always ≤ 0)

---

## Attack & Release Envelope (Gain Smoothing)

Classic one-pole smoothing (Giannoulis/Massberg/Reiss 2012):
```
if |x[n]| > y[n-1]:    y[n] = α_a * y[n-1] + (1 - α_a) * x[n]   // attack
else:                   y[n] = α_r * y[n-1]                        // release
```
Where `α = exp(-1 / (τ * fs))`

**Program-dependent release** (the classic SSL G-Bus trick): dual-stage release — fast release for brief gain changes, slow release for sustained gain reduction. Implement as two one-poles in parallel or as a state machine.

**Look-ahead**: delay the audio path by `LA` samples and run detection on the pre-delayed signal. Enables true zero-attack limiting. Buffer the input, run your detector N samples ahead.

---

## Hardware Emulation Archetypes

### Opto Compressor (LA-2A, LA-3A style)
- Electro-optical attenuator: the photocell resistance is controlled by a light source
- **Key characteristic**: program-dependent, asymmetric attack/release (fast attack ~10ms, very slow release 40–500ms depending on signal history)
- Model: replace the linear one-pole with a **memoryful** slow time-constant
- The LA-2A has no ratio control — fixed ~3:1 compression
- Paper: Universal Audio's Dave Berners analysis articles (search "LA-2A optical element model")

### FET Compressor (UA 1176 style)
- Very fast attack (20μs–800μs), fast release (50ms–1.1s)
- Fixed 4:1, 8:1, 12:1, 20:1 ratios; all-buttons-in mode yields ~20:1 with unique character
- Input transformer saturation contributes to sound — model with soft saturation on input path
- The "British Mode" (all ratio buttons pressed): feedback loop with multiple gain stages saturating

### VCA Compressor (dbx 160, SSL G-Bus, API 2500)
- Linear-in-dB gain computation, very transparent
- dbx 160: RMS detection, hard knee, characteristic "over-easy" mode = soft knee
- SSL G-Bus: fast attack, program-dependent release, feed-forward with auto-release

### Vari-Mu (Fairchild 670, Manley Vari-Mu)
- Variable-transconductance tube gain reduction
- Feedback topology with very soft, musical knee
- Release time changes with signal level (program-dependent)
- Extremely nonlinear — model with lookup tables derived from tube curves

---

## Transient Shapers

Not a compressor. Operates on the *rate of change* of the envelope (attack transient) rather than the amplitude itself.

```
Transient signal ≈ d/dt (envelope)
```

Attack control: boost/cut the initial transient envelope surge
Sustain control: boost/cut the decaying tail

Implementation: dual-envelope followers with different time constants; their difference encodes transient information.

---

## Limiters

A compressor with very high ratio (∞:1) and very fast attack. Key difference from compressors:
- **True peak limiting**: must oversample or use intersample peak detection (reconstruct via sinc interpolation at 4x minimum)
- **LUFS/loudness normalization**: integrate ITU-R BS.1770 loudness metering into your limiter
- **Look-ahead** is essential for transparent limiting

---

## Key Papers & Resources

| Source | Topic | URL |
|--------|--------|-----|
| Giannoulis, Massberg, Reiss (JAES 2012) | Definitive compressor topology survey | `eecs.qmul.ac.uk/~josh/documents/2012/GiannoulisMassbergReiss...` |
| DAFx proceedings (1998–2024) | Compressor modeling papers | `dafx.de/paper-archive/` |
| Universal Audio WebZine — Berners | DRC analysis, LA-2A model | Search "UA WebZine Berners DRC" |
| Abel & Berners AES 2003 | Peak vs RMS feedback/feedforward | AES Convention Paper 5914 |
| Zoelzer — DAFX Book Ch. 5 | Full compressor chapter | Wiley, 2nd ed |
| ChowDSP Chow Tape + BYOD | Open source compressor + dist code | `chowdsp.com/products.html` |

---

## Reference Implementations

- **ChunkWare SimpleComp** — minimal, well-commented C++ compressor: `github.com/RubinettiMatteo/simple-compressor`
- **BYOD (ChowDSP)** — open source nonlinear circuit modeling: `github.com/Chowdhury-DSP/BYOD`
- **Wolf Shaper** — open-source waveshaper with graphical editor
- **Signalsmith Audio** — excellent blog post on compressor design: `signalsmith-audio.co.uk`

---

## Implementation Gotchas

1. **Gain smoothing before or after gain computer?** Smooth the *gain reduction* signal, not the detector output. Common mistake.
2. **Stereo linking**: for stereo, use `max(L_detection, R_detection)` for the gain computer — never compute gain separately per channel.
3. **Makeup gain**: apply *after* gain computer; auto-makeup = `(T - T/R)` dB.
4. **Ballistics units**: specify attack/release in ms → convert to `α` per sample. Document which definition of "attack time" you use (63% time constant vs -3dB point matters).
5. **DC offset**: high-pass filter the detector path to remove DC before detection.
6. **Oversampling**: run at 2x–4x for nonlinear elements (tube/opto emulation). Downsample after VCA.

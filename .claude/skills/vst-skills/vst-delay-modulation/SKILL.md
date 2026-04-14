---
name: vst-delay-modulation
description: >
  Complete reference for building delay and modulation VST plugins including
  tape delay, digital delay, chorus, flanger, phaser, tremolo, vibrato, rotary
  speaker (Leslie), ring modulation, AM/FM modulation, and BBD bucket-brigade
  device emulation. Covers fractional delay interpolation, LFO design, feedback
  topologies, multi-tap delay, ping-pong delay, and modulated comb filters.
  Use for any time-based or modulation-based effect.
---

# VST Delay & Modulation Effects

## The Delay Line — Foundation of Everything

```cpp
// Circular buffer delay line
class DelayLine {
    std::vector<float> buffer;
    int writePos = 0;
    int size;
public:
    void write(float x)  { buffer[writePos++ & (size-1)] = x; }
    float read(int d)    { return buffer[(writePos - d) & (size-1)]; }
    float readFrac(float d); // fractional delay — see below
};
```

**Fractional delay** is essential for any modulated delay (chorus, flanger, vibrato).

---

## Fractional Delay Interpolation Methods

### Linear Interpolation (fast, poor quality)
```cpp
float readFrac(float d) {
    int i = (int)d;
    float frac = d - i;
    float y0 = read(i), y1 = read(i+1);
    return y0 + frac * (y1 - y0);
}
```
Introduces high-frequency loss (~6dB at Nyquist/4 when frac=0.5). Fine for low modulation depths.

### Hermite 4-point Cubic (quality standard)
```cpp
float hermite4(float frac, float y_1, float y0, float y1, float y2) {
    float c0 = y0;
    float c1 = 0.5f*(y1 - y_1);
    float c2 = y_1 - 2.5f*y0 + 2.f*y1 - 0.5f*y2;
    float c3 = 0.5f*(y2 - y_1) + 1.5f*(y0 - y1);
    return ((c3*frac + c2)*frac + c1)*frac + c0;
}
```
Good quality, low cost. Use for chorus and flanger.

### Allpass Interpolation (best for short delays)
```cpp
// First-order allpass: H(z) = (d - z⁻¹) / (1 - d*z⁻¹)
// where d = (1 - frac) / (1 + frac) for delay of (1-frac) samples
float allpassFrac(float x, float d, float& state) {
    float out = -d * x + state;
    state = x + d * out;
    return out;
}
```
Flat frequency response (no high-frequency loss). Best for flangers where comb coloration matters.

### Sinc (best quality, expensive)
Windowed-sinc interpolation (Lanczos or Kaiser window). Used in high-quality pitch shifting.

---

## LFO Design

```cpp
// Triangle LFO (cheapest, avoidable aliasing)
// Sine LFO (standard — use lookup table or fast approximation)
// Sine approximation (Bhaskara I):
float fastSin(float x) {  // x in [0, π]
    return (16*x*(π-x)) / (5*π*π - 4*x*(π-x));
}

// LFO class pattern:
class LFO {
    float phase = 0, rate, sampleRate;
public:
    float tick() {
        float out = std::sin(2*π*phase);
        phase += rate/sampleRate;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }
};
```

---

## Chorus

A slight pitch shift + detuning by modulating a delay line.

```
input → delay line (6–30ms, modulated) → mix with dry → output
```

**Typical parameters:**
- Delay center: 10–30ms
- LFO depth: ±2–10ms
- LFO rate: 0.2–2 Hz
- Voices: 2–4 delay lines with staggered LFO phases

**Multi-voice chorus** (Juno-style):
```
Voice 1: delay center + LFO1 (0°)
Voice 2: delay center + LFO2 (120°)  
Voice 3: delay center + LFO3 (240°)
Output = dry + sum(voices) / N
```

**Stereo spreading**: pan voices left/right — adjacent voices opposite polarity in right channel.

---

## Flanger

Very short delay (0.5–10ms), modulated + feedback.
The feedback creates a comb filter effect: peaks at multiples of 1/delay.

```
input → short delay (0.5–10ms, modulated) → wet
output = dry + feedback*wet_delayed (positive feedback: "jet" resonance)
```

**Feedback**: `-0.5 to +0.9` range. Positive feedback → flanging peaks, negative → flanging notches.
**Feedforward/feedback combination**: classic "zero flanging" (perfect comb cancellation).

The notch spacing in Hz = `1/delay_in_seconds`.

**Barber-pole flanger** (Barberpole sweep — always sounds like it's rising/falling forever):
Uses multiple stages with offset LFO phases. See Zavalishin "Art of VA Filter Design" ch.6 for allpass-based barberpole phaser.

---

## Phaser

A series of allpass filters whose poles sweep in frequency, creating notches where allpass phase shifts align.

**Single allpass stage:**
```
H(z) = (z⁻¹ - a) / (1 - a*z⁻¹)   where |a| < 1
```
Phase shift goes from 0° (ω=0) to -180° (ω=π) with center at where `cos(ω) = a`.

**N-stage phaser** (N allpasses in series):
- Produces N notches in the frequency response when mixed with dry
- Classic phaser uses 2–12 stages
- LFO modulates pole frequency parameter `a` uniformly across all stages

**Phase shift equation**: total phase shift at frequency ω for N stages:
```
Φ_total(ω) = N * (-π + 2*arctan(sin(ω) / (cos(ω) - a)))
```

Notches occur where `Φ_total(ω) = (2k-1)*π` (odd multiples of π for destructive mixing with dry).

**Small-signal resistor/capacitor phaser**: historical phasers (MXR Phase 90, Uni-Vibe) used cascaded first-order RC stages modulated by an LFO through a JFET/OTA — Zavalishin ch.6 has TPT models.

---

## Tremolo & Vibrato

**Tremolo**: amplitude modulation (AM)
```cpp
y[n] = x[n] * (1 + depth * lfo[n])   // depth ∈ [0, 1]
```
Produces sidebands at ±f_LFO from each frequency component.

**Vibrato**: frequency/pitch modulation (FM)
```cpp
// Implement as modulated fractional delay
y[n] = delayLine.readFrac(baseDelay + depth * lfo[n])
```

**Bias trick**: `(1 + LFO) / 2` biases the modulation to only positive depths (correct for tremolo). Without bias, the AM multiplier goes negative, which inverts the signal (phasiness).

---

## Rotary Speaker (Leslie)

A cabinet with a rotating horn and woofer. Famous on Hammond organ.

**Components:**
1. **Horn (treble)**: rotates at 0.7 Hz (slow/chorale) or 6.5 Hz (fast/tremolo). Creates Doppler pitch modulation + directionality change.
2. **Woofer (bass)**: counter-rotates or rotates at different speed.

**Model:**
```
// Doppler effect: frequency shift from rotation
// At maximum approach: higher pitch
// At maximum recession: lower pitch
// Horn pattern: cardioid (directional) rotating LFO

left[n] = x[n] * directional_amplitude(θ + 90°) * (1 + doppler_depth*sin(θ))
right[n] = x[n] * directional_amplitude(θ - 90°) * (1 + doppler_depth*sin(θ + π))
θ += rotation_speed * 2π / fs
```

For higher realism: also add a comb filter (cabinet resonance) and high-pass for horn vs woofer separation.

---

## Tape Delay Emulation

Beyond basic delay, tape delay adds:
1. **Wow & flutter**: slow pitch instability (wow: <1Hz, flutter: 4–12Hz)
   - Implement as modulated fractional delay on the playback head
   - Flutter depth: 0.01–0.1%
2. **Tape saturation**: see vst-saturation skill
3. **Frequency response**: slight high-frequency rolloff on each pass through tape
   ```cpp
   // In feedback loop, apply first-order lowpass per tap
   y = fc * x + (1 - fc) * prev; // fc ≈ 0.85–0.95
   ```
4. **Head bump**: resonant low-frequency boost (60–100 Hz)
5. **Noise**: add very subtle tape noise (shaped pink noise, very low level)

---

## BBD (Bucket-Brigade Device) Emulation

BBD ICs (MN3005, MN3007, SAD1024) create delay by shifting charge through capacitors.
- Not a clean digital delay — each stage is a sample-and-hold
- Clock frequency controls delay time: `delay = N_stages / (2 * f_clock)`
- Each bucket capacitor adds noise
- Anti-aliasing filter (clock/2 LPF) and reconstruction filter both colorize the sound

**Model:**
1. Apply input anti-aliasing LPF (1st or 2nd order, cutoff = f_clock/2)
2. Delay by target samples
3. Apply output smoothing LPF (same cutoff)
4. Add clock noise: high-frequency noise correlated with the clock
5. Optionally add capacitor droop: slight high-frequency loss per stage

---

## Ping-Pong Delay

```
L input → delay A → R output
R input → delay B → L output
Feedback: R output → delay A input (with fb gain)
```

Each ping pong makes one pass. For polyrhythmic delays, use different time for L and R.

---

## Multi-Tap Delay

Multiple read heads at different points in the delay line. Each tap can have:
- Independent volume
- Independent panning
- Independent filtering
- Individual feedback amounts

```cpp
for (auto& tap : taps) {
    float delayed = delayLine.readFrac(tap.time * fs);
    output += delayed * tap.gain;
    feedback_input += delayed * tap.feedback;
}
```

---

## Key Resources

| Source | Topic | URL/Reference |
|--------|--------|----------------|
| Zavalishin VA Filter Design ch.6 | Phasers, flangers (allpass-based) | `native-instruments.com/...VAFilterDesign_2.1.0.pdf` |
| JOS CCRMA — PASP | Delay effects: phasing, flanging, Leslie | `ccrma.stanford.edu/~jos/pasp/` |
| Zölzer DAFX Book Ch. 6 | Modulation effects | Wiley 2nd ed |
| Signalsmith Audio — "Let's Write a Reverb" | Chorus/delay implementation | `signalsmith-audio.co.uk/writing` |
| Smith & Cook 1992 | Allpass interpolation | ICMC 1992 |
| Dattorro 1997 Part 2 | Chorus and flanging | JAES 1997 |

---

## Implementation Gotchas

1. **Maximum delay buffer**: allocate buffer at `max_delay_ms * fs / 1000 + interpolation_margin` samples. Add 4 extra samples for Hermite interpolation lookback.
2. **Feedback stability**: feedback paths containing filtering can become unstable at near-unity gain. Limit feedback to < 0.98 for stability in typical designs, or use a saturator in the loop.
3. **Zipper noise on delay time change**: when changing delay time, interpolate smoothly. Use a one-pole LP on the delay time control parameter (not the audio).
4. **Block processing with delay**: if the delay time is shorter than your block size, you may need per-sample processing for that delay line to avoid incorrect ordering.
5. **Flanger zero sweep alignment**: the deepest comb notch occurs when delay = 0 samples. Setting LFO to swing through ~0 creates the characteristic "zero sweep" which can cause DC breakthrough — add a small minimum delay (0.1ms).

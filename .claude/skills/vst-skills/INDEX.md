---
name: vst-master-index
description: >
  Master index for the open-source VST plugin development skill bundle. Use this
  as the entry point when starting to build any VST plugin, to navigate to the
  right domain-specific skill file. Contains cross-cutting implementation guidance,
  framework references, plugin format overview (VST3, AU, CLAP, LV2), and links
  to all 8 domain skill files.
---

# VST Plugin Development — Master Index

## Domain Skill Files

| Skill File | Domain | Key Topics |
|------------|--------|------------|
| `vst-dynamics/SKILL.md` | Compressors, limiters, gates | Feed-forward/back, RMS/peak, LA-2A, 1176, VCA |
| `vst-eq-filters/SKILL.md` | EQ, parametric, shelves, filters | Biquad, ZDF/TPT, SVF, ladder, Neve/SSL/Pultec |
| `vst-saturation/SKILL.md` | Distortion, tubes, tape, waveshaping | ADAA, polyBLAMP, oversampling, J-A hysteresis, LSTM |
| `vst-reverb/SKILL.md` | Reverb algorithms, IR, spring | FDN, Schroeder, Dattorro plate, Jot, convolution |
| `vst-delay-modulation/SKILL.md` | Chorus, flanger, phaser, tremolo, delay | Fractional delay, BBD, allpass, Leslie, tape delay |
| `vst-synthesis/SKILL.md` | VA synths, FM, wavetable, physical | polyBLEP, DX7-FM, Karplus-Strong, waveguide |
| `vst-imaging/SKILL.md` | Stereo, M/S, binaural, panning | M/S encode, HRTF, Haas, correlation meter |
| `vst-pitch-time/SKILL.md` | Pitch shift, time stretch, pitch correction | Phase vocoder, PSOLA, granular, YIN, formants |

---

## Cross-Cutting Resources (All Domains)

### Essential Free Textbooks
| Book | Author | URL |
|------|--------|-----|
| Introduction to Digital Filters | Julius O. Smith III | `ccrma.stanford.edu/~jos/filters/` |
| Physical Audio Signal Processing | Julius O. Smith III | `ccrma.stanford.edu/~jos/pasp/` |
| Spectral Audio Signal Processing | Julius O. Smith III | `ccrma.stanford.edu/~jos/sasp/` |
| Mathematics of the DFT | Julius O. Smith III | `ccrma.stanford.edu/~jos/mdft/` |
| The Art of VA Filter Design | Vadim Zavalishin | `native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf` |

### Essential Conference Archives
| Source | URL | Notes |
|--------|-----|-------|
| DAFx Proceedings (1998–2024) | `dafx.de/paper-archive/` | Fully open access. Best directly applicable research. |
| arXiv cs.SD / eess.AS | `arxiv.org` | Preprints. Current ML-audio research. |
| AES (Audio Eng. Society) | `aes.org` | Paywall but abstracts free; authors self-host |
| ICASSP / ISMIR | IEEE / ISMIR.net | Signal processing and music IR |

### Framework & Plugin Format References
| Framework | Format | URL |
|-----------|--------|-----|
| JUCE | VST3, AU, AAX, CLAP, LV2, Standalone | `juce.com` |
| iPlug2 | VST2/3, AU, AAX, CLAP, Web | `iplug2.github.io` |
| CLAP SDK | CLAP (modern open format) | `github.com/free-audio/clap` |
| DPF | LV2, VST2/3, JACK | `github.com/DISTRHO/DPF` |
| VST3 SDK | VST3 | `github.com/steinbergmedia/vst3sdk` |

### Open Source Reference Implementations
| Plugin | Domain | Source |
|--------|--------|--------|
| ChowDSP BYOD | Distortion, compressor, filter | `github.com/Chowdhury-DSP/BYOD` |
| ChowTape | Tape saturation (Jiles-Atherton) | `github.com/Chowdhury-DSP/AnalogTapeModel` |
| Surge XT | Full synth (VA, FM, wavetable) | `github.com/surge-synthesizer/surge` |
| Dexed | DX7 FM emulator | `github.com/asb2m10/dexed` |
| ZynAddSubFX | Additive synthesis | `github.com/zynaddsubfx/zynaddsubfx` |
| Rubber Band | Time stretch / pitch shift | `github.com/breakfastquay/rubberband` |
| RTNeural | Real-time neural audio | `github.com/jatinchowdhury18/RTNeural` |
| Freeverb | Schroeder reverb | Search "freeverb3 source" |
| Zita-rev1 | FDN reverb (C++) | `kokkinizita.linuxaudio.org` |

---

## Universal DSP Concepts

### Sample Rate Independence
Never hardcode sample-rate-dependent values. Always scale by `fs`:
```cpp
float alpha = std::exp(-1.0f / (timeConstant_ms * 0.001f * sampleRate));
```

### Avoiding Denormals
Denormal (subnormal) floats cause catastrophic CPU slowdowns in feedback paths.
Solutions:
```cpp
// Option 1: FTZ (Flush-to-Zero) mode
_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

// Option 2: Add tiny DC offset to state variables
state += 1e-30f;

// Option 3: Conditional zero:
if (std::abs(state) < 1e-15f) state = 0.0f;
```

### Smoothing Parameter Changes
Never apply abrupt parameter changes — always smooth to avoid clicks and zipper noise:
```cpp
// One-pole LP smoother
const float SMOOTH_COEF = std::exp(-2.0f*M_PI * 20.0f / sampleRate);  // 20 Hz cutoff
currentValue = SMOOTH_COEF * currentValue + (1.0f - SMOOTH_COEF) * targetValue;
```

### Block Processing
Process audio in blocks (typically 64–2048 samples). Keep per-sample overhead minimal. Batch-compute parameters at block boundaries, then process samples in inner loop.

---

## Hardware Schematics & Circuit Analysis Sources

| Source | Content | URL |
|--------|---------|-----|
| Gyraf Audio (Georg Yordanov) | Neve, Fairchild, LA-2A schematics + analysis | `gyraf.dk` |
| Elliott Sound Products | Dense analog circuit analysis | `sound-au.com` |
| Valve Wizard | Tube amp design theory | `valvewizard.co.uk` |
| GeoFex (RG Keen) | Guitar pedal circuit teardowns | `geofex.com` |
| GroupDIY | Forum with schematics, service manuals | `groupdiy.com` |
| Service manuals | Original manufacturer schematics | Search `[hardware name] service manual site:archive.org` |

---

## DSP Community

| Source | URL |
|--------|-----|
| KVR DSP Forum | `kvraudio.com/forum/viewforum.php?f=33` |
| Signalsmith Audio Blog | `signalsmith-audio.co.uk/writing` |
| musicdsp.org | `musicdsp.org` |
| Valhalla DSP Blog | `valhalladsp.com/blog` |
| ADC (Audio Dev Conf) talks | `youtube.com` → search "Audio Developer Conference" |

---

## Decision Guide: Which Skill to Load

| You're building... | Load skill |
|--------------------|-----------|
| A compressor, limiter, gate, expander | `vst-dynamics` |
| A parametric EQ, graphic EQ, shelving EQ, low-pass/high-pass filter | `vst-eq-filters` |
| Tube amp, tape sat, overdrive, distortion, fuzz, bitcrusher | `vst-saturation` |
| Reverb (hall, room, plate, spring) | `vst-reverb` |
| Chorus, flanger, phaser, delay, tape delay, tremolo | `vst-delay-modulation` |
| Synthesizer (VA, FM, wavetable, physical, additive) | `vst-synthesis` |
| Stereo widener, M/S plugin, panner, binaural | `vst-imaging` |
| Pitch corrector, time stretcher, pitch shifter, formant | `vst-pitch-time` |
| Guitar amp sim | `vst-saturation` + `vst-eq-filters` (tone stack) |
| Tape machine emulation | `vst-saturation` (J-A model) + `vst-delay-modulation` (wow/flutter) |
| Drum machine or physical instrument | `vst-synthesis` (Karplus-Strong, modal) |

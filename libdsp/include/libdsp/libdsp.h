#pragma once

// libdsp — Hungry Ghost DSP library
// Pure C++17, no JUCE dependency

// Utilities
#include "util/Constants.h"
#include "util/SmoothParameter.h"
#include "util/MidSide.h"
#include "util/Oversampler.h"
#include "util/LevelMeter.h"

// Dynamics
#include "dynamics/LevelDetector.h"
#include "dynamics/GainComputer.h"
#include "dynamics/Compressor.h"

// Filters
#include "filters/SVFFilter.h"
#include "filters/LadderFilter.h"
#include "filters/BiquadCascade.h"
#include "filters/Coefficients.h"

// Nonlinear
#include "nonlinear/WaveShaper.h"
#include "nonlinear/ADAA.h"

// Modulation
#include "modulation/LFO.h"
#include "modulation/FractionalDelay.h"
#include "modulation/Chorus.h"

// Reverb
#include "reverb/DelayLine.h"
#include "reverb/FDN.h"
#include "reverb/AllpassChain.h"

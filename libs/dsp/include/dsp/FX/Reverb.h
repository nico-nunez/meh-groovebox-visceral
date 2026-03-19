#pragma once

#include "dsp/Buffers.h"

#include <cstddef>

namespace dsp::fx::reverb {
inline constexpr float DATTORRO_SR = 29761.0f;

// Input diffusers
inline constexpr float APD1_LEN_29K = 142.0f;
inline constexpr float APD2_LEN_29K = 107.0f;
inline constexpr float APD3_LEN_29K = 379.0f;
inline constexpr float APD4_LEN_29K = 277.0f;

// Tank loop A
inline constexpr float MOD_A_LEN_29K = 672.0f;
inline constexpr float DLY1_A_LEN_29K = 4453.0f;
inline constexpr float APF_A_LEN_29K = 1800.0f;
inline constexpr float DLY2_A_LEN_29K = 3720.0f;

// Tank loop B
inline constexpr float MOD_B_LEN_29K = 908.0f;
inline constexpr float DLY1_B_LEN_29K = 4217.0f;
inline constexpr float APF_B_LEN_29K = 2656.0f;
inline constexpr float DLY2_B_LEN_29K = 3163.0f;

// Maximum LFO swing in samples (user modDepth maps to [0, MAX_MOD_DEPTH_SAMPLES])
inline constexpr float MAX_MOD_DEPTH_SAMPLES = 16.0f;

// Tank allpass coefficient scale (paper constant, not user-controllable)
inline constexpr float TANK_DIFF_COEFF = 0.7f;

// Input diffusion coefficient ratio between APD1/2 and APD3/4 (paper: 0.75 vs 0.625)
inline constexpr float DIFF_INNER_RATIO = 0.625f / 0.75f; // ≈ 0.8333

enum DattorroLine : uint8_t {
  APD1 = 0,
  APD2,
  APD3,
  APD4, // input diffusers
  MOD_A,
  DLY1_A,
  APF_A,
  DLY2_A, // tank loop A
  MOD_B,
  DLY1_B,
  APF_B,
  DLY2_B, // tank loop B
  NUM_DATTORRO_LINES
};

struct DattorroState {
  // --- Flat allocation ---
  float* buf = nullptr; // all 12 lines, contiguous

  // --- Per-line metadata (index via DattorroLine enum) ---
  size_t offset[NUM_DATTORRO_LINES] = {}; // start of each line within buf
  size_t len[NUM_DATTORRO_LINES] = {};    // length of each line (modulo divisor)
  size_t modNominal[2] = {};              // nominal delay for MOD_A [0], MOD_B [1]
  size_t head[NUM_DATTORRO_LINES] = {};   // write head per line

  // --- Output tap offsets (scaled from Dattorro 1997 Table 1 in initReverb) ---
  // 7 unique paper offsets; R output reuses the same tap[] values with A↔B line swap.
  // ASPIRATIONAL: verify all 7 offsets against the paper before use.
  size_t tap[7] = {};

  // --- Input bandwidth LP filter state ---
  float bwState = 0.0f; // single one-pole LP

  // --- Tank damping LP filter state (one per loop) ---
  float dampA = 0.0f;
  float dampB = 0.0f;

  // --- Tank damping HP filter state (one per loop) ---
  float lowDampA = 0.0f;
  float lowDampB = 0.0f;

  // --- Modulation LFO state (quadrature, per tank loop) ---
  // Initialized at 0° (A) and 90° (B) for spatial decorrelation
  float lfoAre = 1.0f, lfoAim = 0.0f;
  float lfoBre = 0.0f, lfoBim = 1.0f;

  // --- Pre-delay (separate: length is param-driven, not algorithm-defined) ---
  float* preDelayBuf = nullptr;
  size_t preDelayLen = 0; // allocated capacity
  size_t preDelayHead = 0;

  // --- DC blocking (one-pole high-pass on wet output, fc ≈ 8 Hz at 48kHz) ---
  float dcPrevL = 0.0f, dcPrevR = 0.0f; // x[n-1] per channel
  float dcOutL = 0.0f, dcOutR = 0.0f;   // y[n-1] per channel
};

struct ReverbFX {
  // --- User params ---
  float preDelay = 0.0f;   // ms [0, 100]
  float decay = 0.5f;      // [0, 1] — tank feedback + APF coefficient
  float damping = 0.5f;    // [0, 1] — tank LP intensity
  float lowDamping = 0.0f; // [0, 1] — tank HP intensity
  float bandwidth = 0.75f; // [0, 1] — input LP pass-through amount
  float diffusion = 0.75f; // [0, 1] — input APF coefficient scale (new)
  float modRate = 0.5f;    // Hz [0.01, 5.0] (new)
  float modDepth = 0.5f;   // [0, 1] → [0, 16] samples (new)
  float mix = 0.3f;
  bool enabled = false;

  // --- Derived (set by ReverbDerived UpdateGroup) ---
  float dampCoeff = 0.5f;       // = 1.0f - damping
  float lowDampCoeff = 0.0f;    // = 1.0f - lowDamping
  float preDelaySamples = 0.0f; // = preDelay * sampleRate / 1000.0f
  float modDepthSamples = 8.0f; // = modDepth * MAX_MOD_DEPTH_SAMPLES
  float cosW = 1.0f;            // quadrature oscillator step
  float sinW = 0.0f;
  float decayDiffusion = 0.525f; // = decay * TANK_DIFF_COEFF (= decay * 0.7f)

  DattorroState state;
};

void initReverb(ReverbFX& fx, float sampleRate);
void destroyReverb(ReverbFX& fx);

void recalcReverbDerivedVals(ReverbFX& fx, float sampleRate);

void processReverb(ReverbFX& reverb, buffers::StereoBuffer buf, size_t numSamples);
} // namespace dsp::fx::reverb

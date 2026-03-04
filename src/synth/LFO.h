#pragma once

#include "synth/ModMatrix.h"
#include "synth/WavetableBanks.h"

namespace synth::lfo {
using mod_matrix::ModDest;
using wavetable::banks::WavetableBank;

struct LFO {
  const WavetableBank* bank = nullptr; // null = S&H mode; set to getBankByID(BankID::Sine) at init
  float phase = 0.0f;                  // normalized [0.0, 1.0)
  float rate = 1.0f;                   // Hz, no DSP ceiling; UI default range [0.0, 20.0]
  float amplitude = 1.0f;              // output range [-amplitude, +amplitude]
  float shHeld = 0.0f;                 // S&H: last held random value
  bool retrigger = false;              // reset phase on note-on?
};

struct LFOModState {
  float lfo1;
  float lfo2;
  float lfo3;

  float contribs[ModDest::DEST_COUNT];
};

void clearContribs(LFOModState& modState);

float advanceLFO(LFO& lfo, float invSampleRate, float effectiveRate, float effectiveAmplitude);

} // namespace synth::lfo

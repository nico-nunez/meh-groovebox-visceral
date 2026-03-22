#include "LFO.h"
#include "dsp/Math.h"

namespace synth::lfo {

void clearContribs(LFOModState& modState) {
  for (auto& destContrib : modState.contribs)
    destContrib = 0.0f;
}

float advanceLFO(LFO& lfo, float invSampleRate, float effectiveRate, float effectiveAmplitude) {
  lfo.phase += effectiveRate * invSampleRate;
  bool wrapped = lfo.phase >= 1.0f;
  if (wrapped)
    lfo.phase -= 1.0f;

  // Fade-in envelope: only active for retriggered LFOs
  float fadeScale = 1.0f;
  if (lfo.retrigger) {
    if (lfo.delayTimer > 0.0f) {
      lfo.delayTimer -= 1.0f;
      return 0.0f; // silent during delay; no further processing needed
    }
    if (lfo.attackTimer > 0.0f) {
      if (lfo.attackCount >= 1.0f)
        fadeScale = 1.0f - (lfo.attackTimer / lfo.attackCount);
      lfo.attackTimer -= 1.0f;
    }
  }

  // S&H: null bank is the sentinel — see Sample and Hold
  if (lfo.bank == nullptr) {
    if (wrapped)
      lfo.shHeld = dsp::math::randNoiseValue();
    return fadeScale * effectiveAmplitude * lfo.shHeld;
  }

  float tablePhase = lfo.phase * dsp::wavetable::TABLE_SIZE_F;
  return fadeScale * effectiveAmplitude *
         dsp::wavetable::readTable(lfo.bank->frames[0].mips[0], tablePhase);
}
} // namespace synth::lfo

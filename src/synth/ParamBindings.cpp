#include "ParamBindings.h"

#include "dsp/FX/Distortion.h"
#include "synth/Envelope.h"
#include "synth/Filters.h"
#include "synth/LFO.h"
#include "synth/MonoMode.h"
#include "synth/Noise.h"
#include "synth/ParamDefs.h"
#include "synth/Saturator.h"
#include "synth/Tempo.h"
#include "synth/Types.h"
#include "synth/Unison.h"
#include "synth/VoicePool.h"
#include "synth/WavetableOsc.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace synth::param::bindings {
using voices::VoicePool;
using wavetable::osc::WavetableOsc;

// Anonymous Helpers
namespace {
namespace dist = dsp::fx::distortion;

ParamBinding makeFloatBinding(float* ptr) {
  ParamBinding b;
  b.floatPtr = ptr;
  return b;
}
ParamBinding makeInt8Binding(int8_t* ptr) {
  ParamBinding b;
  b.int8Ptr = ptr;
  return b;
}
ParamBinding makeBoolBinding(bool* ptr) {
  ParamBinding b;
  b.boolPtr = ptr;
  return b;
}

/* FilterMode is the only enum with a binding — it has a small fixed range (0–3)
 * suitable for MIDI CC and potential mod destination use. Other enums (bank,
 * fmSource, noise type) resolve to pointers or string lookups and are direct-write. */
ParamBinding makeFilterModeBinding(SVFMode* ptr) {
  ParamBinding b;
  b.svfModePtr = ptr;
  return b;
}

ParamBinding makeDistortionTypeBinding(dist::DistortionType* ptr) {
  ParamBinding b;
  b.distortionTypePtr = ptr;
  return b;
}

void bindOscillator(ParamBinding* bindings, const OscParamIDs& ids, WavetableOsc& osc) {
  bindings[ids.mixLevel] = makeFloatBinding(&osc.mixLevel);
  bindings[ids.detune] = makeFloatBinding(&osc.detuneAmount);
  bindings[ids.octave] = makeInt8Binding(&osc.octaveOffset);
  bindings[ids.scanPos] = makeFloatBinding(&osc.scanPos);
  bindings[ids.fmDepth] = makeFloatBinding(&osc.fmDepth);
  bindings[ids.fmRatio] = makeFloatBinding(&osc.fmRatio);
  bindings[ids.enabled] = makeBoolBinding(&osc.enabled);
}

// Noise Oscillator Binding
void bindNoise(ParamBinding* bindings, noise::Noise& noise) {
  bindings[NOISE_MIX_LEVEL] = makeFloatBinding(&noise.mixLevel);
  bindings[NOISE_ENABLED] = makeBoolBinding(&noise.enabled);
}

// Filter Bindings
void bindSVFilter(ParamBinding* bindings, filters::SVFilter& filter) {
  bindings[SVF_MODE] = makeFilterModeBinding(&filter.mode);
  bindings[SVF_CUTOFF] = makeFloatBinding(&filter.cutoff);
  bindings[SVF_RESONANCE] = makeFloatBinding(&filter.resonance);
  bindings[SVF_ENABLED] = makeBoolBinding(&filter.enabled);
}

void bindLadderFilter(ParamBinding* bindings, filters::LadderFilter& filter) {
  bindings[LADDER_CUTOFF] = makeFloatBinding(&filter.cutoff);
  bindings[LADDER_RESONANCE] = makeFloatBinding(&filter.resonance);
  bindings[LADDER_DRIVE] = makeFloatBinding(&filter.drive);
  bindings[LADDER_ENABLED] = makeBoolBinding(&filter.enabled);
}

// Saturator Bindings
void bindSaturator(ParamBinding* bindings, saturator::Saturator& saturator) {
  bindings[SATURATOR_DRIVE] = makeFloatBinding(&saturator.drive);
  bindings[SATURATOR_MIX] = makeFloatBinding(&saturator.mix);
  bindings[SATURATOR_ENABLED] = makeBoolBinding(&saturator.enabled);
}

// LFO Binding
void bindLFO(ParamBinding* bindings, LFOParamIDs ids, lfo::LFO& lfo) {
  bindings[ids.rate] = makeFloatBinding(&lfo.rate);
  bindings[ids.amplitude] = makeFloatBinding(&lfo.amplitude);
  bindings[ids.retrigger] = makeBoolBinding(&lfo.retrigger);
  bindings[ids.tempoSync] = makeBoolBinding(&lfo.tempoSync);
}

// Envelope Bindings
void bindEnvelope(ParamBinding* bindings, EnvParamIDs ids, envelope::Envelope& env) {
  bindings[ids.attack] = makeFloatBinding(&env.attackMs);
  bindings[ids.decay] = makeFloatBinding(&env.decayMs);
  bindings[ids.sustain] = makeFloatBinding(&env.sustainLevel);
  bindings[ids.release] = makeFloatBinding(&env.releaseMs);
  bindings[ids.attackCurve] = makeFloatBinding(&env.attackCurveParam);
  bindings[ids.decayCurve] = makeFloatBinding(&env.decayCurveParam);
  bindings[ids.releaseCurve] = makeFloatBinding(&env.releaseCurveParam);
}

// Mono Bindings
void bindMono(ParamBinding* bindings, mono::MonoState& mono) {
  bindings[MONO_ENABLED] = makeBoolBinding(&mono.enabled);
  bindings[MONO_LEGATO] = makeBoolBinding(&mono.legato);
}

// Portamento Bindings
void bindPorta(ParamBinding* bindings, voices::Portamento& porta) {
  bindings[PORTA_TIME] = makeFloatBinding(&porta.time);
  bindings[PORTA_LEGATO] = makeBoolBinding(&porta.legato);
  bindings[PORTA_ENABLED] = makeBoolBinding(&porta.enabled);
}

void bindUnison(ParamBinding* bindings, unison::UnisonState& uni) {
  bindings[UNISON_VOICES] = makeInt8Binding(&uni.voices);
  bindings[UNISON_DETUNE] = makeFloatBinding(&uni.detune);
  bindings[UNISON_SPREAD] = makeFloatBinding(&uni.spread);
  bindings[UNISON_ENABLED] = makeBoolBinding(&uni.enabled);
}

void initMIDIBindings(ParamRouter& router) {
  for (auto& cc : router.midiBindings)
    cc = ParamID::UNKNOWN;

  router.midiBindings[7] = ParamID::MASTER_GAIN;
  router.midiBindings[74] = ParamID::SVF_CUTOFF;
  router.midiBindings[71] = ParamID::SVF_RESONANCE;
}

// =========== Effects =============
void bindDistortion(ParamRouter& r, dist::DistortionFX& d) {
  r.paramBindings[FX_DISTORTION_DRIVE] = makeFloatBinding(&d.drive);
  r.paramBindings[FX_DISTORTION_MIX] = makeFloatBinding(&d.mix);
  r.paramBindings[FX_DISTORTION_ENABLED] = makeBoolBinding(&d.enabled);
  r.paramBindings[FX_DISTORTION_TYPE] = makeDistortionTypeBinding(&d.type);
}

} // namespace

// ==== APIs ====

void initParamRouter(ParamRouter& router, voices::VoicePool& pool, tempo::TempoState& tempo) {

  bindOscillator(router.paramBindings, OSC_PARAM_IDS[0], pool.osc1);
  bindOscillator(router.paramBindings, OSC_PARAM_IDS[1], pool.osc2);
  bindOscillator(router.paramBindings, OSC_PARAM_IDS[2], pool.osc3);
  bindOscillator(router.paramBindings, OSC_PARAM_IDS[3], pool.osc4);

  bindEnvelope(router.paramBindings, ENV_PARAM_IDS[0], pool.ampEnv);
  bindEnvelope(router.paramBindings, ENV_PARAM_IDS[1], pool.filterEnv);
  bindEnvelope(router.paramBindings, ENV_PARAM_IDS[2], pool.modEnv);

  bindLFO(router.paramBindings, LFO_PARAM_IDS[0], pool.lfo1);
  bindLFO(router.paramBindings, LFO_PARAM_IDS[1], pool.lfo2);
  bindLFO(router.paramBindings, LFO_PARAM_IDS[2], pool.lfo3);

  bindNoise(router.paramBindings, pool.noise);
  bindSVFilter(router.paramBindings, pool.svf);
  bindLadderFilter(router.paramBindings, pool.ladder);
  bindSaturator(router.paramBindings, pool.saturator);
  bindMono(router.paramBindings, pool.mono);
  bindPorta(router.paramBindings, pool.porta);
  bindUnison(router.paramBindings, pool.unison);

  router.paramBindings[PITCH_BEND_RANGE] = makeFloatBinding(&pool.pitchBend.range);
  router.paramBindings[MASTER_GAIN] = makeFloatBinding(&pool.masterGain);
  router.paramBindings[BPM] = makeFloatBinding(&tempo.bpm);

  initMIDIBindings(router);
}

void initFXParamBindings(ParamRouter& router, fx_chain::FXChain& fxChain) {
  bindDistortion(router, fxChain.distortion);
}

ParamID
handleMIDICC(ParamRouter& router, VoicePool& pool, uint8_t cc, uint8_t value, float sampleRate) {
  if (cc == 1) {
    pool.modWheelValue = value / 127.0f;
    return ParamID::UNKNOWN;
  }
  if (cc == 64) {
    bool wasHeld = pool.sustain.held;
    pool.sustain.held = (value >= 64);

    if (wasHeld && !pool.sustain.held) {
      // Mono: handle the one voice directly
      if (pool.mono.enabled) {
        uint32_t v = pool.mono.voiceIndex;

        if (v < MAX_VOICES && pool.sustain.notes[v]) {
          pool.sustain.notes[v] = false;

          if (pool.mono.stackDepth > 0) {
            uint8_t prevNote = pool.mono.noteStack[pool.mono.stackDepth - 1];

            if (pool.porta.enabled) {
              float from = static_cast<float>(pool.midiNotes[v]);
              float to = static_cast<float>(prevNote);
              pool.porta.offsets[v] = (from - to) + pool.porta.offsets[v];
              pool.porta.lastNote = prevNote;
            }

            voices::redirectVoicePitch(pool, v, prevNote, sampleRate);
          } else {
            envelope::triggerRelease(pool.ampEnv, v);
            envelope::triggerRelease(pool.filterEnv, v);
            envelope::triggerRelease(pool.modEnv, v);
          }
        }
        return ParamID::UNKNOWN;
      }

      // Poly: release all deferred voices
      for (uint32_t i = 0; i < MAX_VOICES; ++i) {
        if (pool.sustain.notes[i]) {
          pool.sustain.notes[i] = false;
          envelope::triggerRelease(pool.ampEnv, i);
          envelope::triggerRelease(pool.filterEnv, i);
          envelope::triggerRelease(pool.modEnv, i);
        }
      }
    }
    return ParamID::UNKNOWN;
  }

  ParamID paramID = router.midiBindings[cc];
  if (paramID == ParamID::UNKNOWN)
    return paramID;

  const auto& def = getParamDef(paramID);
  float denorm = def.min + (value / 127.0f) * (def.max - def.min);

  setParamValue(router, paramID, denorm);

  return paramID;
}

// ========================
// Getters / Setters
// ========================

// Get param value by ID (normalized value)
// Retreives current denormalized and returns normalized value
float getParamValueByID(const ParamRouter& router, ParamID id) {
  if (id < 0 || id >= PARAM_COUNT)
    return 0.0f;

  const auto& def = param::getParamDef(id);
  const ParamBinding& binding = router.paramBindings[id];

  switch (def.type) {
  case ParamType::Float:
    return *binding.floatPtr;

  case ParamType::Int8:
    return static_cast<float>(*binding.int8Ptr);

  case ParamType::Bool:
    return *binding.boolPtr ? 1.0f : 0.0f;

  case ParamType::FilterMode:
    return static_cast<float>(*binding.svfModePtr);

  case ParamType::DistortionType:
    return static_cast<float>(*binding.distortionTypePtr);
  }

  return 0.0f;
}

// Set param value by ID
// Expects normalized values, denormalizes, and updates value
void setParamValue(ParamRouter& router, ParamID id, float value) {
  if (id < 0 || id >= PARAM_COUNT)
    return;

  const auto& def = param::getParamDef(id);
  ParamBinding& binding = router.paramBindings[id];

  value = std::clamp(value, def.min, def.max);

  switch (def.type) {
  case ParamType::Float:
    *binding.floatPtr = value;
    break;
  case ParamType::Int8:
    *binding.int8Ptr = static_cast<int8_t>(std::round(value));
    break;
  case ParamType::Bool:
    *binding.boolPtr = value >= 0.5f;
    break;
  case ParamType::FilterMode:
    *binding.svfModePtr = static_cast<SVFMode>(static_cast<int>(std::round(value)));
    break;
  case ParamType::DistortionType:
    *binding.distortionTypePtr =
        static_cast<dist::DistortionType>(static_cast<int>(std::round(value)));
    break;
  }
}

ParamID getParamIDByName(const char* name) {
  for (size_t i = 0; i < PARAM_DEF_COUNT; i++) {
    if (strcmp(PARAM_DEFS[i].name, name) == 0) {
      return static_cast<ParamID>(i);
    }
  }
  return ParamID::UNKNOWN;
}

// ParamID → String (for 'get' commands, help text, errors)
const char* getParamName(ParamID id) {
  if (id >= 0 && id < PARAM_COUNT - 1)
    return param::PARAM_DEFS[id].name;
  return nullptr;
}

// Print parameter options
void printParamList(const char* optionalParam) {
  if (optionalParam != nullptr) {
    printf("Available parameters for: %s\n", optionalParam);
    for (const auto& def : PARAM_DEFS) {
      if (strstr(def.name, optionalParam) != NULL)
        printf("  %s\n", def.name);
    }
  } else {
    printf("Available parameters:\n");
    for (const auto& def : PARAM_DEFS) {
      printf("  %s\n", def.name);
    }
  }
}

} // namespace synth::param::bindings

#include "ParamBindings.h"

#include "Envelope.h"
#include "dsp/Math.h"
#include "synth/Filters.h"
#include "synth/LFO.h"
#include "synth/MonoMode.h"
#include "synth/Noise.h"
#include "synth/ParamRanges.h"
#include "synth/Saturator.h"
#include "synth/VoicePool.h"
#include "synth/WavetableOsc.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace synth::param::bindings {
using voices::VoicePool;

// Anonymous Helpers
namespace {

ParamBinding makeParamBinding(bool* ptr) {
  ParamBinding binding;
  binding.boolPtr = ptr;
  binding.type = BOOL;
  binding.min = 0.0f;
  binding.max = 1.0f;
  return binding;
}

ParamBinding makeParamBinding(int8_t* ptr, int min, int max) {
  ParamBinding binding;
  binding.int8Ptr = ptr;
  binding.type = INT8;
  binding.min = static_cast<float>(min);
  binding.max = static_cast<float>(max);
  return binding;
}

ParamBinding makeParamBinding(float* ptr, float min, float max) {
  ParamBinding binding;
  binding.floatPtr = ptr;
  binding.type = FLOAT;
  binding.min = min;
  binding.max = max;
  return binding;
}

ParamBinding makeParamBinding(SVFMode* ptr, int min, int max) {
  ParamBinding binding;
  binding.svfModePtr = ptr;
  binding.type = FILTER_MODE;
  binding.min = static_cast<float>(min);
  binding.max = static_cast<float>(max);
  return binding;
}

// Oscillator Bindings
void bindOscillator(ParamBinding* bindings, ParamID baseId, wavetable::osc::WavetableOsc& osc) {
  bindings[baseId + 0] =
      makeParamBinding(&osc.mixLevel, ranges::osc::MIX_LEVEL_MIN, ranges::osc::MIX_LEVEL_MAX);
  bindings[baseId + 1] =
      makeParamBinding(&osc.detuneAmount, ranges::osc::DETUNE_MIN, ranges::osc::DETUNE_MAX);
  bindings[baseId + 2] =
      makeParamBinding(&osc.octaveOffset, ranges::osc::OCTAVE_MIN, ranges::osc::OCTAVE_MAX);
  bindings[baseId + 3] =
      makeParamBinding(&osc.scanPos, ranges::osc::SCAN_POS_MIN, ranges::osc::SCAN_POS_MAX);
  bindings[baseId + 4] =
      makeParamBinding(&osc.fmDepth, ranges::osc::FM_DEPTH_MIN, ranges::osc::FM_DEPTH_MAX);
  bindings[baseId + 5] =
      makeParamBinding(&osc.fmRatio, ranges::osc::FM_RATIO_MIN, ranges::osc::FM_RATIO_MAX);
  bindings[baseId + 6] = makeParamBinding(&osc.enabled);
}

// Noise Oscillator Binding
void bindNoise(ParamBinding* bindings, ParamID baseId, noise::Noise& noise) {
  bindings[baseId + 0] = makeParamBinding(&noise.mixLevel,
                                          ranges::osc::noise::MIX_LEVEL_MIN,
                                          ranges::osc::noise::MIX_LEVEL_MAX);
  bindings[baseId + 1] = makeParamBinding(&noise.enabled);
}

// Filter Bindings
void bindSVFilter(ParamBinding* bindings, ParamID baseId, filters::SVFilter& filter) {
  bindings[baseId + 0] = makeParamBinding(&filter.mode,
                                          ranges::filter::FILTER_MODE_MIN,
                                          ranges::filter::FILTER_MODE_MAX);
  bindings[baseId + 1] =
      makeParamBinding(&filter.cutoff, ranges::filter::CUTOFF_MIN, ranges::filter::CUTOFF_MAX);
  bindings[baseId + 2] = makeParamBinding(&filter.resonance,
                                          ranges::filter::RESONANCE_MIN,
                                          ranges::filter::RESONANCE_MAX);
  bindings[baseId + 3] = makeParamBinding(&filter.enabled);
}

void bindLadderFilter(ParamBinding* bindings, ParamID baseId, filters::LadderFilter& filter) {
  bindings[baseId + 0] =
      makeParamBinding(&filter.cutoff, ranges::filter::CUTOFF_MIN, ranges::filter::CUTOFF_MAX);
  bindings[baseId + 1] = makeParamBinding(&filter.resonance,
                                          ranges::filter::RESONANCE_MIN,
                                          ranges::filter::RESONANCE_MAX);
  bindings[baseId + 2] =
      makeParamBinding(&filter.drive, ranges::filter::DRIVE_MIN, ranges::filter::DRIVE_MAX);
  bindings[baseId + 3] = makeParamBinding(&filter.enabled);
}

// Saturator Bindings
void bindSaturator(ParamBinding* bindings, ParamID baseId, saturator::Saturator& saturator) {
  bindings[baseId + 0] = makeParamBinding(&saturator.drive,
                                          ranges::saturator::DRIVE_MIN,
                                          ranges::saturator::DRIVE_MAX);
  bindings[baseId + 1] =
      makeParamBinding(&saturator.mix, ranges::saturator::MIX_MIN, ranges::saturator::MIX_MAX);
  bindings[baseId + 2] = makeParamBinding(&saturator.enabled);
}

// LFO Binding
void bindLFO(ParamBinding* bindings, ParamID baseId, lfo::LFO& lfo) {
  bindings[baseId + 0] = makeParamBinding(&lfo.rate, ranges::lfo::RATE_MIN, ranges::lfo::RATE_MAX);
  bindings[baseId + 1] =
      makeParamBinding(&lfo.amplitude, ranges::lfo::AMPLITUDE_MIN, ranges::lfo::AMPLITUDE_MAX);
  bindings[baseId + 2] = makeParamBinding(&lfo.retrigger);
}

// Envelope Bindings
void bindEnvelope(ParamBinding* bindings, ParamID baseId, envelope::Envelope& env) {
  bindings[baseId + 0] =
      makeParamBinding(&env.attackMs, ranges::env::TIME_MIN, ranges::env::TIME_MAX);
  bindings[baseId + 1] =
      makeParamBinding(&env.decayMs, ranges::env::TIME_MIN, ranges::env::TIME_MAX);
  bindings[baseId + 2] =
      makeParamBinding(&env.sustainLevel, ranges::env::SUSTAIN_MIN, ranges::env::SUSTAIN_MAX);
  bindings[baseId + 3] =
      makeParamBinding(&env.releaseMs, ranges::env::TIME_MIN, ranges::env::TIME_MAX);
  bindings[baseId + 4] =
      makeParamBinding(&env.attackCurveParam, ranges::env::CURVE_MIN, ranges::env::CURVE_MAX);
  bindings[baseId + 5] =
      makeParamBinding(&env.decayCurveParam, ranges::env::CURVE_MIN, ranges::env::CURVE_MAX);
  bindings[baseId + 6] =
      makeParamBinding(&env.releaseCurveParam, ranges::env::CURVE_MIN, ranges::env::CURVE_MAX);
}

// Mono Bindings
void bindMono(ParamBinding* bindings, mono::MonoState& mono) {
  bindings[MONO_ENABLED] = makeParamBinding(&mono.enabled);
  bindings[MONO_LEGATO] = makeParamBinding(&mono.legato);
}

// Portamento Bindings
void bindPorta(ParamBinding* bindings, voices::Portamento& porta) {
  bindings[PORTA_TIME] =
      makeParamBinding(&porta.time, ranges::porta::TIME_MIN, ranges::porta::TIME_MAX);
  bindings[PORTA_LEGATO] = makeParamBinding(&porta.legato);
  bindings[PORTA_ENABLED] = makeParamBinding(&porta.enabled);
}

// Handle updates to params with derived values
void onParamUpdate(VoicePool& pool, ParamID id) {
  switch (id) {
  case OSC1_ENABLED:
  case OSC2_ENABLED:
  case OSC3_ENABLED:
  case OSC4_ENABLED:
  case NOISE_ENABLED: {
    int enabledCount = pool.osc1.enabled + pool.osc2.enabled + pool.osc3.enabled +
                       pool.osc4.enabled + pool.noise.enabled;
    pool.oscMixGain = (enabledCount > 0) ? 1.0f / static_cast<float>(enabledCount) : 1.0f;
    break;
  }

  case AMP_ENV_ATTACK:
  case AMP_ENV_DECAY:
  case AMP_ENV_RELEASE:
    envelope::updateIncrements(pool.ampEnv, pool.sampleRate);
    break;

  case AMP_ENV_ATTACK_CURVE:
  case AMP_ENV_DECAY_CURVE:
  case AMP_ENV_RELEASE_CURVE:
    envelope::updateCurveTables(pool.ampEnv);
    break;

  case FILTER_ENV_ATTACK:
  case FILTER_ENV_DECAY:
  case FILTER_ENV_RELEASE:
    envelope::updateIncrements(pool.filterEnv, pool.sampleRate);
    break;

  case FILTER_ENV_ATTACK_CURVE:
  case FILTER_ENV_DECAY_CURVE:
  case FILTER_ENV_RELEASE_CURVE:
    envelope::updateCurveTables(pool.filterEnv);
    break;

  case MOD_ENV_ATTACK:
  case MOD_ENV_DECAY:
  case MOD_ENV_RELEASE:
    envelope::updateIncrements(pool.modEnv, pool.sampleRate);
    break;

  case MOD_ENV_ATTACK_CURVE:
  case MOD_ENV_DECAY_CURVE:
  case MOD_ENV_RELEASE_CURVE:
    envelope::updateCurveTables(pool.modEnv);
    break;

    // Update Filter Coefficient(s) on param changes
  case SVF_CUTOFF:
  case SVF_RESONANCE:
    filters::updateSVFCoefficients(pool.svf, pool.invSampleRate);
    break;

  case LADDER_CUTOFF:
  case LADDER_RESONANCE:
    filters::updateLadderCoefficient(pool.ladder, pool.invSampleRate);
    break;

  case SATURATOR_DRIVE:
    pool.saturator.invDrive = saturator::calcInvDrive(pool.saturator.drive);
    break;

  case MONO_ENABLED:
    if (pool.mono.enabled) {
      // Kill any active poly voices
      for (uint32_t i = 0; i < pool.activeCount; i++) {
        uint32_t v = pool.activeIndices[i];
        envelope::triggerRelease(pool.ampEnv, v);
        envelope::triggerRelease(pool.filterEnv, v);
        envelope::triggerRelease(pool.modEnv, v);
      }
      pool.mono.voiceIndex = MAX_VOICES;
      pool.mono.stackDepth = 0;
    } else {
      voices::releaseMonoVoice(pool);
    }
    break;

  case PORTA_TIME: {
    pool.porta.coeff = dsp::math::calcPortamento(pool.porta.time, pool.sampleRate);
    break;
  }

    // No special handling needed for other params like
    // Oscillator pitch params - no active voice updates (avoid clicks)
  default:
    break;
  }
}

void initMIDIBindings(ParamRouter& router) {
  for (auto& cc : router.midiBindings)
    cc = ParamID::UNKOWN;

  router.midiBindings[7] = ParamID::MASTER_GAIN;
  router.midiBindings[74] = ParamID::SVF_CUTOFF;
  router.midiBindings[71] = ParamID::SVF_RESONANCE;
}

} // namespace

// ==== APIs ====

void initParamRouter(ParamRouter& router, VoicePool& pool) {
  // IMPORTANT: ParamID enum layouts must match!

  bindOscillator(router.paramBindings, OSC1_MIX_LEVEL, pool.osc1);
  bindOscillator(router.paramBindings, OSC2_MIX_LEVEL, pool.osc2);
  bindOscillator(router.paramBindings, OSC3_MIX_LEVEL, pool.osc3);
  bindOscillator(router.paramBindings, OSC4_MIX_LEVEL, pool.osc4);

  bindNoise(router.paramBindings, NOISE_MIX_LEVEL, pool.noise);

  bindEnvelope(router.paramBindings, AMP_ENV_ATTACK, pool.ampEnv);
  bindEnvelope(router.paramBindings, FILTER_ENV_ATTACK, pool.filterEnv);
  bindEnvelope(router.paramBindings, MOD_ENV_ATTACK, pool.modEnv);

  bindSVFilter(router.paramBindings, SVF_MODE, pool.svf);
  bindLadderFilter(router.paramBindings, LADDER_CUTOFF, pool.ladder);

  bindSaturator(router.paramBindings, SATURATOR_DRIVE, pool.saturator);

  bindLFO(router.paramBindings, LFO1_RATE, pool.lfo1);
  bindLFO(router.paramBindings, LFO2_RATE, pool.lfo2);
  bindLFO(router.paramBindings, LFO3_RATE, pool.lfo3);

  router.paramBindings[PITCH_BEND_RANGE] = makeParamBinding(&pool.pitchBend.range,
                                                            ranges::pitch::BEND_RANGE_MIN,
                                                            ranges::pitch::BEND_RANGE_MAX);

  router.paramBindings[MASTER_GAIN] = makeParamBinding(&pool.masterGain,
                                                       ranges::global::MASTER_GAIN_MIN,
                                                       ranges::global::MASTER_GAIN_MAX);
  bindMono(router.paramBindings, pool.mono);
  bindPorta(router.paramBindings, pool.porta);

  initMIDIBindings(router);
}

void handleMIDICC(ParamRouter& router, VoicePool& pool, uint8_t cc, uint8_t value) {
  if (cc == 1) {
    pool.modWheelValue = value / 127.0f;
    return;
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
              pool.porta.offsets[v] =
                  static_cast<float>(pool.midiNotes[v]) - static_cast<float>(prevNote);
              pool.porta.lastNote = prevNote;
            }

            voices::redirectVoicePitch(pool, v, prevNote, pool.sampleRate);
          } else {
            envelope::triggerRelease(pool.ampEnv, v);
            envelope::triggerRelease(pool.filterEnv, v);
            envelope::triggerRelease(pool.modEnv, v);
          }
        }
        return;
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
    return;
  }

  ParamID paramID = router.midiBindings[cc];
  if (paramID == ParamID::UNKOWN)
    return;

  auto& binding = router.paramBindings[paramID];
  float denorm = binding.min + (value / 127.0f) * (binding.max - binding.min);

  setParamValueByID(router, pool, paramID, denorm);
}

// ========================
// Getters / Setters
// ========================

// Get param value by ID (normalized value)
// Retreives current denormalized and returns normalized value
float getParamValueByID(const ParamRouter& router, ParamID id, ParamValueFormat valueFormat) {
  if (id < 0 || id >= PARAM_COUNT) {
    return 0.0f;
  }

  const ParamBinding& binding = router.paramBindings[id];
  float value = 0.0f;

  // Read the current value based on type
  switch (binding.type) {
  case FLOAT:
    value = *binding.floatPtr;
    break;

  case INT8:
    value = static_cast<float>(*binding.int8Ptr);
    break;

  case BOOL:
    value = *binding.boolPtr ? 1.0f : 0.0f;
    break;

  case FILTER_MODE:
    value = static_cast<float>(static_cast<int>(*binding.svfModePtr));
    break;
  }

  if (valueFormat == ParamValueFormat::DENORMALIZED)
    return value;

  // Noramlize
  float range = binding.max - binding.min;
  if (range > 0.0f)
    return (value - binding.min) / range;

  return 0.0f;
}

// Set param value by ID
// Expects normalized values, denormalizes, and updates value
void setParamValueByID(ParamRouter& router,
                       VoicePool& pool,
                       ParamID id,
                       float value,
                       ParamValueFormat valueFormat) {
  if (id < 0 || id >= PARAM_COUNT) {
    return;
  }

  ParamBinding& binding = router.paramBindings[id];

  // Denormalize
  if (valueFormat == ParamValueFormat::NORMALIZED) {
    if (value < 0.0f)
      value = 0.0f;
    if (value > 1.0f)
      value = 1.0f;

    value = binding.min + (value * (binding.max - binding.min));
  }

  switch (binding.type) {
  case FLOAT:
    *binding.floatPtr = value;
    break;

  case INT8:
    *binding.int8Ptr = static_cast<int8_t>(std::round(value));
    break;

  case BOOL:
    *binding.boolPtr = value >= 0.5f;
    break;

  case FILTER_MODE:
    *binding.svfModePtr = static_cast<filters::SVFMode>(static_cast<int>(std::round(value)));
    break;
  }

  // Handle post-update logic for params with derived values (i.e. Envelopes)
  onParamUpdate(pool, id);
}

// String → ParamID (for parsing 'set' commands)
ParamMapping findParamByName(const char* name) {
  for (const auto& mapping : PARAM_NAMES) {
    if (strcmp(mapping.name, name) == 0) {
      return mapping;
    }
  }
  return PARAM_MAPPING_NOT_FOUND;
}

// ParamID → String (for 'get' commands, help text, errors)
const char* getParamName(ParamID id) {
  for (const auto& mapping : PARAM_NAMES) {
    if (mapping.id == id) {
      return mapping.name;
    }
  }
  return nullptr;
}

// Print parameter options
void printParamList(const char* optionalParam) {
  if (optionalParam != nullptr) {
    printf("Available parameters for: %s\n", optionalParam);
    for (const auto& mapping : PARAM_NAMES) {
      if (strstr(mapping.name, optionalParam) != NULL)
        printf("  %s\n", mapping.name);
    }

  } else {

    printf("Available parameters:\n");
    for (const auto& mapping : PARAM_NAMES) {
      printf("  %s\n", mapping.name);
    }
  }
}

SVFMode getSVFModeType(const char* inputValue) {
  if (strcasecmp(inputValue, "bp") == 0)
    return filters::SVFMode::BP;

  if (strcasecmp(inputValue, "hp") == 0)
    return filters::SVFMode::HP;

  if (strcasecmp(inputValue, "lp") == 0)
    return filters::SVFMode::LP;

  if (strcasecmp(inputValue, "notch") == 0)
    return filters::SVFMode::Notch;

  // default to Sine
  return filters::SVFMode::LP;
};
} // namespace synth::param::bindings

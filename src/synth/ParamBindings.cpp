#include "ParamBindings.h"

#include "Engine.h"
#include "Envelope.h"
#include "synth/Filters.h"
#include "synth/LFO.h"
#include "synth/ParamRanges.h"
#include "synth/Saturator.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace synth::param::bindings {

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
  bindings[baseId + 5] = makeParamBinding(&osc.enabled);
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
}

// Handle updates to params with derived values
void onParamUpdate(Engine& engine, ParamID id) {
  switch (id) {
  // Update Amp Envelope on param changes
  case AMP_ENV_ATTACK:
  case AMP_ENV_DECAY:
  case AMP_ENV_RELEASE:
    envelope::updateIncrements(engine.voicePool.ampEnv, engine.sampleRate);
    break;

  // Update Filter Envelope on param changes
  case FILTER_ENV_ATTACK:
  case FILTER_ENV_DECAY:
  case FILTER_ENV_RELEASE:
    envelope::updateIncrements(engine.voicePool.filterEnv, engine.sampleRate);
    break;

  // Update Filter Coefficient(s) on param changes
  case SVF_CUTOFF:
  case SVF_RESONANCE:
    filters::updateSVFCoefficients(engine.voicePool.svf, engine.voicePool.invSampleRate);
    break;

  case LADDER_CUTOFF:
  case LADDER_RESONANCE:
    filters::updateLadderCoefficient(engine.voicePool.ladder, engine.voicePool.invSampleRate);
    break;

  case SATURATOR_DRIVE:
    engine.voicePool.saturator.invDrive = saturator::calcInvDrive(engine.voicePool.saturator.drive);
    break;

    // No special handling needed for other params like
    // Oscillator pitch params - no active voice updates (avoid clicks)
  default:
    break;
  }
}

} // namespace

// ==== APIs ====
void initParamBindings(Engine& engine) {
  // IMPORTANT: ParamID enum layouts must match!

  bindOscillator(engine.paramBindings, OSC1_MIX_LEVEL, engine.voicePool.osc1);
  bindOscillator(engine.paramBindings, OSC2_MIX_LEVEL, engine.voicePool.osc2);
  bindOscillator(engine.paramBindings, OSC3_MIX_LEVEL, engine.voicePool.osc3);
  bindOscillator(engine.paramBindings, OSC4_MIX_LEVEL, engine.voicePool.osc4);

  bindNoise(engine.paramBindings, NOISE_MIX_LEVEL, engine.voicePool.noise);

  bindEnvelope(engine.paramBindings, AMP_ENV_ATTACK, engine.voicePool.ampEnv);
  bindEnvelope(engine.paramBindings, FILTER_ENV_ATTACK, engine.voicePool.filterEnv);
  bindEnvelope(engine.paramBindings, MOD_ENV_ATTACK, engine.voicePool.modEnv);

  bindSVFilter(engine.paramBindings, SVF_MODE, engine.voicePool.svf);
  bindLadderFilter(engine.paramBindings, LADDER_CUTOFF, engine.voicePool.ladder);

  bindSaturator(engine.paramBindings, SATURATOR_DRIVE, engine.voicePool.saturator);

  bindLFO(engine.paramBindings, LFO1_RATE, engine.voicePool.lfo1);
  bindLFO(engine.paramBindings, LFO2_RATE, engine.voicePool.lfo2);
  bindLFO(engine.paramBindings, LFO3_RATE, engine.voicePool.lfo3);

  engine.paramBindings[MASTER_GAIN] = makeParamBinding(&engine.voicePool.masterGain,
                                                       ranges::global::MASTER_GAIN_MIN,
                                                       ranges::global::MASTER_GAIN_MAX);
}

// ========================
// Getters / Setters
// ========================

// Get param value by ID (normalized value)
// Retreives current denormalized and returns normalized value
float getParamValueByID(const Engine& engine, ParamID id, ParamValueFormat valueFormat) {
  if (id < 0 || id >= PARAM_COUNT) {
    return 0.0f;
  }

  const ParamBinding& binding = engine.paramBindings[id];
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
void setParamValueByID(Engine& engine, ParamID id, float value, ParamValueFormat valueFormat) {
  if (id < 0 || id >= PARAM_COUNT) {
    return;
  }

  ParamBinding& binding = engine.paramBindings[id];

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
  onParamUpdate(engine, id);
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

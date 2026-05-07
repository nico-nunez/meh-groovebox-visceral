#include "app/display/SynthDisplayState.h"

#include "synth/params/ParamUtils.h"

namespace app::display {
namespace {

using synth::param::ParamID;
namespace sp = synth::param;

constexpr ParamID CURATED_SYNTH_PARAM_IDS[DISPLAY_SYNTH_PARAM_CAPACITY] = {
    // Oscillators: 10 params x 4 oscillators = 40.
    sp::OSC1_ENABLED,
    sp::OSC1_BANK_ID,
    sp::OSC1_MIX_LEVEL,
    sp::OSC1_DETUNE,
    sp::OSC1_OCTAVE,
    sp::OSC1_SCAN_POS,
    sp::OSC1_FM_DEPTH,
    sp::OSC1_RATIO,
    sp::OSC1_FIXED,
    sp::OSC1_FIXED_FREQ,
    sp::OSC2_ENABLED,
    sp::OSC2_BANK_ID,
    sp::OSC2_MIX_LEVEL,
    sp::OSC2_DETUNE,
    sp::OSC2_OCTAVE,
    sp::OSC2_SCAN_POS,
    sp::OSC2_FM_DEPTH,
    sp::OSC2_RATIO,
    sp::OSC2_FIXED,
    sp::OSC2_FIXED_FREQ,
    sp::OSC3_ENABLED,
    sp::OSC3_BANK_ID,
    sp::OSC3_MIX_LEVEL,
    sp::OSC3_DETUNE,
    sp::OSC3_OCTAVE,
    sp::OSC3_SCAN_POS,
    sp::OSC3_FM_DEPTH,
    sp::OSC3_RATIO,
    sp::OSC3_FIXED,
    sp::OSC3_FIXED_FREQ,
    sp::OSC4_ENABLED,
    sp::OSC4_BANK_ID,
    sp::OSC4_MIX_LEVEL,
    sp::OSC4_DETUNE,
    sp::OSC4_OCTAVE,
    sp::OSC4_SCAN_POS,
    sp::OSC4_FM_DEPTH,
    sp::OSC4_RATIO,
    sp::OSC4_FIXED,
    sp::OSC4_FIXED_FREQ,

    // Noise/filter: 9.
    sp::NOISE_ENABLED,
    sp::NOISE_MIX_LEVEL,
    sp::SVF_ENABLED,
    sp::SVF_CUTOFF,
    sp::SVF_RESONANCE,
    sp::LADDER_ENABLED,
    sp::LADDER_CUTOFF,
    sp::LADDER_RESONANCE,
    sp::LADDER_DRIVE,

    // Amp envelope: 4.
    sp::AMP_ENV_ATTACK,
    sp::AMP_ENV_DECAY,
    sp::AMP_ENV_SUSTAIN,
    sp::AMP_ENV_RELEASE,

    // Voice/global: 6.
    sp::MONO_ENABLED,
    sp::UNISON_ENABLED,
    sp::UNISON_VOICES,
    sp::UNISON_DETUNE,
    sp::UNISON_SPREAD,
    sp::MASTER_GAIN,

    // FX overview enabled flags: 5.
    sp::FX_DISTORTION_ENABLED,
    sp::FX_CHORUS_ENABLED,
    sp::FX_PHASER_ENABLED,
    sp::FX_DELAY_ENABLED,
    sp::FX_REVERB_ENABLED,
};

static_assert(sizeof(CURATED_SYNTH_PARAM_IDS) / sizeof(CURATED_SYNTH_PARAM_IDS[0]) ==
                  DISPLAY_SYNTH_PARAM_CAPACITY,
              "curated synth display payload must fill all 64 slots");

void appendParam(SynthRuntimeTelemetry& out, const synth::Engine& engine, ParamID id) {
  out.params[out.paramCount].id = id;
  out.params[out.paramCount].value = sp::utils::getParamValueByID(&engine, id);
  out.paramCount += 1;
}

} // namespace

const ParamID* curatedSynthParamIDs() {
  return CURATED_SYNTH_PARAM_IDS;
}

uint8_t curatedSynthParamCount() {
  return DISPLAY_SYNTH_PARAM_CAPACITY;
}

void fillSynthRuntimeTelemetry(SynthRuntimeTelemetry& out, const synth::Engine& engine) {
  out = SynthRuntimeTelemetry{};
  out.noteCount = engine.noteCount;

  for (uint8_t i = 0; i < curatedSynthParamCount(); ++i)
    appendParam(out, engine, CURATED_SYNTH_PARAM_IDS[i]);
}

SynthSummarySnapshot makeSynthSummarySnapshot(const SynthRuntimeTelemetry& telemetry) {
  SynthSummarySnapshot snapshot{};
  snapshot.noteCount = telemetry.noteCount;
  snapshot.paramCount = telemetry.paramCount;

  for (uint8_t i = 0; i < telemetry.paramCount; ++i) {
    const ParamID id = telemetry.params[i].id;
    snapshot.params[i].id = id;
    snapshot.params[i].def = id == sp::PARAM_UNKNOWN ? nullptr : &sp::PARAM_DEFS[id];
    snapshot.params[i].value = telemetry.params[i].value;
  }

  return snapshot;
}

const SynthParamDisplayValue* findSynthParam(const SynthSummarySnapshot& snapshot, sp::ParamID id) {
  for (uint8_t i = 0; i < snapshot.paramCount; ++i) {
    if (snapshot.params[i].id == id)
      return &snapshot.params[i];
  }
  return nullptr;
}

} // namespace app::display

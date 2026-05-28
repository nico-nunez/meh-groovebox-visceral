#include "DocSynthSettingsMetadata.h"

#include <cstring>

namespace app::doc {
namespace {

namespace sp = synth::param;

template <typename T, std::size_t N> constexpr DocMetadataSpan<T> spanOf(const T (&items)[N]) {
  return {items, N};
}

bool equals(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

constexpr AuthoredSynthParamField field(const char* authoredPath,
                                        const char* canonicalParam,
                                        sp::ParamID paramID,
                                        DocLuaValueKind valueKind,
                                        const char* doc = "") {
  return {authoredPath, canonicalParam, paramID, valueKind, doc};
}

#define OSC_AUTHORED_FIELDS(N)                                                                     \
  field("osc" #N ".enabled", "osc" #N ".enabled", sp::OSC##N##_ENABLED, DocLuaValueKind::Boolean), \
      field("osc" #N ".bank", "osc" #N ".bank", sp::OSC##N##_BANK_ID, DocLuaValueKind::String),    \
      field("osc" #N ".mix",                                                                       \
            "osc" #N ".mixLevel",                                                                  \
            sp::OSC##N##_MIX_LEVEL,                                                                \
            DocLuaValueKind::Number),                                                              \
      field("osc" #N ".detune",                                                                    \
            "osc" #N ".detuneAmount",                                                              \
            sp::OSC##N##_DETUNE,                                                                   \
            DocLuaValueKind::Number),                                                              \
      field("osc" #N ".octave",                                                                    \
            "osc" #N ".octaveOffset",                                                              \
            sp::OSC##N##_OCTAVE,                                                                   \
            DocLuaValueKind::Integer),                                                             \
      field("osc" #N ".scan",                                                                      \
            "osc" #N ".scanPos",                                                                   \
            sp::OSC##N##_SCAN_POS,                                                                 \
            DocLuaValueKind::Number),                                                              \
      field("osc" #N ".fmDepth",                                                                   \
            "osc" #N ".fmDepth",                                                                   \
            sp::OSC##N##_FM_DEPTH,                                                                 \
            DocLuaValueKind::Number),                                                              \
      field("osc" #N ".ratio", "osc" #N ".ratio", sp::OSC##N##_RATIO, DocLuaValueKind::Number),    \
      field("osc" #N ".fixed", "osc" #N ".fixed", sp::OSC##N##_FIXED, DocLuaValueKind::Boolean),   \
      field("osc" #N ".fixedFreq",                                                                 \
            "osc" #N ".fixedFreq",                                                                 \
            sp::OSC##N##_FIXED_FREQ,                                                               \
            DocLuaValueKind::Number)

constexpr AuthoredSynthParamField kAuthoredSynthParamFields[] = {
    OSC_AUTHORED_FIELDS(1),
    OSC_AUTHORED_FIELDS(2),
    OSC_AUTHORED_FIELDS(3),
    OSC_AUTHORED_FIELDS(4),

    field("noise.enabled", "noise.enabled", sp::NOISE_ENABLED, DocLuaValueKind::Boolean),
    field("noise.type", "noise.type", sp::NOISE_TYPE, DocLuaValueKind::String),
    field("noise.mix", "noise.mixLevel", sp::NOISE_MIX_LEVEL, DocLuaValueKind::Number),

    field("ampEnv.attack", "ampEnv.attackMs", sp::AMP_ENV_ATTACK, DocLuaValueKind::Number),
    field("ampEnv.decay", "ampEnv.decayMs", sp::AMP_ENV_DECAY, DocLuaValueKind::Number),
    field("ampEnv.sustain", "ampEnv.sustainLevel", sp::AMP_ENV_SUSTAIN, DocLuaValueKind::Number),
    field("ampEnv.release", "ampEnv.releaseMs", sp::AMP_ENV_RELEASE, DocLuaValueKind::Number),

    field("modEnv.attack", "modEnv.attackMs", sp::MOD_ENV_ATTACK, DocLuaValueKind::Number),
    field("modEnv.decay", "modEnv.decayMs", sp::MOD_ENV_DECAY, DocLuaValueKind::Number),
    field("modEnv.sustain", "modEnv.sustainLevel", sp::MOD_ENV_SUSTAIN, DocLuaValueKind::Number),
    field("modEnv.release", "modEnv.releaseMs", sp::MOD_ENV_RELEASE, DocLuaValueKind::Number),

    field("filterEnv.attack", "filterEnv.attackMs", sp::FILTER_ENV_ATTACK, DocLuaValueKind::Number),
    field("filterEnv.decay", "filterEnv.decayMs", sp::FILTER_ENV_DECAY, DocLuaValueKind::Number),
    field("filterEnv.sustain",
          "filterEnv.sustainLevel",
          sp::FILTER_ENV_SUSTAIN,
          DocLuaValueKind::Number),
    field("filterEnv.release",
          "filterEnv.releaseMs",
          sp::FILTER_ENV_RELEASE,
          DocLuaValueKind::Number),

    field("svf.enabled", "svf.enabled", sp::SVF_ENABLED, DocLuaValueKind::Boolean),
    field("svf.mode", "svf.mode", sp::SVF_MODE, DocLuaValueKind::String),
    field("svf.cutoff", "svf.cutoff", sp::SVF_CUTOFF, DocLuaValueKind::Number),
    field("svf.resonance", "svf.resonance", sp::SVF_RESONANCE, DocLuaValueKind::Number),

    field("ladder.enabled", "ladder.enabled", sp::LADDER_ENABLED, DocLuaValueKind::Boolean),
    field("ladder.cutoff", "ladder.cutoff", sp::LADDER_CUTOFF, DocLuaValueKind::Number),
    field("ladder.resonance", "ladder.resonance", sp::LADDER_RESONANCE, DocLuaValueKind::Number),
    field("ladder.drive", "ladder.drive", sp::LADDER_DRIVE, DocLuaValueKind::Number),

    field("mono.enabled", "mono.enabled", sp::MONO_ENABLED, DocLuaValueKind::Boolean),
    field("mono.legato", "mono.legato", sp::MONO_LEGATO, DocLuaValueKind::Boolean),
    field("porta.enabled", "porta.enabled", sp::PORTA_ENABLED, DocLuaValueKind::Boolean),
    field("porta.time", "porta.time", sp::PORTA_TIME, DocLuaValueKind::Number),
    field("porta.legato", "porta.legato", sp::PORTA_LEGATO, DocLuaValueKind::Boolean),
    field("unison.enabled", "unison.enabled", sp::UNISON_ENABLED, DocLuaValueKind::Boolean),
    field("unison.voices", "unison.voices", sp::UNISON_VOICES, DocLuaValueKind::Integer),
    field("unison.detune", "unison.detune", sp::UNISON_DETUNE, DocLuaValueKind::Number),
    field("unison.spread", "unison.spread", sp::UNISON_SPREAD, DocLuaValueKind::Number),
    field("pitchBend.range", "pitchBend.range", sp::PITCH_BEND_RANGE, DocLuaValueKind::Number),
    field("master.gain", "masterGain", sp::MASTER_GAIN, DocLuaValueKind::Number),

    field("fx.distortion.enabled",
          "fx.distortion.enabled",
          sp::FX_DISTORTION_ENABLED,
          DocLuaValueKind::Boolean),
    field("fx.distortion.mix", "fx.distortion.mix", sp::FX_DISTORTION_MIX, DocLuaValueKind::Number),
    field("fx.chorus.enabled",
          "fx.chorus.enabled",
          sp::FX_CHORUS_ENABLED,
          DocLuaValueKind::Boolean),
    field("fx.chorus.mix", "fx.chorus.mix", sp::FX_CHORUS_MIX, DocLuaValueKind::Number),
    field("fx.phaser.enabled",
          "fx.phaser.enabled",
          sp::FX_PHASER_ENABLED,
          DocLuaValueKind::Boolean),
    field("fx.phaser.mix", "fx.phaser.mix", sp::FX_PHASER_MIX, DocLuaValueKind::Number),
    field("fx.delay.enabled", "fx.delay.enabled", sp::FX_DELAY_ENABLED, DocLuaValueKind::Boolean),
    field("fx.delay.mix", "fx.delay.mix", sp::FX_DELAY_MIX, DocLuaValueKind::Number),
    field("fx.reverb.enabled",
          "fx.reverb.enabled",
          sp::FX_REVERB_ENABLED,
          DocLuaValueKind::Boolean),
    field("fx.reverb.mix", "fx.reverb.mix", sp::FX_REVERB_MIX, DocLuaValueKind::Number),
};

#undef OSC_AUTHORED_FIELDS

} // namespace

DocMetadataSpan<AuthoredSynthParamField> authoredSynthParamFields() {
  return spanOf(kAuthoredSynthParamFields);
}

const AuthoredSynthParamField* findAuthoredSynthParamField(const char* authoredPath) {
  for (const auto& field : authoredSynthParamFields()) {
    if (equals(field.authoredPath, authoredPath))
      return &field;
  }
  return nullptr;
}

const AuthoredSynthParamField*
findAuthoredSynthParamFieldByCanonicalParam(const char* canonicalParam) {
  for (const auto& field : authoredSynthParamFields()) {
    if (equals(field.canonicalParam, canonicalParam))
      return &field;
  }
  return nullptr;
}

DocLuaValueKind authoredSynthValueKindForParamType(synth::param::ParamType type) {
  using Type = synth::param::ParamType;
  switch (type) {
  case Type::Bool:
    return DocLuaValueKind::Boolean;
  case Type::Int8:
    return DocLuaValueKind::Integer;
  case Type::OscBankID:
  case Type::NoiseType:
  case Type::FilterMode:
  case Type::DistortionType:
  case Type::PhaseMode:
  case Type::Subdivision:
    return DocLuaValueKind::String;
  case Type::Float:
    return DocLuaValueKind::Number;
  }
  return DocLuaValueKind::Any;
}

} // namespace app::doc

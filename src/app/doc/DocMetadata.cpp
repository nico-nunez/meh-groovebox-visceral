#include "DocMetadata.h"

#include "app/Constants.h"

#include <cstring>

namespace app::doc {
namespace {

template <typename T, std::size_t N> constexpr DocMetadataSpan<T> spanOf(const T (&items)[N]) {
  return {items, N};
}

bool equals(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

constexpr DocIntegerBounds intBounds(int min, int max) {
  return {min, max, true, true};
}

constexpr DocNumberBounds finiteMin(double min) {
  return {min, true, true};
}

constexpr DocFieldMetadata kMixerTrackSettingsFields[] = {
    {"gain", DocLuaValueKind::Number, false, {}, {}, "", "Track output gain. Range 0.0..1.0."},
    {"pan", DocLuaValueKind::Number, false, {}, {}, "", "Track pan position. Range -1.0..1.0."},
    {"mute", DocLuaValueKind::Boolean, false, {}, {}, "", "Track mute state."},
};

constexpr DocFunctionArgMetadata kMixerArgs[] = {
    {"trackNumber",
     DocLuaValueKind::Integer,
     "",
     intBounds(1, app::MAX_TRACKS),
     "One-based track number."},
    {"settings", DocLuaValueKind::Table, doctype::MixerSettings, {}, "MixerSettings table."},
};

constexpr DocFieldMetadata kSynthOscFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"bank", DocLuaValueKind::String, false, {}, {}, doctype::SynthOscBank},
    {"mixLevel", DocLuaValueKind::Number, false},
    {"detuneAmount", DocLuaValueKind::Number, false},
    {"octaveOffset", DocLuaValueKind::Integer, false},
    {"scanPos", DocLuaValueKind::Number, false},
    {"fmDepth", DocLuaValueKind::Number, false},
    {"randomRange", DocLuaValueKind::Number, false},
    {"resetPhase", DocLuaValueKind::Number, false},
    {"phaseMode", DocLuaValueKind::String, false, {}, {}, doctype::SynthOscPhaseMode},
    {"ratio", DocLuaValueKind::Number, false},
    {"fixed", DocLuaValueKind::Boolean, false},
    {"fixedFreq", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthLfoFields[] = {
    {"bank", DocLuaValueKind::String, false, {}, {}, doctype::SynthOscBank},
    {"rate", DocLuaValueKind::Number, false},
    {"amplitude", DocLuaValueKind::Number, false},
    {"retrigger", DocLuaValueKind::Boolean, false},
    {"delay", DocLuaValueKind::Number, false},
    {"attack", DocLuaValueKind::Number, false},
    {"subdivision", DocLuaValueKind::String, false, {}, {}, doctype::TempoSubdivision},
    {"tempoSync", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthNoiseFields[] = {
    {"type", DocLuaValueKind::String, false, {}, {}, doctype::SynthNoiseType},
    {"enabled", DocLuaValueKind::Boolean, false},
    {"mixLevel", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthEnvelopeFields[] = {
    {"attack", DocLuaValueKind::Number, false},
    {"attackCurve", DocLuaValueKind::Number, false},
    {"decay", DocLuaValueKind::Number, false},
    {"decayCurve", DocLuaValueKind::Number, false},
    {"sustain", DocLuaValueKind::Number, false},
    {"release", DocLuaValueKind::Number, false},
    {"releaseCurve", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthSVFFields[] = {
    {"mode", DocLuaValueKind::String, false, {}, {}, doctype::SynthSVFMode},
    {"cutoff", DocLuaValueKind::Number, false},
    {"resonance", DocLuaValueKind::Number, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthLadderFields[] = {
    {"cutoff", DocLuaValueKind::Number, false},
    {"resonance", DocLuaValueKind::Number, false},
    {"drive", DocLuaValueKind::Number, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthSaturatorFields[] = {
    {"drive", DocLuaValueKind::Number, false},
    {"mix", DocLuaValueKind::Number, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthPitchBendFields[] = {
    {"range", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthMonoFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"legato", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthPortaFields[] = {
    {"time", DocLuaValueKind::Number, false},
    {"legato", DocLuaValueKind::Boolean, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthUnisonFields[] = {
    {"voices", DocLuaValueKind::Integer, false},
    {"detune", DocLuaValueKind::Number, false},
    {"spread", DocLuaValueKind::Number, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthModRouteEntryFields[] = {
    {"src",
     DocLuaValueKind::String,
     true,
     {},
     {},
     doctype::SynthModSource,
     "Mod source name (e.g. \"lfo1\", \"velocity\", \"modenv\")."},
    {"dest",
     DocLuaValueKind::String,
     true,
     {},
     {},
     doctype::SynthModDestination,
     "Mod destination path (e.g. \"osc1.pitch\", \"svf.cutoff\")."},
    {"amount",
     DocLuaValueKind::Number,
     true,
     {},
     {},
     "",
     "Modulation amount. Signed; range depends on destination."},
};

constexpr DocFieldMetadata kSynthFMRouteEntryFields[] = {
    {"carrier",
     DocLuaValueKind::String,
     true,
     {},
     {},
     doctype::SynthFMSource,
     "Carrier oscillator name (\"osc1\"–\"osc4\")."},
    {"mod",
     DocLuaValueKind::String,
     true,
     {},
     {},
     doctype::SynthFMSource,
     "Modulator oscillator name (\"osc1\"–\"osc4\"). Must differ from carrier."},
    {"depth", DocLuaValueKind::Number, true, {}, {}, "", "FM depth. Range [0.0, 10.0]."},
};

constexpr DocFieldMetadata kSynthMasterFields[] = {
    {"gain", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthFXDistortionFields[] = {
    {"drive", DocLuaValueKind::Number, false},
    {"mix", DocLuaValueKind::Number, false},
    {"type", DocLuaValueKind::String, false, {}, {}, doctype::SynthFXDistortionType},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthFXChorusFields[] = {
    {"rate", DocLuaValueKind::Number, false},
    {"depth", DocLuaValueKind::Number, false},
    {"mix", DocLuaValueKind::Number, false},
    {"feedback", DocLuaValueKind::Number, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthFXPhaserFields[] = {
    {"stages", DocLuaValueKind::Integer, false},
    {"rate", DocLuaValueKind::Number, false},
    {"depth", DocLuaValueKind::Number, false},
    {"feedback", DocLuaValueKind::Number, false},
    {"mix", DocLuaValueKind::Number, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthFXDelayFields[] = {
    {"time", DocLuaValueKind::Number, false},
    {"subdivision", DocLuaValueKind::String, false, {}, {}, doctype::TempoSubdivision},
    {"tempoSync", DocLuaValueKind::Boolean, false},
    {"feedback", DocLuaValueKind::Number, false},
    {"damping", DocLuaValueKind::Number, false},
    {"hpDamping", DocLuaValueKind::Number, false},
    {"mix", DocLuaValueKind::Number, false},
    {"pingPong", DocLuaValueKind::Boolean, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthFXReverbFields[] = {
    {"preDelay", DocLuaValueKind::Number, false},
    {"decay", DocLuaValueKind::Number, false},
    {"damping", DocLuaValueKind::Number, false},
    {"lowDamping", DocLuaValueKind::Number, false},
    {"diffusion", DocLuaValueKind::Number, false},
    {"bandwidth", DocLuaValueKind::Number, false},
    {"modRate", DocLuaValueKind::Number, false},
    {"modDepth", DocLuaValueKind::Number, false},
    {"mix", DocLuaValueKind::Number, false},
    {"enabled", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthFXFields[] = {
    {"distortion", DocLuaValueKind::Table, false, {}, {}, doctype::SynthFXDistortionSettings},
    {"chorus", DocLuaValueKind::Table, false, {}, {}, doctype::SynthFXChorusSettings},
    {"phaser", DocLuaValueKind::Table, false, {}, {}, doctype::SynthFXPhaserSettings},
    {"delay", DocLuaValueKind::Table, false, {}, {}, doctype::SynthFXDelaySettings},
    {"reverb", DocLuaValueKind::Table, false, {}, {}, doctype::SynthFXReverbSettings},
};

constexpr DocFieldMetadata kSynthSettingsFields[] = {
    {"osc1", DocLuaValueKind::Table, false, {}, {}, doctype::SynthOscSettings},
    {"osc2", DocLuaValueKind::Table, false, {}, {}, doctype::SynthOscSettings},
    {"osc3", DocLuaValueKind::Table, false, {}, {}, doctype::SynthOscSettings},
    {"osc4", DocLuaValueKind::Table, false, {}, {}, doctype::SynthOscSettings},
    {"lfo1", DocLuaValueKind::Table, false, {}, {}, doctype::SynthLfoSettings},
    {"lfo2", DocLuaValueKind::Table, false, {}, {}, doctype::SynthLfoSettings},
    {"lfo3", DocLuaValueKind::Table, false, {}, {}, doctype::SynthLfoSettings},
    {"noise", DocLuaValueKind::Table, false, {}, {}, doctype::SynthNoiseSettings},
    {"ampEnv", DocLuaValueKind::Table, false, {}, {}, doctype::SynthAmpEnvSettings},
    {"modEnv", DocLuaValueKind::Table, false, {}, {}, doctype::SynthModEnvSettings},
    {"filterEnv", DocLuaValueKind::Table, false, {}, {}, doctype::SynthFilterEnvSettings},
    {"svf", DocLuaValueKind::Table, false, {}, {}, doctype::SynthSVFSettings},
    {"ladder", DocLuaValueKind::Table, false, {}, {}, doctype::SynthLadderSettings},
    {"saturator", DocLuaValueKind::Table, false, {}, {}, doctype::SynthSaturatorSettings},
    {"mono", DocLuaValueKind::Table, false, {}, {}, doctype::SynthMonoSettings},
    {"porta", DocLuaValueKind::Table, false, {}, {}, doctype::SynthPortaSettings},
    {"unison", DocLuaValueKind::Table, false, {}, {}, doctype::SynthUnisonSettings},
    {"pitchBend", DocLuaValueKind::Table, false, {}, {}, doctype::SynthPitchBendSettings},
    {"modMatrix",
     DocLuaValueKind::Table,
     false,
     {},
     {},
     doctype::SynthModRouteEntryTable,
     "Mod matrix routes. Array of { src, dest, amount } tables. Full replace when present."},
    {"fmRoutes",
     DocLuaValueKind::Table,
     false,
     {},
     {},
     doctype::SynthFMRouteEntryTable,
     "FM routes. Array of { carrier, mod, depth } tables. Full replace when present."},
    {"signalChain",
     DocLuaValueKind::Table,
     false,
     {},
     {},
     doctype::SynthSignalChainTable,
     "Signal chain order. Array of processor name strings (\"svf\", \"ladder\", \"saturator\"). "
     "Full replace when present."},
    {"master", DocLuaValueKind::Table, false, {}, {}, doctype::SynthMasterSettings},
    {"fx", DocLuaValueKind::Table, false, {}, {}, doctype::SynthFXSettings},
};

constexpr DocFieldMetadata kTrackSettingsFields[] = {
    {
        "patterns",
        DocLuaValueKind::Table,
        false,
        {},
        {},
        doctype::PatternSlots,
        "Pattern slots keyed one-based in authored Lua.",
    },
    {
        "activeSlot",
        DocLuaValueKind::Integer,
        false,
        intBounds(1, app::sequencer::PATTERNS_PER_LANE),
        {},
        "",
        "One-based active pattern slot.",
    },
    {
        "synth",
        DocLuaValueKind::Table,
        false,
        {},
        {},
        doctype::SynthSettings,
        "Patch-style synth settings for this track.",
    },
    {
        "mixer",
        DocLuaValueKind::Table,
        false,
        {},
        {},
        doctype::MixerSettings,
        "Patch-style track mixer settings.",
    },

};

constexpr DocFieldMetadata kPatternFields[] = {
    {
        "numSteps",
        DocLuaValueKind::Integer,
        true,
        intBounds(1, app::sequencer::MAX_PATTERN_STEPS),
        {},
        "",
        "Pattern step count.",
    },
    {
        "stepsPerBeat",
        DocLuaValueKind::Integer,
        true,
        intBounds(1, app::sequencer::MAX_STEPS_PER_BEAT),
        {},
        "",
        "Timing resolution for the pattern.",
    },
    {
        "steps",
        DocLuaValueKind::Table,
        true,
        {},
        {},
        doctype::StepTable,
        "Array of authored step tables.",
    },
};

constexpr DocFieldMetadata kStepNoteFields[] = {
    {"noteOn", DocLuaValueKind::Boolean, false},
    {"tie", DocLuaValueKind::Boolean, false},
    {"note", DocLuaValueKind::Integer, false, intBounds(0, 127)},
    {"velocity", DocLuaValueKind::Integer, false, intBounds(0, 127)},
    {"gate", DocLuaValueKind::Number, false, {}, finiteMin(0.0)},
};

constexpr DocFieldMetadata kStepLockFields[] = {
    {
        "param",
        DocLuaValueKind::String,
        true,
        {},
        {},
        "", // TODO:  figure out x-macro/meta way.
        "Synth parameter name from synth::param::PARAM_DEFS.",
    },
    {"value", DocLuaValueKind::Number, true},
};

constexpr DocFieldMetadata kStepFields[] = {
    {"active", DocLuaValueKind::Boolean, false},
    {
        "notes",
        DocLuaValueKind::Table,
        false,
        {},
        {},
        doctype::StepNoteTable,
        "Array of up to MAX_NOTES_PER_STEP notes. Required for all note data, including one-note "
        "steps.",
    },
    {
        "locks",
        DocLuaValueKind::Table,
        false,
        {},
        {},
        doctype::StepLockTable,
        "At most MAX_LOCKS_PER_STEP entries.",
    },
};

constexpr DocTypeMetadata kAuthoredTypes[] = {
    // ==== Aliases ====
    {
        doctype::TempoSubdivision,
        DocMetadataKind::Alias,
        {},
        docalias::TempoSubdivision,
        "Tempo subdivision",
    },
    {
        doctype::PatternSlots,
        DocMetadataKind::Alias,
        {},
        docalias::PatternSlots,
        "One-based pattern slot table, 1..PATTERNS_PER_LANE.",
    },
    {
        doctype::SynthOscBank,
        DocMetadataKind::Alias,
        {},
        docalias::SynthOscBank,
        "Oscillator bank type",
    },
    {
        doctype::SynthOscPhaseMode,
        DocMetadataKind::Alias,
        {},
        docalias::SynthOscPhaseMode,
        "Oscillator phase mode",
    },
    {
        doctype::SynthNoiseType,
        DocMetadataKind::Alias,
        {},
        docalias::SynthNoiseType,
        "Noise type",
    },
    {
        doctype::SynthSVFMode,
        DocMetadataKind::Alias,
        {},
        docalias::SynthSVFMode,
        "SVF mode type",
    },
    {
        doctype::SynthModSource,
        DocMetadataKind::Alias,
        {},
        docalias::SynthModSource,
        "Mod matrix route source.",
    },
    {
        doctype::SynthModDestination,
        DocMetadataKind::Alias,
        {},
        docalias::SynthModDestination,
        "Mod matrix route destination.",
    },
    {
        doctype::SynthFMSource,
        DocMetadataKind::Alias,
        {},
        docalias::SynthFMSource,
        "FM route source.",
    },
    {
        doctype::SynthFXDistortionType,
        DocMetadataKind::Alias,
        {},
        docalias::SynthFXDistortionType,
        "FX distortion type",
    },
    // ==== Classes ====
    {
        doctype::Pattern,
        DocMetadataKind::Struct,
        spanOf(kPatternFields),
        {},
        "Sequencer lane pattern.",
    },
    {
        doctype::StepNote,
        DocMetadataKind::Struct,
        spanOf(kStepNoteFields),
        {},
        "One note event inside a sequencer step.",
    },
    {
        doctype::StepLock,
        DocMetadataKind::Struct,
        spanOf(kStepLockFields),
        {},
        "Per-step synth parameter lock.",
    },
    {
        doctype::Step,
        DocMetadataKind::Struct,
        spanOf(kStepFields),
        {},
        "Sequencer step event.",
    },
    {
        doctype::SynthSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthSettingsFields),
        {},
        "Patch-style authored synth settings.",
    },
    {
        doctype::SynthOscSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthOscFields),
        {},
        "Oscillator settings.",
    },
    {
        doctype::SynthLfoSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthLfoFields),
        {},
        "LFO settings.",
    },
    {
        doctype::SynthNoiseSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthNoiseFields),
        {},
        "Noise settings.",
    },
    {
        doctype::SynthAmpEnvSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthEnvelopeFields),
        {},
        "Amp envelope settings.",
    },
    {
        doctype::SynthModEnvSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthEnvelopeFields),
        {},
        "Mod envelope settings.",
    },
    {
        doctype::SynthFilterEnvSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthEnvelopeFields),
        {},
        "Filter envelope settings.",
    },
    {
        doctype::SynthSVFSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthSVFFields),
        {},
        "SVF settings.",
    },
    {
        doctype::SynthLadderSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthLadderFields),
        {},
        "Ladder filter settings.",
    },
    {
        doctype::SynthSaturatorSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthSaturatorFields),
        {},
        "Saturator settings.",
    },
    {
        doctype::SynthMonoSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthMonoFields),
        {},
        "Mono settings.",
    },
    {
        doctype::SynthPortaSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthPortaFields),
        {},
        "Portamento settings.",
    },
    {
        doctype::SynthUnisonSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthUnisonFields),
        {},
        "Unison settings.",
    },
    {
        doctype::SynthPitchBendSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthPitchBendFields),
        {},
        "Pitch bend settings.",
    },
    {
        doctype::SynthModRouteEntry,
        DocMetadataKind::Struct,
        spanOf(kSynthModRouteEntryFields),
        {},
        "One mod matrix route. src and dest are string tokens; amount is signed float.",
    },
    {
        doctype::SynthFMRouteEntry,
        DocMetadataKind::Struct,
        spanOf(kSynthFMRouteEntryFields),
        {},
        "One FM route. carrier and mod are osc name strings; depth is [0.0, 10.0].",
    },
    {
        doctype::SynthMasterSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthMasterFields),
        {},
        "Master synth settings.",
    },
    {
        doctype::SynthFXSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthFXFields),
        {},
        "FX settings.",
    },
    {
        doctype::SynthFXDistortionSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthFXDistortionFields),
        {},
        "FX distortion settings.",
    },
    {
        doctype::SynthFXChorusSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthFXChorusFields),
        {},
        "FX chorus settings.",
    },
    {
        doctype::SynthFXPhaserSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthFXPhaserFields),
        {},
        "FX phaser settings.",
    },
    {
        doctype::SynthFXDelaySettings,
        DocMetadataKind::Struct,
        spanOf(kSynthFXDelayFields),
        {},
        "FX delay settings.",
    },
    {
        doctype::SynthFXReverbSettings,
        DocMetadataKind::Struct,
        spanOf(kSynthFXReverbFields),
        {},
        "FX reverb settings.",
    },
    {
        doctype::MixerSettings,
        DocMetadataKind::Struct,
        spanOf(kMixerTrackSettingsFields),
        {},
        "Patch-style authored track mixer settings.",
    },
    {
        doctype::MixerTrackSettings,
        DocMetadataKind::Struct,
        spanOf(kMixerTrackSettingsFields),
        {},
        "Track-scoped mixer settings (gain, pan, mute).",
    },
    {
        doctype::TrackSettings,
        DocMetadataKind::Struct,
        spanOf(kTrackSettingsFields),
        {},
        "Implemented sequencer track settings.",
    },
};

constexpr DocFunctionArgMetadata kSynthArgs[] = {
    {"trackNumber",
     DocLuaValueKind::Integer,
     "",
     intBounds(1, app::MAX_TRACKS),
     "One-based track number."},
    {"settings", DocLuaValueKind::Table, doctype::SynthSettings, {}, "SynthSettings table."},
};

constexpr DocFunctionArgMetadata kTrackArgs[] = {
    {
        "trackNumber",
        DocLuaValueKind::Integer,
        "",
        intBounds(1, app::MAX_TRACKS),
        "One-based track number.",
    },
    {
        "settings",
        DocLuaValueKind::Table,
        doctype::TrackSettings,
        {},
        "TrackSettings table.",
    },
};

constexpr DocFunctionMetadata kAuthoredFunctions[] = {
    {
        docglobal::Mixer,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kMixerArgs),
        "",
        "Capture patch-style authored mixer state for one track.",
    },
    {
        docglobal::Track,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kTrackArgs),
        "",
        "Capture authored sequencer state for one track.",
    },
    {
        docglobal::Synth,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kSynthArgs),
        "",
        "Capture patch-style authored synth state for one track.",
    },
};

constexpr const char* kAuthoredConstructors[] = {
    docctor::TrackSettings,
    docctor::SynthSettings,
    docctor::MixerSettings,
};

constexpr DocDiagnosticMetadata kDiagnostics[] = {
    {
        docdiag::MixerPlanningFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::Planner,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "mixer:*",
        "Mixer target-state construction failed before runtime admission.",
    },
    {
        docdiag::MixerTrackInvalidIndex,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "mixer",
        "Mixer track index is not an integer or is out of range.",
    },
    {
        docdiag::MixerSettingsInvalidShape,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "mixer:N",
        "MixerSettings is present but is not a table.",
    },
    {
        docdiag::MixerParamUnknown,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "mixer:N.param",
        "Mixer field key is unknown.",
    },
    {
        docdiag::MixerParamTypeMismatch,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "mixer:N.param",
        "Mixer parameter value has the wrong Lua type.",
    },
    {
        docdiag::MixerParamOutOfRange,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "mixer:N.param",
        "Mixer parameter value is outside its allowed range.",
    },
    {
        docdiag::MixerParamDuplicateWrite,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "mixer:N.param",
        "Mixer parameter is written more than once with conflicting values.",
    },
    {
        docdiag::MixerAdmissionFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::GrooveboxAdmission,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "mixer:N.param",
        "Mixer param could not be admitted (control queue full).",
    },
    {
        docdiag::SequencerPlanningFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::Planner,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:*.patterns",
        "Sequencer target-state construction failed before runtime admission.",
    },
    {
        docdiag::SequencerTrackInvalidIndex,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track",
        "Track index is not an integer or is out of range.",
    },
    {
        docdiag::SequencerTrackInvalidSettings,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track",
        "Track settings argument is not a table.",
    },
    {
        docdiag::SequencerPatternsInvalidShape,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.patterns",
        "patterns is present but is not a table.",
    },
    {
        docdiag::SequencerPatternSlotInvalidKey,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.patterns",
        "Pattern slot key is not an integer.",
    },
    {
        docdiag::SequencerPatternSlotOutOfRange,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.patterns",
        "Pattern slot key is outside 1..PATTERNS_PER_LANE.",
    },
    {
        docdiag::SequencerPatternInvalidShape,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.patterns[S]",
        "Pattern or step table cannot be parsed.",
    },
    {
        docdiag::SequencerActiveSlotInvalidType,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.activeSlot",
        "activeSlot is present but is not an integer.",
    },
    {
        docdiag::SequencerActiveSlotOutOfRange,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.activeSlot",
        "activeSlot is outside 1..PATTERNS_PER_LANE.",
    },
    {
        docdiag::SequencerActiveSlotMissingPatterns,
        DiagnosticSeverity::Error,
        DiagnosticSource::Normalizer,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.activeSlot",
        "activeSlot requires populated patterns.",
    },
    {
        docdiag::SequencerActiveSlotEmptySlot,
        DiagnosticSeverity::Error,
        DiagnosticSource::Normalizer,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.activeSlot",
        "activeSlot points to an empty pattern slot.",
    },
    {
        docdiag::SequencerAdmissionFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::GrooveboxAdmission,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "track:N.patterns or track:N.activeSlot",
        "Sequencer rejected planned authored state.",
    },
    {
        docdiag::SynthPlanningFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::Planner,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:*",
        "Synth target-state construction failed before runtime admission.",
    },
    {
        docdiag::SynthTrackInvalidIndex,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth",
        "Synth track index is not an integer or is out of range.",
    },
    {
        docdiag::SynthSettingsInvalidShape,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N",
        "SynthSettings is present but is not a table.",
    },
    {
        docdiag::SynthParamUnknown,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.param",
        "Synth parameter path is unknown or deferred.",
    },
    {
        docdiag::SynthParamTypeMismatch,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.param",
        "Synth parameter value has the wrong Lua type.",
    },
    {
        docdiag::SynthParamEnumUnknown,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.param",
        "Synth enum token is unknown.",
    },
    {
        docdiag::SynthParamOutOfRange,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.param",
        "Synth parameter value is outside its allowed range.",
    },
    {
        docdiag::SynthParamDuplicateWrite,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.param",
        "Synth parameter is written more than once with conflicting values.",
    },
    {
        docdiag::SynthAdmissionFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::GrooveboxAdmission,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.param",
        "Synth param patch could not be admitted to the target track.",
    },
    {
        docdiag::SynthModRouteInvalidShape,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.modMatrix",
        "modMatrix is not a table, or a route entry is not a table.",
    },
    {
        docdiag::SynthModRouteInvalidSrc,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.modMatrix[R].src",
        "ModMatrix route src is not a string, or the token is unknown.",
    },
    {
        docdiag::SynthModRouteInvalidDest,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.modMatrix[R].dest",
        "ModMatrix route dest is not a string, or the token is unknown.",
    },
    {
        docdiag::SynthModRouteCapacityExceeded,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.modMatrix",
        "Route count exceeds MAX_MOD_ROUTES.",
    },
    {
        docdiag::SynthFMRouteInvalidShape,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.fmRoutes",
        "fm is not a table, a route entry is not a table, or a depth field is missing.",
    },
    {
        docdiag::SynthFMRouteInvalidCarrier,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.fmRoutes[R].carrier",
        "FM route carrier is not a string, or the token is unknown.",
    },
    {
        docdiag::SynthFMRouteInvalidMod,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.fmRoutes[R].mod",
        "FM route mod is not a string, or the token is unknown.",
    },
    {
        docdiag::SynthFMRouteSelfMod,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.fmRoutes[R]",
        "FM route carrier and mod name the same oscillator.",
    },
    {
        docdiag::SynthFMRouteOutOfRange,
        DiagnosticSeverity::Error,
        DiagnosticSource::Planner,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Reserved,
        "synth:N.fmRoutes[R].depth",
        "FM route depth is outside [0.0, 10.0]. Reserved: planner currently uses "
        "SynthPlanningFailed.",
    },
    {
        docdiag::SynthFMRouteDuplicate,
        DiagnosticSeverity::Error,
        DiagnosticSource::Planner,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Reserved,
        "synth:N.fmRoutes",
        "Duplicate (carrier, mod) pair. Reserved: planner currently uses SynthPlanningFailed.",
    },
    {
        docdiag::SynthFMRouteCapacityExceeded,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.fmRoutes",
        "FM route count exceeds per-carrier or total capacity.",
    },
    {
        docdiag::SynthSignalChainInvalidShape,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.signalChain",
        "signalChain is not a table, or an entry is not a string.",
    },
    {
        docdiag::SynthSignalChainUnknownProcessor,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.signalChain[S]",
        "Signal chain processor name is unknown.",
    },
    {
        docdiag::SynthSignalChainDuplicate,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.signalChain[S]",
        "Same processor appears more than once in the signal chain.",
    },
    {
        docdiag::SynthSignalChainCapacityExceeded,
        DiagnosticSeverity::Error,
        DiagnosticSource::Validator,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "synth:N.signalChain",
        "Signal chain length exceeds MAX_CHAIN_SLOTS.",
    },
    {
        docdiag::DocumentLuaStateFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::Parser,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "document",
        "Failed to create the restricted authored-document Lua state.",
    },
    {
        docdiag::DocumentLuaEvalFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::Parser,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "document",
        "Authored document Lua evaluation failed.",
    },
    {
        docdiag::DocumentFileReadFailed,
        DiagnosticSeverity::Error,
        DiagnosticSource::Parser,
        DocMetadataSurface::RuntimeFileApply,
        DocMetadataStatus::Implemented,
        "path",
        "Runtime file apply could not read the requested file.",
    },
    {
        docdiag::InternalPlannerError,
        DiagnosticSeverity::Error,
        DiagnosticSource::Planner,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        "document",
        "Internal document planner error.",
    },
};

} // namespace

DocMetadataSpan<DocFunctionMetadata> authoredDocumentFunctions() {
  return spanOf(kAuthoredFunctions);
}

DocMetadataSpan<DocTypeMetadata> authoredDocumentTypes() {
  return spanOf(kAuthoredTypes);
}

DocMetadataSpan<const char*> authoredDocumentConstructors() {
  return spanOf(kAuthoredConstructors);
}

DocMetadataSpan<DocDiagnosticMetadata> documentDiagnosticCatalog() {
  return spanOf(kDiagnostics);
}

const DocFunctionMetadata* findAuthoredDocumentFunction(const char* name) {
  for (const auto& function : authoredDocumentFunctions()) {
    if (equals(function.name, name))
      return &function;
  }
  return nullptr;
}

const DocTypeMetadata* findAuthoredDocumentType(const char* name) {
  for (const auto& type : authoredDocumentTypes()) {
    if (equals(type.name, name))
      return &type;
  }
  return nullptr;
}

const DocDiagnosticMetadata* findDocumentDiagnostic(const char* code) {
  for (const auto& diagnostic : documentDiagnosticCatalog()) {
    if (equals(diagnostic.code, code))
      return &diagnostic;
  }
  return nullptr;
}

} // namespace app::doc

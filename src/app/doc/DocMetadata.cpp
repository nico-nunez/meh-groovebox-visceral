#include "app/doc/DocMetadata.h"

#include "app/Constants.h"
#include "app/Sequencer.h"

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
    {"gain",
     DocLuaValueKind::Number,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     "",
     "Track output gain. Range 0.0..1.0."},
    {"pan",
     DocLuaValueKind::Number,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     "",
     "Track pan position. Range -1.0..1.0."},
    {"mute",
     DocLuaValueKind::Boolean,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     "",
     "Track mute state."},
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
    {"bank", DocLuaValueKind::String, false},
    {"mix", DocLuaValueKind::Number, false},
    {"detune", DocLuaValueKind::Number, false},
    {"octave", DocLuaValueKind::Integer, false},
    {"scan", DocLuaValueKind::Number, false},
    {"fmDepth", DocLuaValueKind::Number, false},
    {"ratio", DocLuaValueKind::Number, false},
    {"fixed", DocLuaValueKind::Boolean, false},
    {"fixedFreq", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthNoiseFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"type", DocLuaValueKind::String, false},
    {"mix", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthAmpEnvFields[] = {
    {"attack", DocLuaValueKind::Number, false},
    {"decay", DocLuaValueKind::Number, false},
    {"sustain", DocLuaValueKind::Number, false},
    {"release", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthSVFFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"mode", DocLuaValueKind::String, false},
    {"cutoff", DocLuaValueKind::Number, false},
    {"resonance", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthLadderFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"cutoff", DocLuaValueKind::Number, false},
    {"resonance", DocLuaValueKind::Number, false},
    {"drive", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthMonoFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"legato", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthPortaFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"time", DocLuaValueKind::Number, false},
    {"legato", DocLuaValueKind::Boolean, false},
};

constexpr DocFieldMetadata kSynthUnisonFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"voices", DocLuaValueKind::Integer, false},
    {"detune", DocLuaValueKind::Number, false},
    {"spread", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthPitchBendFields[] = {
    {"range", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthMasterFields[] = {
    {"gain", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthFXUnitFields[] = {
    {"enabled", DocLuaValueKind::Boolean, false},
    {"mix", DocLuaValueKind::Number, false},
};

constexpr DocFieldMetadata kSynthFXFields[] = {
    {"distortion",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthFXUnitSettings},
    {"chorus",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthFXUnitSettings},
    {"phaser",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthFXUnitSettings},
    {"delay",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthFXUnitSettings},
    {"reverb",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthFXUnitSettings},
};

constexpr DocFieldMetadata kSynthSettingsFields[] = {
    {"osc1",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthOscSettings},
    {"osc2",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthOscSettings},
    {"osc3",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthOscSettings},
    {"osc4",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthOscSettings},
    {"noise",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthNoiseSettings},
    {"ampEnv",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthAmpEnvSettings},
    {"svf",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthSVFSettings},
    {"ladder",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthLadderSettings},
    {"mono",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthMonoSettings},
    {"porta",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthPortaSettings},
    {"unison",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthUnisonSettings},
    {"pitchBend",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthPitchBendSettings},
    {"master",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthMasterSettings},
    {"fx",
     DocLuaValueKind::Table,
     false,
     DocMetadataStatus::Implemented,
     {},
     {},
     doctype::SynthFXSettings},
};

constexpr DocFieldMetadata kTrackSettingsFields[] = {
    {
        "patterns",
        DocLuaValueKind::Table,
        false,
        DocMetadataStatus::Implemented,
        {},
        {},
        doctype::PatternSlots,
        "Pattern slots keyed one-based in authored Lua.",
    },
    {
        "activeSlot",
        DocLuaValueKind::Integer,
        false,
        DocMetadataStatus::Implemented,
        intBounds(1, app::sequencer::PATTERNS_PER_LANE),
        {},
        "",
        "One-based active pattern slot.",
    },
    {
        "synth",
        DocLuaValueKind::Table,
        false,
        DocMetadataStatus::Implemented,
        {},
        {},
        doctype::SynthSettings,
        "Patch-style synth settings for this track.",
    },
    {
        "mixer",
        DocLuaValueKind::Table,
        false,
        DocMetadataStatus::Implemented,
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
        DocMetadataStatus::Implemented,
        intBounds(1, app::sequencer::MAX_PATTERN_STEPS),
        {},
        "",
        "Pattern step count.",
    },
    {
        "stepsPerBeat",
        DocLuaValueKind::Integer,
        true,
        DocMetadataStatus::Implemented,
        intBounds(1, app::sequencer::MAX_STEPS_PER_BEAT),
        {},
        "",
        "Timing resolution for the pattern.",
    },
    {
        "steps",
        DocLuaValueKind::Table,
        true,
        DocMetadataStatus::Implemented,
        {},
        {},
        doctype::Step,
        "Array of authored step tables.",
    },
};

constexpr DocFieldMetadata kStepFields[] = {
    {"active", DocLuaValueKind::Boolean, false},
    {
        "note",
        DocLuaValueKind::Integer,
        false,
        DocMetadataStatus::Implemented,
        intBounds(0, 127),
    },
    {
        "velocity",
        DocLuaValueKind::Integer,
        false,
        DocMetadataStatus::Implemented,
        intBounds(0, 127),
    },
    {
        "gate",
        DocLuaValueKind::Number,
        false,
        DocMetadataStatus::Implemented,
        {},
        finiteMin(0.0),
    },
    {"legato", DocLuaValueKind::Boolean, false},
    {
        "locks",
        DocLuaValueKind::Table,
        false,
        DocMetadataStatus::Implemented,
        {},
        {},
        doctype::StepLock,
        "At most MAX_LOCKS_PER_STEP entries.",
    },
};

constexpr DocFieldMetadata kStepLockFields[] = {
    {
        "param",
        DocLuaValueKind::String,
        true,
        DocMetadataStatus::Implemented,
        {},
        {},
        "",
        "Synth parameter name from synth::param::PARAM_DEFS.",
    },
    {"value", DocLuaValueKind::Number, true},
};

constexpr DocTypeMetadata kAuthoredTypes[] = {
    {
        doctype::TrackSettings,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kTrackSettingsFields),
        "Implemented sequencer track settings.",
    },
    {
        doctype::PatternSlots,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        {},
        "One-based pattern slot table, 1..PATTERNS_PER_LANE.",
    },
    {
        doctype::Pattern,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kPatternFields),
        "Sequencer lane pattern.",
    },
    {
        doctype::Step,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kStepFields),
        "Sequencer step event.",
    },
    {
        doctype::StepLock,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kStepLockFields),
        "Per-step synth parameter lock.",
    },
    {
        doctype::SynthSettings,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kSynthSettingsFields),
        "Patch-style authored synth settings.",
    },
    {doctype::SynthOscSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthOscFields),
     "Oscillator settings."},
    {doctype::SynthNoiseSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthNoiseFields),
     "Noise settings."},
    {doctype::SynthAmpEnvSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthAmpEnvFields),
     "Amp envelope settings."},
    {doctype::SynthSVFSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthSVFFields),
     "SVF settings."},
    {doctype::SynthLadderSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthLadderFields),
     "Ladder filter settings."},
    {doctype::SynthMonoSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthMonoFields),
     "Mono settings."},
    {doctype::SynthPortaSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthPortaFields),
     "Portamento settings."},
    {doctype::SynthUnisonSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthUnisonFields),
     "Unison settings."},
    {doctype::SynthPitchBendSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthPitchBendFields),
     "Pitch bend settings."},
    {doctype::SynthMasterSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthMasterFields),
     "Master synth settings."},
    {doctype::SynthFXSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthFXFields),
     "FX settings."},
    {doctype::SynthFXUnitSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kSynthFXUnitFields),
     "FX unit settings."},
    {doctype::MixerSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kMixerTrackSettingsFields),
     "Patch-style authored track mixer settings."},
    {doctype::MixerTrackSettings,
     DocMetadataSurface::AuthoredDocument,
     DocMetadataStatus::Implemented,
     spanOf(kMixerTrackSettingsFields),
     "Track-scoped mixer settings (gain, pan, mute)."},
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

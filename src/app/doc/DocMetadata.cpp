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
        DocMetadataStatus::Reserved,
        {},
        "Reserved constructor; authored synth semantics are not implemented.",
    },
    {
        doctype::MixerSettings,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Reserved,
        {},
        "Reserved constructor; authored mixer semantics are not implemented.",
    },
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
        docglobal::Track,
        DocMetadataSurface::AuthoredDocument,
        DocMetadataStatus::Implemented,
        spanOf(kTrackArgs),
        "",
        "Capture authored sequencer state for one track.",
    },
};

constexpr const char* kAuthoredConstructors[] = {
    docctor::TrackSettings,
    docctor::SynthSettings,
    docctor::MixerSettings,
};

constexpr DocDiagnosticMetadata kDiagnostics[] = {
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

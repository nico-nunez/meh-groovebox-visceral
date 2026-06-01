#pragma once

#include "app/doc/DocDiagnostics.h"

#include <cstddef>
#include <cstdint>

namespace app::doc {

template <typename T> struct DocMetadataSpan {
  const T* data = nullptr;
  std::size_t size = 0;

  const T* begin() const { return data; }
  const T* end() const { return data ? data + size : nullptr; }
  bool empty() const { return size == 0; }
};

enum class DocMetadataSurface : uint8_t {
  AuthoredDocument,
  RuntimeFileApply,
};

enum class DocMetadataStatus : uint8_t {
  Implemented,
  Reserved,
};

enum class DocLuaValueKind : uint8_t {
  Any,
  Boolean,
  Integer,
  Number,
  String,
  Table,
  Function,
};

struct DocIntegerBounds {
  int min = 0;
  int max = 0;
  bool hasMin = false;
  bool hasMax = false;
};

struct DocNumberBounds {
  double min = 0.0;
  bool hasMin = false;
  bool finiteOnly = false;
};

struct DocFieldMetadata {
  const char* name = "";
  DocLuaValueKind kind = DocLuaValueKind::Any;
  bool required = false;
  DocMetadataStatus status = DocMetadataStatus::Implemented;
  DocIntegerBounds integerBounds{};
  DocNumberBounds numberBounds{};
  const char* elementType = "";
  const char* doc = "";
};

struct DocTypeMetadata {
  const char* name = "";
  DocMetadataSurface surface = DocMetadataSurface::AuthoredDocument;
  DocMetadataStatus status = DocMetadataStatus::Implemented;
  DocMetadataSpan<DocFieldMetadata> fields{};
  const char* doc = "";
};

struct DocFunctionArgMetadata {
  const char* name = "";
  DocLuaValueKind kind = DocLuaValueKind::Any;
  const char* typeName = "";
  DocIntegerBounds integerBounds{};
  const char* doc = "";
};

struct DocFunctionMetadata {
  const char* name = "";
  DocMetadataSurface surface = DocMetadataSurface::AuthoredDocument;
  DocMetadataStatus status = DocMetadataStatus::Implemented;
  DocMetadataSpan<DocFunctionArgMetadata> args{};
  const char* returnsTypeName = "";
  const char* doc = "";
};

struct DocDiagnosticMetadata {
  const char* code = "";
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
  DiagnosticSource source = DiagnosticSource::Parser;
  DocMetadataSurface surface = DocMetadataSurface::AuthoredDocument;
  DocMetadataStatus status = DocMetadataStatus::Implemented;
  const char* targetShape = "";
  const char* doc = "";
};

DocMetadataSpan<DocFunctionMetadata> authoredDocumentFunctions();
DocMetadataSpan<DocTypeMetadata> authoredDocumentTypes();
DocMetadataSpan<const char*> authoredDocumentConstructors();
DocMetadataSpan<DocDiagnosticMetadata> documentDiagnosticCatalog();

const DocFunctionMetadata* findAuthoredDocumentFunction(const char* name);
const DocTypeMetadata* findAuthoredDocumentType(const char* name);
const DocDiagnosticMetadata* findDocumentDiagnostic(const char* code);

namespace docglobal {
inline constexpr const char* Track = "track";
inline constexpr const char* Synth = "synth";
inline constexpr const char* Mixer = "mixer";
} // namespace docglobal

namespace docctor {
inline constexpr const char* TrackSettings = "TrackSettings";
inline constexpr const char* SynthSettings = "SynthSettings";
inline constexpr const char* MixerSettings = "MixerSettings";
} // namespace docctor

namespace doctype {
inline constexpr const char* SynthSettings = "SynthSettings";
inline constexpr const char* SynthOscSettings = "SynthOscSettings";
inline constexpr const char* SynthLfoSettings = "SynthLfoSettings";
inline constexpr const char* SynthNoiseSettings = "SynthNoiseSettings";
inline constexpr const char* SynthAmpEnvSettings = "SynthAmpEnvSettings";
inline constexpr const char* SynthModEnvSettings = "SynthModEnvSettings";
inline constexpr const char* SynthFilterEnvSettings = "SynthFilterEnvSettings";
inline constexpr const char* SynthSVFSettings = "SynthSVFSettings";
inline constexpr const char* SynthLadderSettings = "SynthLadderSettings";
inline constexpr const char* SynthSaturatorSettings = "SynthSaturatorSettings";
inline constexpr const char* SynthMonoSettings = "SynthMonoSettings";
inline constexpr const char* SynthPortaSettings = "SynthPortaSettings";
inline constexpr const char* SynthUnisonSettings = "SynthUnisonSettings";
inline constexpr const char* SynthPitchBendSettings = "SynthPitchBendSettings";
inline constexpr const char* SynthModRouteEntry = "SynthModRouteEntry";
inline constexpr const char* SynthModMatrixEntries = "SynthModRouteEntry[]";
inline constexpr const char* SynthFMRouteEntry = "SynthFMRouteEntry";
inline constexpr const char* SynthFMRouteEntries = "SynthFMRouteEntry[]";
inline constexpr const char* SynthSignalChain = "string[]";
inline constexpr const char* SynthMasterSettings = "SynthMasterSettings";
inline constexpr const char* SynthFXSettings = "SynthFXSettings";
inline constexpr const char* SynthFXDistortionSettings = "SynthFXDistortionSettings";
inline constexpr const char* SynthFXChorusSettings = "SynthFXChorusSettings";
inline constexpr const char* SynthFXPhaserSettings = "SynthFXPhaserSettings";
inline constexpr const char* SynthFXDelaySettings = "SynthFXDelaySettings";
inline constexpr const char* SynthFXReverbSettings = "SynthFXReverbSettings";

inline constexpr const char* MixerSettings = "MixerSettings";
inline constexpr const char* MixerTrackSettings = "MixerTrackSettings";

inline constexpr const char* TrackSettings = "TrackSettings";
inline constexpr const char* PatternSlots = "PatternSlots";
inline constexpr const char* Pattern = "Pattern";
inline constexpr const char* Step = "Step";
inline constexpr const char* StepLock = "StepLock";
} // namespace doctype

namespace docdiag {
inline constexpr const char* MixerPlanningFailed = "mixer.planning_failed";
inline constexpr const char* MixerTrackInvalidIndex = "mixer.track.invalid_index";
inline constexpr const char* MixerSettingsInvalidShape = "mixer.settings.invalid_shape";
inline constexpr const char* MixerParamUnknown = "mixer.param.unknown";
inline constexpr const char* MixerParamTypeMismatch = "mixer.param.type_mismatch";
inline constexpr const char* MixerParamOutOfRange = "mixer.param.out_of_range";
inline constexpr const char* MixerParamDuplicateWrite = "mixer.param.duplicate_write";
inline constexpr const char* MixerAdmissionFailed = "mixer.admission_failed";

inline constexpr const char* SynthPlanningFailed = "synth.planning_failed";
inline constexpr const char* SynthTrackInvalidIndex = "synth.track.invalid_index";
inline constexpr const char* SynthSettingsInvalidShape = "synth.settings.invalid_shape";
inline constexpr const char* SynthParamUnknown = "synth.param.unknown";
inline constexpr const char* SynthParamTypeMismatch = "synth.param.type_mismatch";
inline constexpr const char* SynthParamEnumUnknown = "synth.param.enum_unknown";
inline constexpr const char* SynthParamOutOfRange = "synth.param.out_of_range";
inline constexpr const char* SynthParamDuplicateWrite = "synth.param.duplicate_write";
inline constexpr const char* SynthAdmissionFailed = "synth.admission_failed";
inline constexpr const char* SynthModRouteInvalidShape = "synth.mod_route.invalid_shape";
inline constexpr const char* SynthModRouteInvalidSrc = "synth.mod_route.invalid_src";
inline constexpr const char* SynthModRouteInvalidDest = "synth.mod_route.invalid_dest";
inline constexpr const char* SynthModRouteCapacityExceeded = "synth.mod_route.capacity_exceeded";
inline constexpr const char* SynthFMRouteInvalidShape = "synth.fm_route.invalid_shape";
inline constexpr const char* SynthFMRouteInvalidCarrier = "synth.fm_route.invalid_carrier";
inline constexpr const char* SynthFMRouteInvalidMod = "synth.fm_route.invalid_mod";
inline constexpr const char* SynthFMRouteSelfMod = "synth.fm_route.self_mod";
inline constexpr const char* SynthFMRouteOutOfRange = "synth.fm_route.out_of_range";
inline constexpr const char* SynthFMRouteDuplicate = "synth.fm_route.duplicate";
inline constexpr const char* SynthFMRouteCapacityExceeded = "synth.fm_route.capacity_exceeded";
inline constexpr const char* SynthSignalChainInvalidShape = "synth.signal_chain.invalid_shape";
inline constexpr const char* SynthSignalChainUnknownProcessor =
    "synth.signal_chain.unknown_processor";
inline constexpr const char* SynthSignalChainDuplicate = "synth.signal_chain.duplicate";
inline constexpr const char* SynthSignalChainCapacityExceeded =
    "synth.signal_chain.capacity_exceeded";

inline constexpr const char* SequencerPlanningFailed = "sequencer.planning_failed";
inline constexpr const char* SequencerTrackInvalidIndex = "sequencer.track.invalid_index";
inline constexpr const char* SequencerTrackInvalidSettings = "sequencer.track.invalid_settings";
inline constexpr const char* SequencerPatternsInvalidShape = "sequencer.patterns.invalid_shape";
inline constexpr const char* SequencerPatternSlotInvalidKey = "sequencer.pattern.slot_invalid_key";
inline constexpr const char* SequencerPatternSlotOutOfRange = "sequencer.pattern.slot_out_of_range";
inline constexpr const char* SequencerPatternInvalidShape = "sequencer.pattern.invalid_shape";
inline constexpr const char* SequencerActiveSlotInvalidType = "sequencer.active_slot.invalid_type";
inline constexpr const char* SequencerActiveSlotOutOfRange = "sequencer.active_slot.out_of_range";
inline constexpr const char* SequencerActiveSlotEmptySlot = "sequencer.active_slot.empty_slot";
inline constexpr const char* SequencerAdmissionFailed = "sequencer.admission_failed";
inline constexpr const char* SequencerActiveSlotMissingPatterns =
    "sequencer.active_slot.missing_patterns";

inline constexpr const char* InternalPlannerError = "document.planner.internal_error";

inline constexpr const char* DocumentLuaStateFailed = "document.lua_state_failed";
inline constexpr const char* DocumentLuaEvalFailed = "document.lua_eval_failed";
inline constexpr const char* DocumentFileReadFailed = "document.file.read_failed";
} // namespace docdiag

} // namespace app::doc

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
}

namespace docctor {
inline constexpr const char* TrackSettings = "TrackSettings";
inline constexpr const char* SynthSettings = "SynthSettings";
inline constexpr const char* MixerSettings = "MixerSettings";
} // namespace docctor

namespace doctype {
inline constexpr const char* TrackSettings = "TrackSettings";
inline constexpr const char* PatternSlots = "PatternSlots";
inline constexpr const char* Pattern = "Pattern";
inline constexpr const char* Step = "Step";
inline constexpr const char* StepLock = "StepLock";
inline constexpr const char* SynthSettings = "SynthSettings";
inline constexpr const char* MixerSettings = "MixerSettings";
} // namespace doctype

namespace docdiag {
inline constexpr const char* SequencerTrackInvalidIndex = "sequencer.track.invalid_index";
inline constexpr const char* SequencerTrackInvalidSettings = "sequencer.track.invalid_settings";
inline constexpr const char* SequencerPatternsInvalidShape = "sequencer.patterns.invalid_shape";
inline constexpr const char* SequencerPatternSlotInvalidKey = "sequencer.pattern.slot_invalid_key";
inline constexpr const char* SequencerPatternSlotOutOfRange = "sequencer.pattern.slot_out_of_range";
inline constexpr const char* SequencerPatternInvalidShape = "sequencer.pattern.invalid_shape";
inline constexpr const char* SequencerActiveSlotInvalidType = "sequencer.active_slot.invalid_type";
inline constexpr const char* SequencerActiveSlotOutOfRange = "sequencer.active_slot.out_of_range";
inline constexpr const char* SequencerActiveSlotMissingPatterns =
    "sequencer.active_slot.missing_patterns";
inline constexpr const char* SequencerActiveSlotEmptySlot = "sequencer.active_slot.empty_slot";
inline constexpr const char* SequencerAdmissionFailed = "sequencer.admission_failed";
inline constexpr const char* DocumentLuaStateFailed = "document.lua_state_failed";
inline constexpr const char* DocumentLuaEvalFailed = "document.lua_eval_failed";
inline constexpr const char* DocumentFileReadFailed = "document.file.read_failed";
} // namespace docdiag
} // namespace app::doc

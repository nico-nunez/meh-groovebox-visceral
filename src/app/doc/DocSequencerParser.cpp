#include "app/doc/DocSequencerParser.h"

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocMetadata.h"
#include "app/doc/DocSynthSettingsMetadata.h"
#include "lua/LuaState.h"

#include "synth/params/ParamUtils.h"

#include <cmath>
#include <string>

namespace app::doc {
namespace {

struct LuaSequencerParseContext {
  DocID documentID = 0;
  DocRevision revision = 0;
  AuthoredDocModel* model = nullptr;
  DocDiagnostics diagnostics{};
};

// ====================
// Helpers
// ====================
bool tableHasField(lua_State* L, int tableIndex, const char* field) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, field);
  const bool present = !lua_isnil(L, -1);
  lua_pop(L, 1);
  return present;
}

bool readBoolField(lua_State* L, int tableIndex, const char* field, bool* out) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, field);
  if (!lua_isboolean(L, -1)) {
    lua_pop(L, 1);
    return false;
  }
  *out = lua_toboolean(L, -1) != 0;
  lua_pop(L, 1);
  return true;
}

bool readUInt7Field(lua_State* L, int tableIndex, const char* field, uint8_t* out) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, field);
  if (!lua_isinteger(L, -1)) {
    lua_pop(L, 1);
    return false;
  }
  const int value = static_cast<int>(lua_tointeger(L, -1));
  lua_pop(L, 1);
  if (value < 0 || value > 127)
    return false;
  *out = static_cast<uint8_t>(value);
  return true;
}

bool readPositiveUInt8Field(lua_State* L,
                            int tableIndex,
                            const char* field,
                            uint8_t max,
                            uint8_t* out) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, field);
  if (!lua_isinteger(L, -1)) {
    lua_pop(L, 1);
    return false;
  }
  const int value = static_cast<int>(lua_tointeger(L, -1));
  lua_pop(L, 1);
  if (value < 1 || value > static_cast<int>(max))
    return false;
  *out = static_cast<uint8_t>(value);
  return true;
}

bool finiteNumber(lua_State* L, int index) {
  return lua_isnumber(L, index) && std::isfinite(lua_tonumber(L, index));
}

SourceSpan currentLuaCallSpan(lua_State* L) {
  SourceSpan span{};

  lua_Debug ar{};
  if (lua_getstack(L, 1, &ar) && lua_getinfo(L, "Sl", &ar)) {
    if (ar.currentline > 0) {
      span.line = static_cast<uint32_t>(ar.currentline);
      span.endLine = span.line;
    }
  }

  return span;
}

// ==================
// Target
// ==================
std::string mixerTarget(uint8_t trackIndex, const char* authoredField) {
  std::string target = "mixer:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  if (authoredField && authoredField[0] != '\0') {
    target += ".";
    target += authoredField;
  }
  return target;
}

std::string trackTarget(uint8_t trackIndex, const char* suffix) {
  std::string target = "track:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  if (suffix && suffix[0] != '\0') {
    target += ".";
    target += suffix;
  }
  return target;
}

std::string synthTarget(uint8_t trackIndex, const char* authoredPath) {
  std::string target = "synth:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  if (authoredPath && authoredPath[0] != '\0') {
    target += ".";
    target += authoredPath;
  }
  return target;
}

// ==================
// Diagnostic
// ==================
void pushDiagnostic(LuaSequencerParseContext& ctx,
                    DiagnosticSource source,
                    const char* code,
                    const char* message,
                    SourceSpan span,
                    const char* relatedTarget) {
  DocDiagnostic diagnostic{};
  diagnostic.severity = DiagnosticSeverity::Error;
  diagnostic.source = source;
  diagnostic.documentID = ctx.documentID;
  diagnostic.revision = ctx.revision;
  diagnostic.code = code;
  diagnostic.message = message ? message : "";
  diagnostic.span = span;
  diagnostic.relatedTarget = relatedTarget ? relatedTarget : "";
  ctx.diagnostics.push_back(diagnostic);
}

// =================
// Writers
// =================
bool appendMixerWrite(LuaSequencerParseContext& ctx,
                      AuthoredTrackMixerPatch& patch,
                      const AuthoredMixerParamField& field,
                      float value,
                      SourceSpan span) {
  for (const auto& existing : patch.writes) {
    if (existing.paramID != field.paramID)
      continue;
    if (existing.value == value)
      return true;
    const std::string target = mixerTarget(patch.trackIndex, field.authoredField);
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerParamDuplicateWrite,
                   "mixer param written more than once",
                   span,
                   target.c_str());
    return false;
  }
  patch.writes.push_back({field.paramID, value, &field, span});
  return true;
}

bool appendSynthWrite(LuaSequencerParseContext& ctx,
                      AuthoredTrackSynthPatch& patch,
                      const AuthoredSynthParamField& field,
                      float value,
                      SourceSpan span) {
  for (const auto& existing : patch.writes) {
    if (existing.paramID != field.paramID)
      continue;

    if (existing.value == value)
      return true;

    const std::string target = synthTarget(patch.trackIndex, field.authoredPath);
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthParamDuplicateWrite,
                   "synth param written more than once",
                   span,
                   target.c_str());
    return false;
  }

  patch.writes.push_back({field.paramID, value, &field, span});
  return true;
}

// ==================
// Parsers
// ==================
bool parseSparseStepLocksPatch(lua_State* L,
                               int locksIndex,
                               LuaSequencerParseContext& ctx,
                               AuthoredTrackSeqModel& track,
                               AuthoredStepLocksPatch* out) {
  if (!lua_istable(L, locksIndex)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerPatternInvalidShape,
                   "locks must be a table or false",
                   track.trackSpan,
                   "step.locks");
    return false;
  }

  const int absLocks = lua_absindex(L, locksIndex);
  const int numLocks = static_cast<int>(lua_rawlen(L, absLocks));
  if (numLocks > static_cast<int>(sequencer::MAX_LOCKS_PER_STEP)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerPatternInvalidShape,
                   "too many step locks",
                   track.trackSpan,
                   "step.locks");
    return false;
  }

  out->op = app::PatchObjectOp::Replace;
  out->span = track.trackSpan;
  out->numLocks = 0;

  bool ok = true;
  for (int i = 0; i < numLocks; ++i) {
    lua_rawgeti(L, absLocks, i + 1);
    if (!lua_istable(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "lock must be a table",
                     track.trackSpan,
                     "step.locks");
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const int lockIndex = lua_absindex(L, -1);

    lua_getfield(L, lockIndex, "param");
    if (!lua_isstring(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "lock param must be a string",
                     track.trackSpan,
                     "step.locks.param");
      lua_pop(L, 2);
      ok = false;
      continue;
    }

    const char* paramName = lua_tostring(L, -1);
    auto paramID = synth::param::utils::getParamIDByName(paramName);
    lua_pop(L, 1);
    if (paramID == synth::param::PARAM_UNKNOWN) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "unknown lock param",
                     track.trackSpan,
                     "step.locks.param");
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    lua_getfield(L, lockIndex, "value");
    if (!lua_isnumber(L, -1) || !std::isfinite(lua_tonumber(L, -1))) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "lock value must be finite numeric",
                     track.trackSpan,
                     "step.locks.value");
      lua_pop(L, 2);
      ok = false;
      continue;
    }
    const float value = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    bool duplicate = false;
    for (uint8_t existing = 0; existing < out->numLocks; ++existing) {
      if (out->locks[existing].paramID == static_cast<uint8_t>(paramID)) {
        pushDiagnostic(ctx,
                       DiagnosticSource::Validator,
                       docdiag::SequencerPatternInvalidShape,
                       "duplicate lock param",
                       track.trackSpan,
                       "step.locks.param");
        ok = false;
        duplicate = true;
        break;
      }
    }

    if (!duplicate)
      out->locks[out->numLocks++] = {static_cast<uint8_t>(paramID), value};

    lua_pop(L, 1);
  }

  return ok;
}

bool parseSparseStepPatch(lua_State* L,
                          int stepIndex,
                          LuaSequencerParseContext& ctx,
                          AuthoredTrackSeqModel& track,
                          AuthoredStepPatch* out) {
  const int absStep = lua_absindex(L, stepIndex);
  out->op = app::PatchObjectOp::Patch;
  out->span = track.trackSpan;
  bool ok = true;

  if (tableHasField(L, absStep, "active")) {
    out->hasActive = true;
    if (!readBoolField(L, absStep, "active", &out->active)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "active must be boolean",
                     out->span,
                     "step.active");
      ok = false;
    }
    out->hasNoteOn = true;
    out->noteOn = out->active;
  }

  if (tableHasField(L, absStep, "note")) {
    out->hasNote = true;
    if (!readUInt7Field(L, absStep, "note", &out->note)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "note out of range",
                     out->span,
                     "step.note");
      ok = false;
    }
  }

  if (tableHasField(L, absStep, "velocity")) {
    out->hasVelocity = true;
    if (!readUInt7Field(L, absStep, "velocity", &out->velocity)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "velocity out of range",
                     out->span,
                     "step.velocity");
      ok = false;
    }
  }

  if (tableHasField(L, absStep, "gate")) {
    lua_getfield(L, absStep, "gate");
    const bool valid =
        lua_isnumber(L, -1) && std::isfinite(lua_tonumber(L, -1)) && lua_tonumber(L, -1) >= 0.0;
    if (valid) {
      out->hasGate = true;
      out->gate = static_cast<float>(lua_tonumber(L, -1));
    } else {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "gate out of range",
                     out->span,
                     "step.gate");
      ok = false;
    }
    lua_pop(L, 1);
  }

  if (tableHasField(L, absStep, "legato")) {
    out->hasLegato = true;
    if (!readBoolField(L, absStep, "legato", &out->legato)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "legato must be boolean",
                     out->span,
                     "step.legato");
      ok = false;
    }
  }

  if (tableHasField(L, absStep, "locks")) {
    lua_getfield(L, absStep, "locks");
    if (lua_isboolean(L, -1) && lua_toboolean(L, -1) == 0) {
      out->locks.op = app::PatchObjectOp::Clear;
      out->locks.span = out->span;
      out->locks.numLocks = 0;
    } else {
      ok = parseSparseStepLocksPatch(L, lua_absindex(L, -1), ctx, track, &out->locks) && ok;
    }
    lua_pop(L, 1);
  }

  return ok;
}

bool parseSparsePatternPatch(lua_State* L,
                             int patternIndex,
                             LuaSequencerParseContext& ctx,
                             AuthoredTrackSeqModel& track,
                             AuthoredPatternPatch* out) {
  if (!lua_istable(L, patternIndex))
    return false;

  const int absPattern = lua_absindex(L, patternIndex);
  out->op = app::PatchObjectOp::Patch;
  out->span = track.trackSpan;
  bool ok = true;

  if (tableHasField(L, absPattern, "numSteps")) {
    out->hasNumSteps = true;
    if (!readPositiveUInt8Field(L,
                                absPattern,
                                "numSteps",
                                sequencer::MAX_PATTERN_STEPS,
                                &out->numSteps)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "numSteps out of range",
                     out->span,
                     "pattern.numSteps");
      ok = false;
    }
  }

  if (tableHasField(L, absPattern, "stepsPerBeat")) {
    out->hasStepsPerBeat = true;
    if (!readPositiveUInt8Field(L,
                                absPattern,
                                "stepsPerBeat",
                                sequencer::MAX_STEPS_PER_BEAT,
                                &out->stepsPerBeat)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "stepsPerBeat out of range",
                     out->span,
                     "pattern.stepsPerBeat");
      ok = false;
    }
  }

  lua_getfield(L, absPattern, "steps");
  if (!lua_isnil(L, -1)) {
    if (!lua_istable(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "steps must be a table",
                     out->span,
                     "pattern.steps");
      lua_pop(L, 1);
      return false;
    }

    const int stepsIndex = lua_absindex(L, -1);
    lua_pushnil(L);
    while (lua_next(L, stepsIndex) != 0) {
      if (!lua_isinteger(L, -2)) {
        pushDiagnostic(ctx,
                       DiagnosticSource::Validator,
                       docdiag::SequencerPatternInvalidShape,
                       "step key must be an integer",
                       out->span,
                       "pattern.steps");
        lua_pop(L, 1);
        ok = false;
        continue;
      }

      const int stepNumber = static_cast<int>(lua_tointeger(L, -2));
      if (stepNumber < 1 || stepNumber > static_cast<int>(sequencer::MAX_PATTERN_STEPS)) {
        pushDiagnostic(ctx,
                       DiagnosticSource::Validator,
                       docdiag::SequencerPatternInvalidShape,
                       "step key out of range",
                       out->span,
                       "pattern.steps");
        lua_pop(L, 1);
        ok = false;
        continue;
      }

      const uint8_t step = static_cast<uint8_t>(stepNumber - 1);
      out->hasStep[step] = true;
      if (lua_isboolean(L, -1) && lua_toboolean(L, -1) == 0) {
        out->steps[step].op = app::PatchObjectOp::Clear;
        out->steps[step].span = track.trackSpan;
      } else if (lua_istable(L, -1)) {
        ok = parseSparseStepPatch(L, lua_absindex(L, -1), ctx, track, &out->steps[step]) && ok;
      } else {
        pushDiagnostic(ctx,
                       DiagnosticSource::Validator,
                       docdiag::SequencerPatternInvalidShape,
                       "step must be a table or false",
                       out->span,
                       "pattern.steps");
        ok = false;
      }
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  return ok;
}

bool parseMixerScalarValue(lua_State* L,
                           int valueIndex,
                           LuaSequencerParseContext& ctx,
                           uint8_t trackIndex,
                           const AuthoredMixerParamField& field,
                           SourceSpan span,
                           float& outValue) {
  namespace ap = app::params;
  const auto& def = ap::getAppParamDef(field.paramID);
  const std::string target = mixerTarget(trackIndex, field.authoredField);

  switch (field.valueKind) {
  case DocLuaValueKind::Boolean:
    if (!lua_isboolean(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::MixerParamTypeMismatch,
                     "mixer param must be a boolean",
                     span,
                     target.c_str());
      return false;
    }
    outValue = lua_toboolean(L, valueIndex) ? 1.0f : 0.0f;
    return true;

  case DocLuaValueKind::Number:
    if (!finiteNumber(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::MixerParamTypeMismatch,
                     "mixer param must be a finite number",
                     span,
                     target.c_str());
      return false;
    }
    outValue = static_cast<float>(lua_tonumber(L, valueIndex));
    break;

  default:
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerParamTypeMismatch,
                   "unsupported mixer param value kind",
                   span,
                   target.c_str());
    return false;
  }

  if (outValue < def.min || outValue > def.max) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerParamOutOfRange,
                   "mixer param value out of range",
                   span,
                   target.c_str());
    return false;
  }

  return true;
}

bool parsePatternsField(lua_State* L,
                        int settingsIndex,
                        LuaSequencerParseContext& ctx,
                        AuthoredTrackSeqModel& track) {
  const int absSettingsIndex = lua_absindex(L, settingsIndex);
  lua_getfield(L, absSettingsIndex, "patterns");

  if (lua_isboolean(L, -1) && lua_toboolean(L, -1) == 0) {
    track.patternBankOp = app::PatchObjectOp::Clear;
    lua_pop(L, 1);
    return true;
  }

  if (!lua_istable(L, -1)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerPatternsInvalidShape,
                   "patterns must be a table or false",
                   track.patternsSpan,
                   "track.patterns");
    lua_pop(L, 1);
    return false;
  }

  track.patternBankOp = app::PatchObjectOp::Patch;
  const int patternsIndex = lua_absindex(L, -1);
  bool ok = true;

  lua_pushnil(L);
  while (lua_next(L, patternsIndex) != 0) {
    if (!lua_isinteger(L, -2)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternSlotInvalidKey,
                     "pattern slot key must be an integer",
                     track.patternsSpan,
                     "track.patterns");
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const int slotNumber = static_cast<int>(lua_tointeger(L, -2));
    if (slotNumber < 1 || slotNumber > static_cast<int>(sequencer::PATTERNS_PER_LANE)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternSlotOutOfRange,
                     "pattern slot out of range",
                     track.patternsSpan,
                     "track.patterns");
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const uint8_t slot = static_cast<uint8_t>(slotNumber - 1);
    AuthoredPatternSlotPatch& slotPatch = track.patternSlots[slot];
    slotPatch.span = track.trackSpan;

    if (lua_isboolean(L, -1) && lua_toboolean(L, -1) == 0) {
      track.hasPatternSlot[slot] = true;
      slotPatch.op = app::PatchObjectOp::Clear;
    } else if (lua_istable(L, -1)) {
      slotPatch.op = app::PatchObjectOp::Patch;
      ok = parseSparsePatternPatch(L, lua_absindex(L, -1), ctx, track, &slotPatch.pattern) && ok;
      track.hasPatternSlot[slot] = hasAuthoredPatternPatchEdits(slotPatch.pattern);
    } else {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "pattern slot must be a table or false",
                     slotPatch.span,
                     "track.patterns");
      ok = false;
    }
    lua_pop(L, 1);
  }

  lua_pop(L, 1);
  return ok;
}

bool parseActiveSlotField(lua_State* L,
                          int settingsIndex,
                          LuaSequencerParseContext& ctx,
                          AuthoredTrackSeqModel& track) {
  const int absSettingsIndex = lua_absindex(L, settingsIndex);
  lua_getfield(L, absSettingsIndex, "activeSlot");
  if (!lua_isinteger(L, -1)) {
    const std::string target = trackTarget(track.trackIndex, "activeSlot");
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerActiveSlotInvalidType,
                   "activeSlot must be an integer",
                   track.activeSlotSpan,
                   target.c_str());
    lua_pop(L, 1);
    return false;
  }

  const int slotNumber = static_cast<int>(lua_tointeger(L, -1));
  lua_pop(L, 1);

  if (slotNumber < 1 || slotNumber > static_cast<int>(sequencer::PATTERNS_PER_LANE)) {
    const std::string target = trackTarget(track.trackIndex, "activeSlot");
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerActiveSlotOutOfRange,
                   "activeSlot out of range",
                   track.activeSlotSpan,
                   target.c_str());
    return false;
  }

  track.activeSlot = static_cast<uint8_t>(slotNumber - 1);
  track.hasActiveSlot = true;
  track.activeSlotSource = ActivePatternSlotSource::Explicit;
  return true;
}

bool parseSynthScalarValue(lua_State* L,
                           int valueIndex,
                           LuaSequencerParseContext& ctx,
                           uint8_t trackIndex,
                           const AuthoredSynthParamField& field,
                           SourceSpan span,
                           float& outValue) {
  const auto& def = synth::param::PARAM_DEFS[field.paramID];
  const std::string target = synthTarget(trackIndex, field.authoredPath);

  switch (field.valueKind) {
  case DocLuaValueKind::Boolean:
    if (!lua_isboolean(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamTypeMismatch,
                     "synth param must be a boolean",
                     span,
                     target.c_str());
      return false;
    }
    outValue = lua_toboolean(L, valueIndex) ? 1.0f : 0.0f;
    return true;

  case DocLuaValueKind::Integer:
    if (!lua_isinteger(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamTypeMismatch,
                     "synth param must be an integer",
                     span,
                     target.c_str());
      return false;
    }
    outValue = static_cast<float>(lua_tointeger(L, valueIndex));
    break;

  case DocLuaValueKind::Number:
    if (!finiteNumber(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamTypeMismatch,
                     "synth param must be a finite number",
                     span,
                     target.c_str());
      return false;
    }
    outValue = static_cast<float>(lua_tonumber(L, valueIndex));
    break;

  case DocLuaValueKind::String: {
    if (!lua_isstring(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamTypeMismatch,
                     "synth enum param must be a string",
                     span,
                     target.c_str());
      return false;
    }
    const char* token = lua_tostring(L, valueIndex);
    auto parsed = synth::param::utils::parseEnum(def.type, token);
    if (!parsed.ok) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamEnumUnknown,
                     parsed.error ? parsed.error : "unknown synth enum token",
                     span,
                     target.c_str());
      return false;
    }
    outValue = static_cast<float>(parsed.value);
    break;
  }

  default:
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthParamTypeMismatch,
                   "unsupported synth param value kind",
                   span,
                   target.c_str());
    return false;
  }

  if (outValue < def.min || outValue > def.max) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthParamOutOfRange,
                   "synth param value out of range",
                   span,
                   target.c_str());
    return false;
  }

  return true;
}

bool parseSynthGroup(lua_State* L,
                     int tableIndex,
                     LuaSequencerParseContext& ctx,
                     AuthoredTrackSynthPatch& patch,
                     const std::string& prefix,
                     SourceSpan span) {
  const int absTableIndex = lua_absindex(L, tableIndex);
  bool ok = true;

  lua_pushnil(L);
  while (lua_next(L, absTableIndex) != 0) {
    if (!lua_isstring(L, -2)) {
      const std::string target = synthTarget(patch.trackIndex, prefix.c_str());
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamUnknown,
                     "synth field key must be a string",
                     span,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const char* key = lua_tostring(L, -2);
    std::string path = prefix.empty() ? key : prefix + "." + key;

    if (lua_istable(L, -1)) {
      ok = parseSynthGroup(L, lua_absindex(L, -1), ctx, patch, path, span) && ok;
      lua_pop(L, 1);
      continue;
    }

    const auto* field = findAuthoredSynthParamField(path.c_str());
    if (!field) {
      const std::string target = synthTarget(patch.trackIndex, path.c_str());
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamUnknown,
                     "unknown or deferred synth param",
                     span,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    float value = 0.0f;
    if (parseSynthScalarValue(L, lua_absindex(L, -1), ctx, patch.trackIndex, *field, span, value))
      ok = appendSynthWrite(ctx, patch, *field, value, span) && ok;
    else
      ok = false;

    lua_pop(L, 1);
  }

  return ok;
}

bool parseMixerSettingsForTrack(lua_State* L,
                                int settingsIndex,
                                LuaSequencerParseContext& ctx,
                                uint8_t trackIndex,
                                SourceSpan span) {
  if (!lua_istable(L, settingsIndex)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerSettingsInvalidShape,
                   "MixerSettings must be a table",
                   span,
                   "mixer");
    return false;
  }

  ctx.model->hasMixerState[trackIndex] = true;
  AuthoredTrackMixerPatch& patch = ctx.model->mixerTracks[trackIndex];
  patch.hasPatch = true;
  patch.trackIndex = trackIndex;
  patch.trackSpan = span;

  const int absIndex = lua_absindex(L, settingsIndex);
  bool ok = true;

  lua_pushnil(L);
  while (lua_next(L, absIndex) != 0) {
    if (!lua_isstring(L, -2)) {
      const std::string target = mixerTarget(trackIndex, "");
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::MixerParamUnknown,
                     "mixer field key must be a string",
                     span,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const char* key = lua_tostring(L, -2);
    const auto* field = findAuthoredTrackMixerParamField(key);
    if (!field) {
      const std::string target = mixerTarget(trackIndex, key);
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::MixerParamUnknown,
                     "unknown or deferred mixer param",
                     span,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    float value = 0.0f;
    if (parseMixerScalarValue(L, lua_absindex(L, -1), ctx, trackIndex, *field, span, value))
      ok = appendMixerWrite(ctx, patch, *field, value, span) && ok;
    else
      ok = false;

    lua_pop(L, 1);
  }

  return ok;
}

bool parseSynthSettingsForTrack(lua_State* L,
                                int settingsIndex,
                                LuaSequencerParseContext& ctx,
                                uint8_t trackIndex,
                                SourceSpan span) {
  if (!lua_istable(L, settingsIndex)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthSettingsInvalidShape,
                   "SynthSettings must be a table",
                   span,
                   "synth");
    return false;
  }

  ctx.model->hasSynthState[trackIndex] = true;
  AuthoredTrackSynthPatch& patch = ctx.model->synthTracks[trackIndex];
  patch.hasPatch = true;
  patch.trackIndex = trackIndex;
  patch.trackSpan = span;

  return parseSynthGroup(L, settingsIndex, ctx, patch, "", span);
}

bool parseTrackSettings(lua_State* L,
                        int settingsIndex,
                        LuaSequencerParseContext& ctx,
                        AuthoredTrackSeqModel& track) {
  const int absSettingsIndex = lua_absindex(L, settingsIndex);

  lua_getfield(L, absSettingsIndex, "patterns");
  const bool patternsFieldPresent = !lua_isnil(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, absSettingsIndex, "activeSlot");
  const bool activeSlotFieldPresent = !lua_isnil(L, -1);
  lua_pop(L, 1);

  bool ok = true;

  lua_getfield(L, absSettingsIndex, "synth");
  const bool synthFieldPresent = !lua_isnil(L, -1);
  if (synthFieldPresent) {
    ok = parseSynthSettingsForTrack(L,
                                    lua_absindex(L, -1),
                                    ctx,
                                    track.trackIndex,
                                    track.trackSpan) &&
         ok;
  }
  lua_pop(L, 1);

  lua_getfield(L, absSettingsIndex, "mixer");
  const bool mixerFieldPresent = !lua_isnil(L, -1);
  if (mixerFieldPresent) {
    ok = parseMixerSettingsForTrack(L,
                                    lua_absindex(L, -1),
                                    ctx,
                                    track.trackIndex,
                                    track.trackSpan) &&
         ok;
  }
  lua_pop(L, 1);

  if (patternsFieldPresent)
    ok = parsePatternsField(L, absSettingsIndex, ctx, track) && ok;

  if (activeSlotFieldPresent)
    ok = parseActiveSlotField(L, absSettingsIndex, ctx, track) && ok;

  if (activeSlotFieldPresent && !patternsFieldPresent) {
    const std::string target = trackTarget(track.trackIndex, "activeSlot");
    pushDiagnostic(ctx,
                   DiagnosticSource::Normalizer,
                   docdiag::SequencerActiveSlotMissingPatterns,
                   "activeSlot requires authored patterns in the same track patch",
                   track.activeSlotSpan,
                   target.c_str());
    ok = false;
  }

  return ok;
}

// =====================
// Capture
// =====================
int l_captureMixer(lua_State* L) {
  auto* ctx = static_cast<LuaSequencerParseContext*>(lua_touserdata(L, lua_upvalueindex(1)));
  if (!ctx)
    return luaL_error(L, "missing authored document parse context");

  const SourceSpan span = currentLuaCallSpan(L);

  if (!lua_isinteger(L, 1)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerTrackInvalidIndex,
                   "mixer track index must be an integer",
                   span,
                   "mixer");
    return 0;
  }

  const int trackNumber = static_cast<int>(lua_tointeger(L, 1));
  if (trackNumber < 1 || trackNumber > static_cast<int>(app::MAX_TRACKS)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerTrackInvalidIndex,
                   "mixer track index out of range",
                   span,
                   "mixer");
    return 0;
  }

  if (!lua_istable(L, 2)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerSettingsInvalidShape,
                   "mixer settings must be a table",
                   span,
                   "mixer");
    return 0;
  }

  const uint8_t trackIndex = static_cast<uint8_t>(trackNumber - 1);
  parseMixerSettingsForTrack(L, 2, *ctx, trackIndex, span);
  return 0;
}

int l_captureSynth(lua_State* L) {
  auto* ctx = static_cast<LuaSequencerParseContext*>(lua_touserdata(L, lua_upvalueindex(1)));
  if (!ctx)
    return luaL_error(L, "missing authored document parse context");

  const SourceSpan span = currentLuaCallSpan(L);

  if (!lua_isinteger(L, 1)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthTrackInvalidIndex,
                   "synth track index must be an integer",
                   span,
                   "synth");
    return 0;
  }

  const int trackNumber = static_cast<int>(lua_tointeger(L, 1));
  if (trackNumber < 1 || trackNumber > static_cast<int>(app::MAX_TRACKS)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthTrackInvalidIndex,
                   "synth track index out of range",
                   span,
                   "synth");
    return 0;
  }

  if (!lua_istable(L, 2)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthSettingsInvalidShape,
                   "synth settings must be a table",
                   span,
                   "synth");
    return 0;
  }

  const uint8_t trackIndex = static_cast<uint8_t>(trackNumber - 1);
  parseSynthSettingsForTrack(L, 2, *ctx, trackIndex, span);
  return 0;
}

int l_captureTrack(lua_State* L) {
  auto* ctx = static_cast<LuaSequencerParseContext*>(lua_touserdata(L, lua_upvalueindex(1)));
  if (!ctx)
    return luaL_error(L, "missing sequencer parse context");

  const SourceSpan trackSpan = currentLuaCallSpan(L);

  if (!lua_isinteger(L, 1)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerTrackInvalidIndex,
                   "track index must be an integer",
                   trackSpan,
                   "track");
    return 0;
  }

  const int trackNumber = static_cast<int>(lua_tointeger(L, 1));
  if (trackNumber < 1 || trackNumber > static_cast<int>(app::MAX_TRACKS)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerTrackInvalidIndex,
                   "track index out of range",
                   trackSpan,
                   "track");
    return 0;
  }

  if (!lua_istable(L, 2)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerTrackInvalidSettings,
                   "track settings must be a table",
                   trackSpan,
                   "track");
    return 0;
  }

  const uint8_t trackIndex = static_cast<uint8_t>(trackNumber - 1);

  AuthoredTrackSeqModel& track = ctx->model->sequencer.tracks[trackIndex];
  track = AuthoredTrackSeqModel{};
  track.trackIndex = trackIndex;
  track.trackSpan = trackSpan;
  track.patternsSpan = trackSpan;
  track.activeSlotSpan = trackSpan;

  parseTrackSettings(L, 2, *ctx, track);
  track.hasSequencerPatch = hasAuthoredTrackSeqPatchEdits(track);
  ctx->model->sequencer.hasTrackState[trackIndex] = track.hasSequencerPatch;

  return 0;
}

int l_plainTableConstructor(lua_State* L) {
  if (lua_istable(L, 1)) {
    lua_pushvalue(L, 1);
    return 1;
  }

  lua_newtable(L);
  return 1;
}

void openParserLibraries(lua_State* L) {
  luaL_requiref(L, "_G", luaopen_base, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
  lua_pop(L, 1);
}

void registerPlainConstructor(lua_State* L, const char* name) {
  lua_pushcfunction(L, l_plainTableConstructor);
  lua_setglobal(L, name);
}

bool isAuthoredConstructor(const char* name) {
  for (const char* constructor : authoredDocumentConstructors()) {
    if (std::strcmp(constructor, name) == 0)
      return true;
  }
  return false;
}

// ===================
// Register
// ===================
void registerParserEnvironment(lua_State* L, LuaSequencerParseContext& ctx) {
  openParserLibraries(L);

  const DocFunctionMetadata* mixerFunction = findAuthoredDocumentFunction(docglobal::Mixer);
  if (mixerFunction && mixerFunction->status == DocMetadataStatus::Implemented) {
    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, l_captureMixer, 1);
    lua_setglobal(L, mixerFunction->name);
  }

  const DocFunctionMetadata* trackFunction = findAuthoredDocumentFunction(docglobal::Track);
  if (trackFunction && trackFunction->status == DocMetadataStatus::Implemented) {
    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, l_captureTrack, 1);
    lua_setglobal(L, trackFunction->name);
  }

  const DocFunctionMetadata* synthFunction = findAuthoredDocumentFunction(docglobal::Synth);
  if (synthFunction && synthFunction->status == DocMetadataStatus::Implemented) {
    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, l_captureSynth, 1);
    lua_setglobal(L, synthFunction->name);
  }

  if (isAuthoredConstructor(docctor::TrackSettings))
    registerPlainConstructor(L, docctor::TrackSettings);
  if (isAuthoredConstructor(docctor::SynthSettings))
    registerPlainConstructor(L, docctor::SynthSettings);
  if (isAuthoredConstructor(docctor::MixerSettings))
    registerPlainConstructor(L, docctor::MixerSettings);
}

} // namespace

AuthoredDocNormalizeResult parseAndNormalizeAuthoredDoc(DocID documentID,
                                                        DocRevision revision,
                                                        const char* bufferText,
                                                        AuthoredDocModel* outModel) {
  AuthoredDocNormalizeResult result{};
  if (!outModel) {
    DocDiagnostic d{};
    d.severity = DiagnosticSeverity::Error;
    d.source = DiagnosticSource::Parser;
    d.documentID = documentID;
    d.revision = revision;
    d.code = docdiag::InternalPlannerError;
    d.message = "null authored document output";
    result.diagnostics.push_back(d);
    return result;
  }

  *outModel = AuthoredDocModel{};

  outModel->documentID = documentID;
  outModel->revision = revision;
  outModel->sequencer.documentID = documentID;
  outModel->sequencer.revision = revision;

  LuaSequencerParseContext ctx{};
  ctx.documentID = documentID;
  ctx.revision = revision;
  ctx.model = outModel;

  lua_State* L = luaL_newstate();
  if (!L) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Parser,
                   docdiag::DocumentLuaStateFailed,
                   "failed to create authoring Lua state",
                   SourceSpan{},
                   "");
    result.diagnostics = ctx.diagnostics;
    return result;
  }

  registerParserEnvironment(L, ctx);

  const char* text = bufferText ? bufferText : "";
  if (luaL_dostring(L, text) != LUA_OK) {
    const char* message = lua_tostring(L, -1);
    pushDiagnostic(ctx,
                   DiagnosticSource::Parser,
                   docdiag::DocumentLuaEvalFailed,
                   message ? message : "failed to evaluate authored document",
                   SourceSpan{},
                   "");
    lua_pop(L, 1);
    lua_close(L);

    resetAuthoredDocModel(ctx.model, ctx.documentID, ctx.revision);
    result.diagnostics = ctx.diagnostics;
    return result;
  }

  lua_close(L);

  result.ok = ctx.diagnostics.empty();
  result.diagnostics = ctx.diagnostics;
  return result;
}

} // namespace app::doc

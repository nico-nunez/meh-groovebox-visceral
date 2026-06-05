#include "ParserHelpers.h"
#include "synth/params/ParamUtils.h"

namespace app::doc {
std::string trackTarget(uint8_t trackIndex, const char* suffix) {
  std::string target = "track:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  if (suffix && suffix[0] != '\0') {
    target += ".";
    target += suffix;
  }
  return target;
}

// ==================
// Parsers
// ==================
bool stepHasInvalidScalarNoteFields(lua_State* L, int absStep) {
  return tableHasField(L, absStep, "noteOn") || tableHasField(L, absStep, "tie") ||
         tableHasField(L, absStep, "note") || tableHasField(L, absStep, "velocity") ||
         tableHasField(L, absStep, "gate") || tableHasField(L, absStep, "legato");
}

std::string stepNoteTarget(const char* targetPrefix, const char* field) {
  std::string target = targetPrefix ? targetPrefix : "step.note";
  if (field && field[0] != '\0') {
    target += ".";
    target += field;
  }
  return target;
}

void pushStepNoteDiagnostic(LuaSequencerParseContext& ctx,
                            SourceSpan span,
                            const char* targetPrefix,
                            const char* field,
                            const char* message) {
  const std::string target = stepNoteTarget(targetPrefix, field);
  pushDiagnostic(ctx,
                 DiagnosticSource::Validator,
                 docdiag::SequencerPatternInvalidShape,
                 message,
                 span,
                 target.c_str());
}

bool validateDenseArrayKeys(lua_State* L,
                            int absTable,
                            size_t count,
                            LuaSequencerParseContext& ctx,
                            SourceSpan span,
                            const char* target) {
  bool ok = true;
  lua_pushnil(L);
  while (lua_next(L, absTable) != 0) {
    bool validKey = lua_isinteger(L, -2);
    if (validKey) {
      const lua_Integer key = lua_tointeger(L, -2);
      validKey = key >= 1 && static_cast<size_t>(key) <= count;
    }

    if (!validKey) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "notes must be a dense array",
                     span,
                     target);
      ok = false;
    }
    lua_pop(L, 1);
  }
  return ok;
}

bool parseStepNotePatch(lua_State* L,
                        int absNote,
                        AuthoredStepNotePatch* out,
                        LuaSequencerParseContext& ctx,
                        SourceSpan span,
                        const char* targetPrefix) {
  bool ok = true;
  out->span = span;

  if (tableHasField(L, absNote, "noteOn")) {
    out->hasNoteOn = true;
    if (!readBoolField(L, absNote, "noteOn", &out->noteOn)) {
      pushStepNoteDiagnostic(ctx, span, targetPrefix, "noteOn", "noteOn must be boolean");
      ok = false;
    }
  }

  if (tableHasField(L, absNote, "tie")) {
    out->hasTie = true;
    if (!readBoolField(L, absNote, "tie", &out->tie)) {
      pushStepNoteDiagnostic(ctx, span, targetPrefix, "tie", "tie must be boolean");
      ok = false;
    }
  }

  if (tableHasField(L, absNote, "note")) {
    out->hasNote = true;
    if (!readUInt7Field(L, absNote, "note", &out->note)) {
      pushStepNoteDiagnostic(ctx, span, targetPrefix, "note", "note out of range");
      ok = false;
    }
  }

  if (tableHasField(L, absNote, "velocity")) {
    out->hasVelocity = true;
    if (!readUInt7Field(L, absNote, "velocity", &out->velocity)) {
      pushStepNoteDiagnostic(ctx, span, targetPrefix, "velocity", "velocity out of range");
      ok = false;
    }
  }

  if (tableHasField(L, absNote, "gate")) {
    lua_getfield(L, absNote, "gate");
    const bool valid =
        lua_isnumber(L, -1) && std::isfinite(lua_tonumber(L, -1)) && lua_tonumber(L, -1) >= 0.0;
    if (valid) {
      out->hasGate = true;
      out->gate = static_cast<float>(lua_tonumber(L, -1));
    } else {
      pushStepNoteDiagnostic(ctx, span, targetPrefix, "gate", "gate out of range");
      ok = false;
    }
    lua_pop(L, 1);
  }

  return ok;
}

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
  }

  const bool hasNotesArray = tableHasField(L, absStep, "notes");
  const bool hasInvalidScalarNoteFields = stepHasInvalidScalarNoteFields(L, absStep);

  if (hasInvalidScalarNoteFields) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerPatternInvalidShape,
                   "step note data must be authored through notes[]",
                   out->span,
                   "step.notes");
    ok = false;
  }

  if (hasNotesArray) {
    lua_getfield(L, absStep, "notes");
    if (!lua_istable(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternInvalidShape,
                     "notes must be an array",
                     out->span,
                     "step.notes");
      ok = false;
    } else {
      const size_t count = lua_rawlen(L, -1);
      const int absNotes = lua_absindex(L, -1);

      ok = validateDenseArrayKeys(L, absNotes, count, ctx, out->span, "step.notes") && ok;

      if (count > sequencer::MAX_NOTES_PER_STEP) {
        pushDiagnostic(ctx,
                       DiagnosticSource::Validator,
                       docdiag::SequencerPatternInvalidShape,
                       "too many notes in step",
                       out->span,
                       "step.notes");
        ok = false;
      }

      const size_t capped = std::min<size_t>(count, sequencer::MAX_NOTES_PER_STEP);
      out->hasNoteCount = true;
      out->noteCount = static_cast<uint8_t>(capped);
      for (size_t i = 0; i < capped; ++i) {
        lua_rawgeti(L, absNotes, static_cast<lua_Integer>(i + 1));
        if (!lua_istable(L, -1)) {
          pushDiagnostic(ctx,
                         DiagnosticSource::Validator,
                         docdiag::SequencerPatternInvalidShape,
                         "notes entry must be a table",
                         out->span,
                         "step.notes[]");
          ok = false;
        } else {
          out->hasNotePatch[i] = true;
          ok = parseStepNotePatch(L,
                                  lua_absindex(L, -1),
                                  &out->notes[i],
                                  ctx,
                                  out->span,
                                  "step.notes[]") &&
               ok;
        }
        lua_pop(L, 1);
      }
    }
    lua_pop(L, 1);
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

} // namespace app::doc

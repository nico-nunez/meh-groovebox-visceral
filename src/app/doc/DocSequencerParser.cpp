#include "app/doc/DocSequencerParser.h"
#include "app/doc/DocMetadata.h"

#include "lua/SequencerLuaParsing.h"

namespace app::doc {
namespace {

struct LuaSequencerParseContext {
  DocID documentID = 0;
  DocRevision revision = 0;
  AuthoredSeqDocModel model{};
  DocDiagnostics diagnostics{};
};

std::string trackTarget(uint8_t trackIndex, const char* suffix) {
  std::string target = "track:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  if (suffix && suffix[0] != '\0') {
    target += ".";
    target += suffix;
  }
  return target;
}

std::string patternSlotTarget(uint8_t trackIndex, uint8_t slot) {
  std::string target = "track:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  target += ".patterns[";
  target += std::to_string(static_cast<int>(slot) + 1);
  target += "]";
  return target;
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

bool parsePatternSlot(lua_State* L,
                      int patternIndex,
                      LuaSequencerParseContext& ctx,
                      AuthoredTrackSeqModel& track,
                      uint8_t slot) {
  app::sequencer::LanePattern pattern{};
  auto parseRes = lua::parseLuaLanePattern(L, patternIndex, pattern);
  if (!parseRes.ok) {
    const std::string target = patternSlotTarget(track.trackIndex, slot);
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerPatternInvalidShape,
                   parseRes.err,
                   track.trackSpan,
                   target.c_str());
    return false;
  }

  track.patterns[slot].occupied = true;
  track.patterns[slot].pattern = pattern;
  track.patterns[slot].slotSpan = track.trackSpan;
  return true;
}

uint8_t findFirstOccupiedSlot(const AuthoredTrackSeqModel& track) {
  for (uint8_t slot = 0; slot < sequencer::PATTERNS_PER_LANE; ++slot) {
    if (track.patterns[slot].occupied)
      return slot;
  }

  return sequencer::INVALID_PATTERN_SLOT;
}

bool activeSlotReferencesOccupiedPattern(const AuthoredTrackSeqModel& track) {
  if (track.activeSlot == sequencer::INVALID_PATTERN_SLOT)
    return true;

  if (track.activeSlot >= sequencer::PATTERNS_PER_LANE)
    return false;

  return track.patterns[track.activeSlot].occupied;
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
  track.activeSlotSource = ActivePatternSlotSource::Explicit;
  return true;
}

bool parsePatternsField(lua_State* L,
                        int settingsIndex,
                        LuaSequencerParseContext& ctx,
                        AuthoredTrackSeqModel& track,
                        bool& parsedValidPatternsTable) {
  parsedValidPatternsTable = false;

  const int absSettingsIndex = lua_absindex(L, settingsIndex);
  lua_getfield(L, absSettingsIndex, "patterns");
  if (!lua_istable(L, -1)) {
    const std::string target = trackTarget(track.trackIndex, "patterns");
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SequencerPatternsInvalidShape,
                   "patterns must be a table",
                   track.patternsSpan,
                   target.c_str());
    lua_pop(L, 1);
    return false;
  }

  parsedValidPatternsTable = true;
  const int patternsIndex = lua_absindex(L, -1);
  bool ok = true;

  lua_pushnil(L);
  while (lua_next(L, patternsIndex) != 0) {
    if (!lua_isinteger(L, -2)) {
      const std::string target = trackTarget(track.trackIndex, "patterns");
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternSlotInvalidKey,
                     "pattern slot key must be an integer",
                     track.patternsSpan,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const int slotNumber = static_cast<int>(lua_tointeger(L, -2));
    if (slotNumber < 1 || slotNumber > static_cast<int>(sequencer::PATTERNS_PER_LANE)) {
      const std::string target = trackTarget(track.trackIndex, "patterns");
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SequencerPatternSlotOutOfRange,
                     "pattern slot out of range",
                     track.patternsSpan,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const uint8_t slot = static_cast<uint8_t>(slotNumber - 1);
    if (!lua_isnil(L, -1))
      ok = parsePatternSlot(L, lua_absindex(L, -1), ctx, track, slot) && ok;
    lua_pop(L, 1);
  }

  lua_pop(L, 1);
  return ok;
}

void finalizeTrackNormalization(LuaSequencerParseContext& ctx,
                                AuthoredTrackSeqModel& track,
                                bool patternsFieldPresent,
                                bool parsedValidPatternsTable,
                                bool activeSlotFieldPresent) {
  if (!patternsFieldPresent) {
    track.activeSlot = sequencer::INVALID_PATTERN_SLOT;
    track.activeSlotSource = ActivePatternSlotSource::Unset;
    track.explicitlyAuthoredEmpty = true;

    if (activeSlotFieldPresent) {
      const std::string target = trackTarget(track.trackIndex, "activeSlot");
      pushDiagnostic(ctx,
                     DiagnosticSource::Normalizer,
                     docdiag::SequencerActiveSlotMissingPatterns,
                     "activeSlot requires populated patterns",
                     track.activeSlotSpan,
                     target.c_str());
    }
    return;
  }

  if (!parsedValidPatternsTable)
    return;

  const uint8_t firstSlot = findFirstOccupiedSlot(track);
  if (firstSlot == sequencer::INVALID_PATTERN_SLOT) {
    track.activeSlot = sequencer::INVALID_PATTERN_SLOT;
    track.activeSlotSource = ActivePatternSlotSource::Unset;
    track.explicitlyAuthoredEmpty = true;

    if (activeSlotFieldPresent) {
      const std::string target = trackTarget(track.trackIndex, "activeSlot");
      pushDiagnostic(ctx,
                     DiagnosticSource::Normalizer,
                     docdiag::SequencerActiveSlotMissingPatterns,
                     "activeSlot requires populated patterns",
                     track.activeSlotSpan,
                     target.c_str());
    }
    return;
  }

  track.explicitlyAuthoredEmpty = false;

  if (!activeSlotFieldPresent) {
    track.activeSlot = firstSlot;
    track.activeSlotSource = ActivePatternSlotSource::Inferred;
    return;
  }

  if (!activeSlotReferencesOccupiedPattern(track)) {
    const std::string target = trackTarget(track.trackIndex, "activeSlot");
    pushDiagnostic(ctx,
                   DiagnosticSource::Normalizer,
                   docdiag::SequencerActiveSlotEmptySlot,
                   "activeSlot points to an empty pattern slot",
                   track.activeSlotSpan,
                   target.c_str());
  }
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
  bool parsedValidPatternsTable = false;

  if (patternsFieldPresent)
    ok = parsePatternsField(L, absSettingsIndex, ctx, track, parsedValidPatternsTable) && ok;

  if (activeSlotFieldPresent)
    ok = parseActiveSlotField(L, absSettingsIndex, ctx, track) && ok;

  finalizeTrackNormalization(ctx,
                             track,
                             patternsFieldPresent,
                             parsedValidPatternsTable,
                             activeSlotFieldPresent);
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
  ctx->model.hasTrackState[trackIndex] = true;

  AuthoredTrackSeqModel& track = ctx->model.tracks[trackIndex];
  track = AuthoredTrackSeqModel{};
  track.trackIndex = trackIndex;
  track.trackSpan = trackSpan;
  track.patternsSpan = trackSpan;
  track.activeSlotSpan = trackSpan;

  parseTrackSettings(L, 2, *ctx, track);
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

void registerParserEnvironment(lua_State* L, LuaSequencerParseContext& ctx) {
  openParserLibraries(L);

  const DocFunctionMetadata* trackFunction = findAuthoredDocumentFunction(docglobal::Track);
  if (trackFunction && trackFunction->status == DocMetadataStatus::Implemented) {
    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, l_captureTrack, 1);
    lua_setglobal(L, trackFunction->name);
  }

  if (isAuthoredConstructor(docctor::TrackSettings))
    registerPlainConstructor(L, docctor::TrackSettings);
  if (isAuthoredConstructor(docctor::SynthSettings))
    registerPlainConstructor(L, docctor::SynthSettings);
  if (isAuthoredConstructor(docctor::MixerSettings))
    registerPlainConstructor(L, docctor::MixerSettings);
}

} // namespace

SequencerNormalizeResult parseAndNormalizeSequencerDocument(DocID documentID,
                                                            DocRevision revision,
                                                            const char* bufferText) {
  LuaSequencerParseContext ctx{};
  ctx.documentID = documentID;
  ctx.revision = revision;
  ctx.model.documentID = documentID;
  ctx.model.revision = revision;

  lua_State* L = luaL_newstate();
  if (!L) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Parser,
                   docdiag::DocumentLuaStateFailed,
                   "failed to create authoring Lua state",
                   SourceSpan{},
                   "");
    return {false, ctx.model, ctx.diagnostics};
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
  }

  lua_close(L);

  SequencerNormalizeResult result{};
  result.ok = ctx.diagnostics.empty();
  result.model = ctx.model;
  result.diagnostics = ctx.diagnostics;
  return result;
}

} // namespace app::doc

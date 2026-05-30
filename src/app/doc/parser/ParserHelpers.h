#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocTypes.h"
#include "lua/LuaState.h"

#include <cmath>

namespace app::doc {

struct LuaSequencerParseContext {
  DocID documentID = 0;
  DocRevision revision = 0;
  AuthoredDocModel* model = nullptr;
  DocDiagnostics diagnostics{};
};

inline bool tableHasField(lua_State* L, int tableIndex, const char* field) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, field);
  const bool present = !lua_isnil(L, -1);
  lua_pop(L, 1);
  return present;
}

inline bool readBoolField(lua_State* L, int tableIndex, const char* field, bool* out) {
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

inline bool readUInt7Field(lua_State* L, int tableIndex, const char* field, uint8_t* out) {
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

inline bool
readPositiveUInt8Field(lua_State* L, int tableIndex, const char* field, uint8_t max, uint8_t* out) {
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

inline bool finiteNumber(lua_State* L, int index) {
  return lua_isnumber(L, index) && std::isfinite(lua_tonumber(L, index));
}

inline SourceSpan currentLuaCallSpan(lua_State* L) {
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

inline void pushDiagnostic(LuaSequencerParseContext& ctx,
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

int l_captureMixer(lua_State* L);
bool parseMixerSettingsForTrack(lua_State* L,
                                int settingsIndex,
                                LuaSequencerParseContext& ctx,
                                uint8_t trackIndex,
                                SourceSpan span);

int l_captureSynth(lua_State* L);
bool parseSynthSettingsForTrack(lua_State* L,
                                int settingsIndex,
                                LuaSequencerParseContext& ctx,
                                uint8_t trackIndex,
                                SourceSpan span);

int l_captureTrack(lua_State* L);
bool parseTrackSettings(lua_State* L,
                        int settingsIndex,
                        LuaSequencerParseContext& ctx,
                        AuthoredTrackSeqModel& track);

} // namespace app::doc

#include "SequencerBindings.h"

#include "app/Sequencer.h"

#include "synth/params/ParamUtils.h"

namespace lua::bindings {

namespace {
namespace seq = app::sequencer;

// =========================
// Sequencer — begin (copy)
// =========================
int l_seqCopyPattern(lua_State* L) {
  auto* ctx = getLuaContext(L);
  if (!seq::beginPatternEdit(ctx->app->sequencer, true).ok) {
    luaL_error(L, "edit session already in progress");
    return CMD_FAILURE;
  }
  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Sequencer — begin (new)
// =========================
int l_seqNewPattern(lua_State* L) {
  auto* ctx = getLuaContext(L);
  if (!seq::beginPatternEdit(ctx->app->sequencer, false).ok) {
    luaL_error(L, "edit session already in progress");
    return CMD_FAILURE;
  }
  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Sequencer — commit
// =========================
int l_seqCommit(lua_State* L) {
  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::commitPattern(ctx->app->sequencer));
  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Sequencer — setStep
// =========================
int l_seqSetStep(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  uint8_t step = (uint8_t)luaL_checkinteger(L, 2);
  luaL_checktype(L, 3, LUA_TTABLE);

  seq::StepEvent evt{};

  lua_getfield(L, 3, "active");
  if (!lua_isnil(L, -1))
    evt.active = lua_toboolean(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, 3, "noteOn");
  if (!lua_isnil(L, -1))
    evt.noteOn = lua_toboolean(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, 3, "note");
  if (!lua_isnil(L, -1))
    evt.note = (uint8_t)lua_tointeger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, 3, "velocity");
  if (!lua_isnil(L, -1))
    evt.velocity = (uint8_t)lua_tointeger(L, -1);
  lua_pop(L, 1);

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setStep(ctx->app->sequencer, lane, step, evt));
  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Sequencer — per-field
// =========================
int l_seqSetStepActive(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  uint8_t step = (uint8_t)luaL_checkinteger(L, 2);
  bool active = lua_toboolean(L, 3);
  auto* ctx = getLuaContext(L);

  CMD_CHECK(seq::setStepActive(ctx->app->sequencer, lane, step, active));
}

int l_seqSetStepNote(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  uint8_t step = (uint8_t)luaL_checkinteger(L, 2);
  uint8_t note = (uint8_t)luaL_checkinteger(L, 3);
  auto* ctx = getLuaContext(L);

  CMD_CHECK(seq::setStepNote(ctx->app->sequencer, lane, step, note));
}

int l_seqSetStepVelocity(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  uint8_t step = (uint8_t)luaL_checkinteger(L, 2);
  uint8_t velocity = (uint8_t)luaL_checkinteger(L, 3);
  auto* ctx = getLuaContext(L);

  CMD_CHECK(seq::setStepVelocity(ctx->app->sequencer, lane, step, velocity));
}

int l_seqSetStepNoteOn(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  uint8_t step = (uint8_t)luaL_checkinteger(L, 2);
  bool noteOn = lua_toboolean(L, 3);
  auto* ctx = getLuaContext(L);

  CMD_CHECK(seq::setStepNoteOn(ctx->app->sequencer, lane, step, noteOn));
}

// =========================
// Sequencer — p-locks
// =========================
int l_seqSetStepLock(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  uint8_t step = (uint8_t)luaL_checkinteger(L, 2);
  const char* paramName = luaL_checkstring(L, 3);
  float value = (float)luaL_checknumber(L, 4);

  auto paramID = synth::param::utils::getParamIDByName(paramName);
  if (paramID == synth::param::PARAM_COUNT) {
    luaL_error(L, "unknown param: %s", paramName);
    return CMD_BAD_INPUT;
  }

  auto* ctx = getLuaContext(L);
  CMD_CHECK(
      seq::setStepLock(ctx->app->sequencer, lane, step, static_cast<uint8_t>(paramID), value));
}

int l_seqClearStepLock(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  uint8_t step = (uint8_t)luaL_checkinteger(L, 2);
  const char* paramName = luaL_checkstring(L, 3);

  auto paramID = synth::param::utils::getParamIDByName(paramName);
  if (paramID == synth::param::PARAM_COUNT) {
    luaL_error(L, "unknown param: %s", paramName);
    return CMD_BAD_INPUT;
  }

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::clearStepLock(ctx->app->sequencer, lane, step, static_cast<uint8_t>(paramID)));
}

int l_seqClearStepLocks(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  uint8_t step = (uint8_t)luaL_checkinteger(L, 2);
  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::clearStepLocks(ctx->app->sequencer, lane, step));
}

// =========================
// Sequencer — bulk
// =========================

// Reads a Lua table of integers from the stack at index `stackIdx` into `out`.
// Returns the number of entries read, or -1 on error.
int readUint8Table(lua_State* L, int stackIdx, uint8_t* out, uint8_t maxCount) {
  if (!lua_istable(L, stackIdx))
    return -1;

  int n = (int)lua_rawlen(L, stackIdx);
  if (n > maxCount)
    return -1;

  for (int i = 0; i < n; ++i) {
    lua_rawgeti(L, stackIdx, i + 1);
    out[i] = (uint8_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
  }
  return n;
}

int l_seqSetActive(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  uint8_t values[seq::MAX_PATTERN_STEPS];
  int count = readUint8Table(L, 2, values, seq::MAX_PATTERN_STEPS);
  if (count < 0) {
    luaL_error(L, "setActive: invalid table");
    return CMD_FAILURE;
  }

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setActivePattern(ctx->app->sequencer, lane, values, (uint8_t)count));
}

int l_seqSetNotes(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  uint8_t values[seq::MAX_PATTERN_STEPS];
  int count = readUint8Table(L, 2, values, seq::MAX_PATTERN_STEPS);
  if (count < 0) {
    luaL_error(L, "setNotes: invalid table");
    return CMD_FAILURE;
  }

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setNotePattern(ctx->app->sequencer, lane, values, (uint8_t)count));
}

int l_seqSetVelocities(lua_State* L) {
  uint8_t lane = (uint8_t)luaL_checkinteger(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  uint8_t values[seq::MAX_PATTERN_STEPS];
  int count = readUint8Table(L, 2, values, seq::MAX_PATTERN_STEPS);
  if (count < 0) {
    luaL_error(L, "setVelocities: invalid table");
    return CMD_FAILURE;
  }

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setVelocityPattern(ctx->app->sequencer, lane, values, (uint8_t)count));
}

} // namespace

// =========================
// Registration
// =========================
void registerSeqCommands(lua_State* L) {
  lua_newtable(L);

  registerFunctionCmd(L, l_seqCopyPattern, "copyPattern");
  registerFunctionCmd(L, l_seqNewPattern, "newPattern");
  registerFunctionCmd(L, l_seqCommit, "commit");
  registerFunctionCmd(L, l_seqSetStep, "setStep");
  registerFunctionCmd(L, l_seqSetStepActive, "setStepActive");
  registerFunctionCmd(L, l_seqSetStepNote, "setStepNote");
  registerFunctionCmd(L, l_seqSetStepVelocity, "setStepVelocity");
  registerFunctionCmd(L, l_seqSetStepNoteOn, "setStepNoteOn");
  registerFunctionCmd(L, l_seqSetStepLock, "setStepLock");
  registerFunctionCmd(L, l_seqClearStepLock, "clearStepLock");
  registerFunctionCmd(L, l_seqClearStepLocks, "clearStepLocks");
  registerFunctionCmd(L, l_seqSetActive, "setActive");
  registerFunctionCmd(L, l_seqSetNotes, "setNotes");
  registerFunctionCmd(L, l_seqSetVelocities, "setVelocities");

  lua_setglobal(L, "seq");
  addVisibleGlobal("seq");
}
} // namespace lua::bindings

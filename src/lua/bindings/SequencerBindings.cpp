#include "lua/bindings/LuaBindings.h"

#include "app/Sequencer.h"
#include "app/Types.h"

#include "synth/params/ParamUtils.h"

#include <cassert>
#include <cstdint>

namespace lua::bindings {

static constexpr const char* SEQ_TRACK_METATABLE = "gb.seq.track";
static constexpr const char* SEQ_STEP_METATABLE = "gb.seq.step";

namespace {
using app::VoidResult;
namespace evt = app::events;
namespace seq = app::sequencer;
namespace sp = synth::param;

struct LuaSeqTrackRef {
  uint8_t lane = 0;
};

struct LuaSeqStepRef {
  uint8_t lane = 0;
  uint8_t step = 0;
};

// ==============
// Helpers
// ==============
uint8_t luaTrackToLane(lua_State* L, int argIndex) {
  int track = (int)luaL_checkinteger(L, argIndex);
  if (track < 1 || track > (int)app::MAX_TRACKS)
    luaL_error(L, "track out of range (1-%d)", (int)app::MAX_TRACKS);
  return (uint8_t)(track - 1);
}

uint8_t luaStepToIndex(lua_State* L, int argIndex) {
  int step = (int)luaL_checkinteger(L, argIndex);
  if (step < 1 || step > (int)seq::MAX_PATTERN_STEPS)
    luaL_error(L, "step out of range (1-%d)", (int)seq::MAX_PATTERN_STEPS);
  return (uint8_t)(step - 1);
}

LuaSeqTrackRef* pushSeqTrackRef(lua_State* L, uint8_t lane) {
  auto* ref = static_cast<LuaSeqTrackRef*>(lua_newuserdatauv(L, sizeof(LuaSeqTrackRef), 0));
  ref->lane = lane;
  luaL_getmetatable(L, SEQ_TRACK_METATABLE);
  lua_setmetatable(L, -2);
  return ref;
}

LuaSeqTrackRef* checkSeqTrackRef(lua_State* L, int index) {
  return static_cast<LuaSeqTrackRef*>(luaL_checkudata(L, index, SEQ_TRACK_METATABLE));
}

LuaSeqStepRef* pushSeqStepRef(lua_State* L, uint8_t lane, uint8_t step) {
  auto* ref = static_cast<LuaSeqStepRef*>(lua_newuserdatauv(L, sizeof(LuaSeqStepRef), 0));
  ref->lane = lane;
  ref->step = step;
  luaL_getmetatable(L, SEQ_STEP_METATABLE);
  lua_setmetatable(L, -2);
  return ref;
}

LuaSeqStepRef* checkSeqStepRef(lua_State* L, int index) {
  return static_cast<LuaSeqStepRef*>(luaL_checkudata(L, index, SEQ_STEP_METATABLE));
}

uint8_t getSeqTrackLane(lua_State* L, int index) {
  return checkSeqTrackRef(L, index)->lane;
}

LuaSeqStepRef getSeqStepRef(lua_State* L, int index) {
  return *checkSeqStepRef(L, index);
}

VoidResult parseLuaStepEvent(lua_State* L, int index, seq::StepEvent& outEvt) {
  luaL_checktype(L, index, LUA_TTABLE);

  lua_getfield(L, index, "active");
  if (!lua_isnil(L, -1)) {
    outEvt.active = lua_toboolean(L, -1);
    outEvt.noteOn = lua_toboolean(L, -1);
  }
  lua_pop(L, 1);

  // NOTE: redundant???
  // lua_getfield(L, index, "noteOn");
  // if (!lua_isnil(L, -1))
  //   outEvt.noteOn = lua_toboolean(L, -1);
  // lua_pop(L, 1);

  lua_getfield(L, index, "note");
  if (!lua_isnil(L, -1)) {
    int note = (int)luaL_checkinteger(L, -1);
    if (note < 0 || note > 127)
      return {false, "note out of range"};
    outEvt.note = (uint8_t)note;
  }
  lua_pop(L, 1);

  lua_getfield(L, index, "velocity");
  if (!lua_isnil(L, -1)) {
    int velocity = (int)luaL_checkinteger(L, -1);
    if (velocity < 0 || velocity > 127)
      return {false, "velocity out of range"};
    outEvt.velocity = (uint8_t)velocity;
  }
  lua_pop(L, 1);

  lua_getfield(L, index, "gate");
  if (!lua_isnil(L, -1)) {
    float gate = (float)luaL_checknumber(L, -1);
    if (!std::isfinite(gate) || gate < 0.0f)
      return {false, "gate out of range"};
    outEvt.gate = gate;
  }
  lua_pop(L, 1);

  lua_getfield(L, index, "legato");
  if (!lua_isnil(L, -1))
    outEvt.legato = lua_toboolean(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, index, "locks");
  if (!lua_isnil(L, -1)) {
    luaL_checktype(L, -1, LUA_TTABLE);
    int numLocks = (int)lua_rawlen(L, -1);
    if (numLocks > (int)seq::MAX_LOCKS_PER_STEP)
      return {false, "too many step locks"};

    outEvt.numLocks = 0;
    for (int i = 0; i < numLocks; ++i) {
      lua_rawgeti(L, -1, i + 1);
      luaL_checktype(L, -1, LUA_TTABLE);

      lua_getfield(L, -1, "param");
      const char* paramName = luaL_checkstring(L, -1);
      auto paramID = sp::utils::getParamIDByName(paramName);
      lua_pop(L, 1);
      if (paramID == sp::ParamID::PARAM_UNKNOWN)
        return {false, "unknown param"};

      lua_getfield(L, -1, "value");
      float value = (float)luaL_checknumber(L, -1);
      lua_pop(L, 1);

      for (uint8_t l = 0; l < outEvt.numLocks; ++l) {
        if (outEvt.locks[l].paramID == (uint8_t)paramID)
          return {false, "duplicate lock param"};
      }

      outEvt.locks[outEvt.numLocks++] = {static_cast<uint8_t>(paramID), value};
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  return {true, nullptr};
}

void pushStepEvent(lua_State* L, const seq::StepEvent& evt) {
  lua_newtable(L);

  lua_pushboolean(L, evt.active);
  lua_setfield(L, -2, "active");

  lua_pushboolean(L, evt.noteOn);
  lua_setfield(L, -2, "noteOn");

  lua_pushinteger(L, evt.note);
  lua_setfield(L, -2, "note");

  lua_pushinteger(L, evt.velocity);
  lua_setfield(L, -2, "velocity");

  lua_pushnumber(L, evt.gate);
  lua_setfield(L, -2, "gate");

  lua_pushboolean(L, evt.legato);
  lua_setfield(L, -2, "legato");

  lua_newtable(L); // locks
  for (uint8_t i = 0; i < evt.numLocks; ++i) {
    lua_newtable(L); // loock

    lua_pushstring(L, synth::param::PARAM_DEFS[evt.locks[i].paramID].name);
    lua_setfield(L, -2, "param");

    lua_pushnumber(L, evt.locks[i].value);
    lua_setfield(L, -2, "value");

    lua_rawseti(L, -2, i + 1);
  }
  lua_setfield(L, -2, "locks");
}

void pushLanePattern(lua_State* L, const seq::LanePattern& pattern) {
  lua_newtable(L);

  lua_pushinteger(L, pattern.numSteps);
  lua_setfield(L, -2, "numSteps");

  lua_pushinteger(L, pattern.stepsPerBeat);
  lua_setfield(L, -2, "stepsPerBeat");

  lua_newtable(L);
  for (uint32_t i = 0; i < pattern.numSteps; ++i) {
    pushStepEvent(L, pattern.steps[i]);
    lua_rawseti(L, -2, (int)i + 1);
  }
  lua_setfield(L, -2, "steps");
}

// =====================
// Sequencer methods
// =====================

// seq.track(trackIndex)
int l_seqTrack(lua_State* L) {
  if (lua_gettop(L) == 0) {
    auto* ctx = getLuaContext(L);
    lua_pushinteger(L, ctx->currentTrack + 1);
  }

  uint8_t lane = luaTrackToLane(L, 1);
  pushSeqTrackRef(L, lane);
  return 1;
}

int l_seqListTracks(lua_State* L) {
  auto* ctx = getLuaContext(L);
  uint8_t cur = ctx->currentTrack; // use Lua shadow for * marker

  printf("trk  gain   pan    mute\n");
  for (int i = 0; i < (int)seq::MAX_LANES; ++i) {
    const auto& t = ctx->app->mixer.tracks[i];
    printf("  %d%c %.2f  %+.2f   %s\n",
           i + 1,
           (i == (int)cur) ? '*' : ' ',
           t.gain,
           t.pan,
           t.enabled ? "off" : "MUTE");
  }
  return CMD_SUCCESS;
}

int l_seqSelectTrack(lua_State* L) {
  int track = (int)luaL_checkinteger(L, 1);
  auto* ctx = getLuaContext(L);

  if (track < 1 || track > (int)app::MAX_TRACKS)
    return luaL_error(L, "track %d out of range (1–%d)", track, (int)app::MAX_TRACKS);

  uint8_t idx = (uint8_t)(track - 1);

  // Update Lua-side shadow immediately so subsequent Lua commands target
  // the new track without waiting for the audio callback to drain the queue.
  ctx->currentTrack = idx;

  auto evt = evt::createCurrentTrackEvent(idx);
  if (!pushControlEvent(ctx->app, evt).ok)
    return luaL_error(L, "control queue full");

  // Display reads mixer state directly — one block behind is fine for a print.
  const auto& t = ctx->app->mixer.tracks[idx];
  printf("[track %d]  gain: %.2f  pan: %+.2f  mute: %s\n",
         track,
         t.gain,
         t.pan,
         t.enabled ? "off" : "MUTE");

  return CMD_SUCCESS;
}

// =========================
// Track Methods
// =========================

// track.step(stepIndex)
int l_seqTrackSelectStep(lua_State* L) {
  CHECK_ARG_COUNT(1);
  uint8_t lane = (uint8_t)lua_tointeger(L, lua_upvalueindex(1));
  uint8_t step = luaStepToIndex(L, 1);
  pushSeqStepRef(L, lane, step);
  return 1;
}

// internal lookup for track:<method>(...) and track.step(...)
int l_seqTrackIndex(lua_State* L) {
  CHECK_ARG_COUNT(1 + 1);
  auto* ref = checkSeqTrackRef(L, 1);
  const char* key = luaL_checkstring(L, 2);

  if (strcmp(key, "step") == 0) {
    lua_pushinteger(L, ref->lane);
    lua_pushcclosure(L, l_seqTrackSelectStep, 1);
    return 1;
  }

  luaL_getmetatable(L, SEQ_TRACK_METATABLE);
  lua_getfield(L, -1, "__methods");
  lua_getfield(L, -1, key);
  if (lua_isnil(L, -1))
    return luaL_error(L, "unknown seq.track method '%s'", key);

  lua_replace(L, 1);
  lua_settop(L, 1);
  return 1;
}

// track:setNumSteps(n)
int l_seqTrackSetNumSteps(lua_State* L) {
  CHECK_ARG_COUNT(2);
  uint8_t lane = getSeqTrackLane(L, 1);
  int numSteps = (int)luaL_checkinteger(L, 2);

  if (numSteps < 1 || numSteps > (int)seq::MAX_PATTERN_STEPS)
    return luaL_error(L, "numSteps out of range");

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setPatternNumSteps(ctx->app->sequencer, lane, (uint8_t)numSteps));
}

// track:setStepsPerBeat(n)
int l_seqTrackSetStepsPerBeat(lua_State* L) {
  CHECK_ARG_COUNT(2);
  uint8_t lane = getSeqTrackLane(L, 1);
  int stepsPerBeat = (int)luaL_checkinteger(L, 2);

  if (stepsPerBeat < 1 || stepsPerBeat > (int)seq::MAX_STEPS_PER_BEAT)
    return luaL_error(L, "stepsPerBeat out of range");

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setPatternStepsPerBeat(ctx->app->sequencer, lane, (uint8_t)stepsPerBeat));
}

// track:getPattern()
int l_seqTrackGetPattern(lua_State* L) {
  CHECK_ARG_COUNT(1);
  uint8_t lane = getSeqTrackLane(L, 1);
  auto* ctx = getLuaContext(L);

  auto pattern = seq::getActivePattern(ctx->app->sequencer, lane);
  if (!pattern.ok) {
    luaL_error(L, pattern.err);
    return CMD_FAILURE;
  }

  pushLanePattern(L, *pattern.value);
  return 1;
}

// track:clear()
int l_seqTrackClear(lua_State* L) {
  CHECK_ARG_COUNT(1);
  uint8_t lane = getSeqTrackLane(L, 1);
  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::clearTrack(ctx->app->sequencer, lane));
}

// track:resetPattern()
int l_seqTrackResetPattern(lua_State* L) {
  CHECK_ARG_COUNT(1);
  uint8_t lane = getSeqTrackLane(L, 1);
  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::clearPattern(ctx->app->sequencer, lane));
}

// ==============================
// Seq -> Track -> Step methods
// ==============================

// step:set({...})
int l_seqStepSet(lua_State* L) {
  CHECK_ARG_COUNT(2);
  auto ref = getSeqStepRef(L, 1);
  seq::StepEvent evt{};

  auto parseRes = parseLuaStepEvent(L, 2, evt);
  if (!parseRes.ok)
    return luaL_error(L, "%s", parseRes.err);

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setStep(ctx->app->sequencer, ref.lane, ref.step, evt));
}

// step:setActive(?bool)
int l_seqStepSetActive(lua_State* L) {
  int numArgs = lua_gettop(L);
  auto ref = getSeqStepRef(L, 1);

  auto* ctx = getLuaContext(L);
  bool active = numArgs > 1
                    ? lua_toboolean(L, 2)
                    : !ctx->app->sequencer.store.buffers->lanes[ref.lane].steps[ref.step].active;

  CMD_CHECK(seq::setStepActive(ctx->app->sequencer, ref.lane, ref.step, active));
}

// step:setNote(uint8)
int l_seqStepSetNote(lua_State* L) {
  CHECK_ARG_COUNT(2);
  auto ref = getSeqStepRef(L, 1);
  auto note = (uint8_t)luaL_checkinteger(L, 2);

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setStepNote(ctx->app->sequencer, ref.lane, ref.step, note));
}

// step:setVelocity(uint8)
int l_seqStepSetVelocity(lua_State* L) {
  CHECK_ARG_COUNT(2);
  auto ref = getSeqStepRef(L, 1);
  auto velocity = (uint8_t)luaL_checkinteger(L, 2);

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setStepVelocity(ctx->app->sequencer, ref.lane, ref.step, velocity));
}

// step:setNoteOn(?bool)
int l_seqStepSetNoteOn(lua_State* L) {
  int numArgs = lua_gettop(L);
  auto ref = getSeqStepRef(L, 1);

  auto* ctx = getLuaContext(L);
  bool noteOn = numArgs > 1
                    ? lua_toboolean(L, 2)
                    : !ctx->app->sequencer.store.buffers->lanes[ref.lane].steps[ref.step].noteOn;

  CMD_CHECK(seq::setStepNoteOn(ctx->app->sequencer, ref.lane, ref.step, noteOn));
}

// step:setGate(gate)
int l_seqStepSetGate(lua_State* L) {
  CHECK_ARG_COUNT(2);
  auto ref = getSeqStepRef(L, 1);
  float gate = (float)luaL_checknumber(L, 2);

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setStepGate(ctx->app->sequencer, ref.lane, ref.step, gate));
}

// step:setLegato(?bool)
int l_seqStepSetLegato(lua_State* L) {
  int numArgs = lua_gettop(L);
  auto ref = getSeqStepRef(L, 1);

  auto* ctx = getLuaContext(L);
  bool legato = numArgs > 1
                    ? lua_toboolean(L, 2)
                    : !ctx->app->sequencer.store.buffers->lanes[ref.lane].steps[ref.step].legato;

  CMD_CHECK(seq::setStepLegato(ctx->app->sequencer, ref.lane, ref.step, legato));
}

// step:setLock(paramName, value)
int l_seqStepSetLock(lua_State* L) {
  CHECK_ARG_COUNT(3);
  auto ref = getSeqStepRef(L, 1);
  const char* paramName = luaL_checkstring(L, 2);
  float value = (float)luaL_checknumber(L, 3);

  auto paramID = sp::utils::getParamIDByName(paramName);
  if (paramID == sp::ParamID::PARAM_UNKNOWN)
    return luaL_error(L, "unknown param: %s", paramName);

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::setStepLock(ctx->app->sequencer,
                             ref.lane,
                             ref.step,
                             static_cast<uint8_t>(paramID),
                             value));
}

// step:clearLock(paramName)
int l_seqStepClearLock(lua_State* L) {
  CHECK_ARG_COUNT(2);
  auto ref = getSeqStepRef(L, 1);
  const char* paramName = luaL_checkstring(L, 2);

  auto paramID = sp::utils::getParamIDByName(paramName);
  if (paramID == sp::PARAM_COUNT) {
    luaL_error(L, "unknown param: %s", paramName);
    return CMD_BAD_INPUT;
  }

  auto* ctx = getLuaContext(L);
  CMD_CHECK(
      seq::clearStepLock(ctx->app->sequencer, ref.lane, ref.step, static_cast<uint8_t>(paramID)));
}

int l_seqStepClearLocks(lua_State* L) {
  CHECK_ARG_COUNT(1);
  auto ref = getSeqStepRef(L, 1);

  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::clearStepLocks(ctx->app->sequencer, ref.lane, ref.step));
}

// st:get()
int l_seqStepGet(lua_State* L) {
  CHECK_ARG_COUNT(1);
  auto ref = getSeqStepRef(L, 1);
  auto* ctx = getLuaContext(L);

  auto evt = seq::getStep(ctx->app->sequencer, ref.lane, ref.step);
  if (!evt.ok) {
    luaL_error(L, evt.err);
    return CMD_FAILURE;
  }

  pushStepEvent(L, *evt.value);
  return 1;
}

// st:clear()
int l_seqStepClear(lua_State* L) {
  CHECK_ARG_COUNT(1);
  auto ref = getSeqStepRef(L, 1);
  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::clearStep(ctx->app->sequencer, ref.lane, ref.step));
}

// ===============================
// Sequencer -> Track — bulk edit
// ===============================

// Reads a Lua table of integers from the stack at index `stackIdx` into `out`.
// Returns the number of entries read, or -1 on error.
int readUint8Table(lua_State* L, int stackIdx, uint8_t* out, uint8_t fillCount, uint8_t maxCount) {
  if (!lua_istable(L, stackIdx))
    return -1;

  int n = (int)lua_rawlen(L, stackIdx);
  if (n > fillCount || fillCount > maxCount)
    return -1;

  for (int i = 0; i < fillCount; ++i) {
    lua_rawgeti(L, stackIdx, ((i % n) + 1));
    out[i] = (uint8_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
  }
  return fillCount;
}

// seq.editPattern() (aka copy)
int l_seqEditPattern(lua_State* L) {
  CHECK_ARG_COUNT(0);
  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::beginPatternEdit(ctx->app->sequencer, true));
}

// seq.newPattern();
int l_seqNewPattern(lua_State* L) {
  CHECK_ARG_COUNT(0);
  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::beginPatternEdit(ctx->app->sequencer, false));
}

// seq.commitPattern();
int l_seqCommitPattern(lua_State* L) {
  CHECK_ARG_COUNT(0);
  auto* ctx = getLuaContext(L);
  CMD_CHECK(seq::commitPattern(ctx->app->sequencer));
}

// track:setActiveSteps({...})
int l_seqTrackSetActiveSteps(lua_State* L) {
  CHECK_ARG_COUNT(2);
  auto lane = getSeqTrackLane(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  auto* ctx = getLuaContext(L);
  auto patternRes = seq::getPendingPattern(ctx->app->sequencer, lane);
  if (!patternRes.ok) {
    luaL_error(L, patternRes.err);
    return CMD_FAILURE;
  }

  uint8_t values[seq::MAX_PATTERN_STEPS];
  int count = readUint8Table(L, 2, values, patternRes.value->numSteps, seq::MAX_PATTERN_STEPS);
  if (count < 0) {
    luaL_error(L, "setActive: invalid table");
    return CMD_FAILURE;
  }

  CMD_CHECK(seq::setActivePattern(ctx->app->sequencer, lane, values, (uint8_t)count));
}

// track:setNotes({...});
int l_seqTrackSetNotes(lua_State* L) {
  CHECK_ARG_COUNT(2);
  auto lane = getSeqTrackLane(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  auto* ctx = getLuaContext(L);
  auto patternRes = seq::getPendingPattern(ctx->app->sequencer, lane);
  if (!patternRes.ok) {
    luaL_error(L, patternRes.err);
    CMD_FAILURE;
  }

  uint8_t values[seq::MAX_PATTERN_STEPS];
  int count = readUint8Table(L, 2, values, patternRes.value->numSteps, seq::MAX_PATTERN_STEPS);
  if (count < 0) {
    luaL_error(L, "setNotes: invalid table");
    return CMD_FAILURE;
  }
  CMD_CHECK(seq::setNotePattern(ctx->app->sequencer, lane, values, (uint8_t)count));
}

// track:setVelocities({...});
int l_seqTrackSetVelocities(lua_State* L) {
  CHECK_ARG_COUNT(2);
  auto lane = getSeqTrackLane(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  auto* ctx = getLuaContext(L);
  auto patternRes = seq::getPendingPattern(ctx->app->sequencer, lane);
  if (!patternRes.ok)
    return luaL_error(L, patternRes.err);

  uint8_t values[seq::MAX_PATTERN_STEPS];

  int count = readUint8Table(L, 2, values, patternRes.value->numSteps, seq::MAX_PATTERN_STEPS);
  if (count < 0) {
    luaL_error(L, "setActive: invalid table");
    return CMD_FAILURE;
  }

  CMD_CHECK(seq::setVelocityPattern(ctx->app->sequencer, lane, values, (uint8_t)count));
}

} // namespace

// =========================
// Registration
// =========================
void registerSeqTrackType(lua_State* L) {
  luaL_newmetatable(L, SEQ_TRACK_METATABLE);

  lua_pushcfunction(L, l_seqTrackIndex);
  lua_setfield(L, -2, "__index");

  lua_newtable(L);
  registerFunction(L, l_seqTrackSetNumSteps, "setNumSteps");
  registerFunction(L, l_seqTrackSetStepsPerBeat, "setStepsPerBeat");

  registerFunction(L, l_seqTrackGetPattern, "getPattern");
  registerFunction(L, l_seqTrackSetActiveSteps, "setPattern");
  registerFunction(L, l_seqTrackSetNotes, "setNotes");
  registerFunction(L, l_seqTrackSetVelocities, "setVelocities");
  registerFunction(L, l_seqTrackResetPattern, "resetPattern");

  registerFunction(L, l_seqTrackClear, "clear");

  lua_setfield(L, -2, "__methods");

  lua_pop(L, 1);
}

void registerSeqStepType(lua_State* L) {
  luaL_newmetatable(L, SEQ_STEP_METATABLE);

  lua_newtable(L);
  registerFunction(L, l_seqStepGet, "get");
  registerFunction(L, l_seqStepSet, "set");
  registerFunction(L, l_seqStepClear, "clear");

  registerFunction(L, l_seqStepSetActive, "setActive");
  registerFunction(L, l_seqStepSetNoteOn, "setNoteOn");
  registerFunction(L, l_seqStepSetNote, "setNote");
  registerFunction(L, l_seqStepSetVelocity, "setVelocity");
  registerFunction(L, l_seqStepSetGate, "setGate");
  registerFunction(L, l_seqStepSetLegato, "setLegato");
  registerFunction(L, l_seqStepSetLock, "setLock");
  registerFunction(L, l_seqStepClearLock, "clearLock");
  registerFunction(L, l_seqStepClearLocks, "clearLocks");

  lua_setfield(L, -2, "__index");

  lua_pop(L, 1);
}

void registerSeqCommands(lua_State* L) {
  registerSeqTrackType(L);
  registerSeqStepType(L);

  lua_newtable(L);
  registerFunction(L, l_seqTrack, "track");
  registerFunction(L, l_seqListTracks, "listTracks");
  registerFunction(L, l_seqSelectTrack, "selectTrack");

  registerFunction(L, l_seqEditPattern, "edit");
  registerFunction(L, l_seqNewPattern, "new");
  registerFunction(L, l_seqCommitPattern, "commit");

  lua_setglobal(L, "seq");
  addVisibleGlobal("seq");
}

} // namespace lua::bindings

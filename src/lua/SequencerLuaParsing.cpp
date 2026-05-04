#include "lua/SequencerLuaParsing.h"

#include "synth/params/ParamUtils.h"

#include <cmath>
#include <cstdint>

namespace lua {
namespace {

using app::VoidResult;
namespace seq = app::sequencer;
namespace sp = synth::param;

bool tableFieldIsNil(lua_State* L, int tableIndex, const char* field) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, field);
  const bool isNil = lua_isnil(L, -1);
  lua_pop(L, 1);
  return isNil;
}

VoidResult readOptionalBoolField(lua_State* L, int tableIndex, const char* field, bool& out) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, field);
  if (!lua_isnil(L, -1)) {
    if (!lua_isboolean(L, -1)) {
      lua_pop(L, 1);
      return {false, "boolean field has invalid type"};
    }
    out = lua_toboolean(L, -1) != 0;
  }
  lua_pop(L, 1);
  return {true, nullptr};
}

VoidResult readOptionalUInt7Field(lua_State* L, int tableIndex, const char* field, uint8_t& out) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, field);
  if (!lua_isnil(L, -1)) {
    if (!lua_isinteger(L, -1)) {
      lua_pop(L, 1);
      return {false, "integer field has invalid type"};
    }

    const int value = static_cast<int>(lua_tointeger(L, -1));
    if (value < 0 || value > 127) {
      lua_pop(L, 1);
      return {false, "integer field out of range"};
    }

    out = static_cast<uint8_t>(value);
  }
  lua_pop(L, 1);
  return {true, nullptr};
}

VoidResult readOptionalGateField(lua_State* L, int tableIndex, float& out) {
  const int absTable = lua_absindex(L, tableIndex);
  lua_getfield(L, absTable, "gate");
  if (!lua_isnil(L, -1)) {
    if (!lua_isnumber(L, -1)) {
      lua_pop(L, 1);
      return {false, "gate out of range"};
    }

    const float value = static_cast<float>(lua_tonumber(L, -1));
    if (!std::isfinite(value) || value < 0.0f) {
      lua_pop(L, 1);
      return {false, "gate out of range"};
    }

    out = value;
  }
  lua_pop(L, 1);
  return {true, nullptr};
}

} // namespace

VoidResult parseLuaStepEvent(lua_State* L, int index, seq::StepEvent& outEvent) {
  if (!lua_istable(L, index))
    return {false, "step must be a table"};

  const int stepIndex = lua_absindex(L, index);

  if (!tableFieldIsNil(L, stepIndex, "active")) {
    bool active = false;
    auto activeRes = readOptionalBoolField(L, stepIndex, "active", active);
    if (!activeRes.ok)
      return {false, "active must be boolean"};
    outEvent.active = active;
    outEvent.noteOn = active;
  }

  auto noteRes = readOptionalUInt7Field(L, stepIndex, "note", outEvent.note);
  if (!noteRes.ok)
    return {false, "note out of range"};

  auto velocityRes = readOptionalUInt7Field(L, stepIndex, "velocity", outEvent.velocity);
  if (!velocityRes.ok)
    return {false, "velocity out of range"};

  auto gateRes = readOptionalGateField(L, stepIndex, outEvent.gate);
  if (!gateRes.ok)
    return gateRes;

  auto legatoRes = readOptionalBoolField(L, stepIndex, "legato", outEvent.legato);
  if (!legatoRes.ok)
    return {false, "legato must be boolean"};

  lua_getfield(L, stepIndex, "locks");
  if (!lua_isnil(L, -1)) {
    if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      return {false, "locks must be a table"};
    }

    const int locksIndex = lua_absindex(L, -1);
    const int numLocks = static_cast<int>(lua_rawlen(L, locksIndex));
    if (numLocks > static_cast<int>(seq::MAX_LOCKS_PER_STEP)) {
      lua_pop(L, 1);
      return {false, "too many step locks"};
    }

    outEvent.numLocks = 0;
    for (int i = 0; i < numLocks; ++i) {
      lua_rawgeti(L, locksIndex, i + 1);
      if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return {false, "lock must be a table"};
      }

      const int lockIndex = lua_absindex(L, -1);
      lua_getfield(L, lockIndex, "param");
      if (!lua_isstring(L, -1)) {
        lua_pop(L, 3);
        return {false, "unknown param"};
      }
      const char* paramName = lua_tostring(L, -1);
      auto paramID = sp::utils::getParamIDByName(paramName);
      lua_pop(L, 1);
      if (paramID == sp::ParamID::PARAM_UNKNOWN) {
        lua_pop(L, 2);
        return {false, "unknown param"};
      }

      lua_getfield(L, lockIndex, "value");
      if (!lua_isnumber(L, -1)) {
        lua_pop(L, 3);
        return {false, "lock value must be numeric"};
      }
      const float value = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);

      for (uint8_t existing = 0; existing < outEvent.numLocks; ++existing) {
        if (outEvent.locks[existing].paramID == static_cast<uint8_t>(paramID)) {
          lua_pop(L, 2);
          return {false, "duplicate lock param"};
        }
      }

      outEvent.locks[outEvent.numLocks++] = {static_cast<uint8_t>(paramID), value};
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  return {true, nullptr};
}

VoidResult parseLuaLanePattern(lua_State* L, int index, seq::LanePattern& outPattern) {
  if (!lua_istable(L, index))
    return {false, "pattern must be a table"};

  const int patternIndex = lua_absindex(L, index);

  lua_getfield(L, patternIndex, "numSteps");
  if (!lua_isinteger(L, -1)) {
    lua_pop(L, 1);
    return {false, "numSteps out of range"};
  }
  const int numSteps = static_cast<int>(lua_tointeger(L, -1));
  lua_pop(L, 1);
  if (numSteps < 1 || numSteps > static_cast<int>(seq::MAX_PATTERN_STEPS))
    return {false, "numSteps out of range"};

  lua_getfield(L, patternIndex, "stepsPerBeat");
  if (!lua_isinteger(L, -1)) {
    lua_pop(L, 1);
    return {false, "stepsPerBeat out of range"};
  }
  const int stepsPerBeat = static_cast<int>(lua_tointeger(L, -1));
  lua_pop(L, 1);
  if (stepsPerBeat < 1 || stepsPerBeat > static_cast<int>(seq::MAX_STEPS_PER_BEAT))
    return {false, "stepsPerBeat out of range"};

  lua_getfield(L, patternIndex, "steps");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return {false, "steps must be a table"};
  }

  outPattern.numSteps = static_cast<uint8_t>(numSteps);
  outPattern.stepsPerBeat = static_cast<uint8_t>(stepsPerBeat);

  const int stepsIndex = lua_absindex(L, -1);
  for (uint8_t step = 0; step < outPattern.numSteps; ++step) {
    lua_rawgeti(L, stepsIndex, step + 1);
    seq::StepEvent event{};
    auto stepRes = parseLuaStepEvent(L, -1, event);
    lua_pop(L, 1);
    if (!stepRes.ok) {
      lua_pop(L, 1);
      return stepRes;
    }
    outPattern.steps[step] = event;
  }

  lua_pop(L, 1);
  return {true, nullptr};
}

} // namespace lua

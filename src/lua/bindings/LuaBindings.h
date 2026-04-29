#pragma once

#include "lua/LuaState.h"

#include <string>
#include <vector>

// IMPORTANT: only use this if your not returning/adding anything to the stack
#define CMD_CHECK(expr)                                                                            \
  do {                                                                                             \
    if (!(expr).ok) {                                                                              \
      luaL_error(L, "%s", (expr).err);                                                             \
      return CMD_FAILURE;                                                                          \
    }                                                                                              \
    printf("OK\n");                                                                                \
    return CMD_SUCCESS;                                                                            \
  } while (0)

#define CHECK_ARG_COUNT(numExpected)                                                               \
  do {                                                                                             \
    if (lua_gettop(L) != (numExpected)) {                                                          \
      return luaL_error(L, "expected " #numExpected " arguments, got %d", lua_gettop(L));          \
    }                                                                                              \
  } while (0)

#define CMD_BAD_INPUT 10
#define CMD_FAILURE   1
#define CMD_SUCCESS   0

namespace lua::bindings {

const std::vector<std::string>& getVisibleGlobals();
void addVisibleGlobal(const char* name);

// IMPORTANT:  table must be top of stack! <- improve at some point...
inline void registerFunction(lua_State* L, lua_CFunction l_func, const char* name) {
  lua_pushcfunction(L, l_func);
  lua_setfield(L, -2, name);
}

// ==== Params ====
const std::vector<std::string>* getParamFields(const char* group);
int paramGroupIndex(lua_State* L);
int paramGroupNewIndex(lua_State* L);
void registerParamBindings(lua_State*);

// ==== Synth ====
void registerSynthBindings(lua_State* L, AppContext& appCtx);

// ==== Mixer ====
const std::vector<std::string>* getAppParamFields(const char* table);
void registerMixerBindings(lua_State* L);

// ==== Sequencer ====
void registerSeqCommands(lua_State* L);

// ==== MIDI ====
void registerMIDICommands(lua_State*);

// ==== Transport ====
void registerTransportCommands(lua_State* L);

} // namespace lua::bindings

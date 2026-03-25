#pragma once

#include "synth/Engine.h"
#include "synth/VoicePool.h"
#include "synth_io/SynthIO.h"

#include <cstdint>
#include <cstdio>
#include <iostream>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

namespace lua {
using synth::Engine;
using synth_io::hSynthSession;

constexpr uint8_t MAX_PARTS = 8;

struct PartContext {
  Engine* enginePtr = nullptr;
  hSynthSession sessionPtr = nullptr;
};

struct LuaContext {
  Engine* engines[MAX_PARTS] = {};
  hSynthSession sessions[MAX_PARTS] = {};
  uint8_t currentPart = 0;
};

struct LuaManager {

  void initManager(Engine* enginePtr, hSynthSession sessionPtr) {
    if (L && ctx) {
      printf("LuaState already initialized");
      return;
    }

    L = luaL_newstate();
    luaL_openlibs(L);

    ctx = new LuaContext();

    addContextPart(enginePtr, sessionPtr);
  }

  bool addContextPart(Engine* enginePtr, hSynthSession sessionPtr) {
    uint8_t index = findFreeSlot();

    if (index >= MAX_PARTS) {
      printf("Unable to add context part: no free slots");
      return false;
    }

    ctx->engines[index] = enginePtr;
    ctx->sessions[index] = sessionPtr;

    lua_pushlightuserdata(L, ctx);
    lua_setfield(L, LUA_REGISTRYINDEX, "synthctx");

    return true;
  }

  bool removeContextPart(Engine* enginePtr, hSynthSession sessionPtr) {
    for (uint8_t i = 0; i < MAX_PARTS; i++) {
      if (ctx->engines[i] == enginePtr && ctx->sessions[i] == sessionPtr) {
        ctx->engines[i] = nullptr;
        ctx->sessions[i] = nullptr;
        return true;
      }
    }
    printf("Unable to find context part to remove");
    return false;
  }

  void destroyManager() {
    if (L)
      lua_close(L);

    if (ctx)
      delete ctx;
  }

  PartContext getCurrentPart() const {
    lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");

    auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    return {ctx->engines[ctx->currentPart], ctx->sessions[ctx->currentPart]};
  }

private:
  lua_State* L = nullptr;
  LuaContext* ctx = nullptr;

  uint8_t findFreeSlot() {
    for (uint8_t i = 0; i < MAX_PARTS; i++) {
      if (ctx->engines[i] == nullptr && ctx->sessions[i] == nullptr)
        return i;
    }
    return MAX_PARTS;
  }
};

inline int lPanic(LuaManager* man) {
  auto ctx = man->getCurrentPart();
  synth::voices::panicVoicePool(ctx.enginePtr->voicePool);
  return 0;
}

class LuaState {
public:
  LuaState() {
    L = luaL_newstate();
    luaL_openlibs(L);
  }

  ~LuaState() {
    if (L)
      lua_close(L);
  }

  lua_State* get() { return L; }

  // ==== Run script ====
  void runString(const char* code) {
    if (luaL_dostring(L, code) != LUA_OK) {
      printError();
    }
  }

  void runFile(const char* path) {
    if (luaL_dofile(L, path) != LUA_OK) {
      printError();
    }
  }

  // ==== Get global (typed) ====
  int getInt(const char* name) {
    StackGuard guard(L);

    lua_getglobal(L, name);
    return (int)luaL_checkinteger(L, -1);
  }

  double getNumber(const char* name) {
    StackGuard guard(L);

    lua_getglobal(L, name);
    return luaL_checknumber(L, -1);
  }

  std::string getString(const char* name) {
    StackGuard guard(L);

    lua_getglobal(L, name);
    return luaL_checkstring(L, -1);
  }

  // ==== Call Lua function ====
  template <typename... Args> int callInt(const char* func, Args... args) {
    StackGuard guard(L);

    lua_getglobal(L, func); // push function
    pushAll(args...);       // push args

    if (lua_pcall(L, sizeof...(Args), 1, 0) != LUA_OK) {
      printError();
    }

    return (int)luaL_checkinteger(L, -1);
  }

  template <typename... Args> double callNumber(const char* func, Args... args) {
    StackGuard guard(L);

    lua_getglobal(L, func);
    pushAll(args...);

    if (lua_pcall(L, sizeof...(Args), 1, 0) != LUA_OK) {
      printError();
    }

    return luaL_checknumber(L, -1);
  }

  // ==== Register C function ====
  void registerFunction(const char* name, lua_CFunction fn) { lua_register(L, name, fn); }

private:
  lua_State* L = nullptr;

  // ==== Stack guard (CRITICAL) ====
  class StackGuard {
  public:
    StackGuard(lua_State* L) : L(L), top(lua_gettop(L)) {}
    ~StackGuard() {
      lua_settop(L, top); // restore stack
    }

  private:
    lua_State* L;
    int top;
  };

  // ==== Push helpers ====
  void push(int v) { lua_pushinteger(L, v); }
  void push(double v) { lua_pushnumber(L, v); }
  void push(const std::string& v) { lua_pushstring(L, v.c_str()); }

  void pushAll() {}

  template <typename T, typename... Rest> void pushAll(T v, Rest... rest) {
    push(v);
    pushAll(rest...);
  }

  // ==== Error handling ====
  void printError() {
    const char* err = lua_tostring(L, -1);
    lua_pop(L, 1);
    std::cout << err;
    // TODO:  handle Lua errors
  }
};

} // namespace lua

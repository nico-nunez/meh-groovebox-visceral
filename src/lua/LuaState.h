#pragma once

#include "app/AppContext.h"

#include "app/AudioSession.h"
#include "synth/Engine.h"
#include "synth/preset/Preset.h"

#include <cstdint>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

namespace lua {
using app::AppContext;
using app::TrackQueues;
using app::TrackRuntime;

using synth::Engine;
using synth::preset::Preset;

struct LuaContext {
  AppContext* app = nullptr;
};

inline LuaContext* getLuaContext(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return ctx;
}

inline TrackRuntime* getTrackRuntime(LuaContext* ctx, uint8_t track = app::MAX_TRACKS) {
  return app::getTrackRuntime(ctx->app, track);
}

inline Engine* getTrackEngine(LuaContext* ctx, uint8_t track = app::MAX_TRACKS) {
  return app::getTrackEngine(ctx->app, track);
}

inline TrackQueues* getTrackQueues(LuaContext* ctx, uint8_t track = app::MAX_TRACKS) {
  return app::getTrackQueues(ctx->app, track);
}

} // namespace lua

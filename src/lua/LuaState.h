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
using app::TrackSession;

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

inline Engine* getTrackEngine(LuaContext* ctx, uint8_t track = app::MAX_TRACKS) {
  return app::getTrackEngine(ctx->app, track);
}

inline app::TrackSession* getTrackSession(LuaContext* ctx, uint8_t track = app::MAX_TRACKS) {
  return app::getTrackSession(ctx->app, track);
}

} // namespace lua

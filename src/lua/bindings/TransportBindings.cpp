#include "LuaBindings.h"
#include "lua/LuaRuntimeMetadata.h"

namespace lua::bindings {

namespace {

int l_setBPM(lua_State* L) {
  auto* ctx = getLuaContext(L);
  float bpm = static_cast<float>(luaL_checknumber(L, 1));

  app::ControlEvent evt{};
  evt.type = app::ControlEvent::Type::SetBPM;
  evt.data.setBPM.bpm = bpm;

  CMD_CHECK(app::pushControlEvent(ctx->app, evt));
}

int l_setPlay(lua_State* L) {
  auto* ctx = getLuaContext(L);

  app::ControlEvent evt{};
  evt.type = app::ControlEvent::Type::Play;

  CMD_CHECK(app::pushControlEvent(ctx->app, evt));
}

int l_setPause(lua_State* L) {
  auto* ctx = getLuaContext(L);

  app::ControlEvent evt{};
  evt.type = app::ControlEvent::Type::Pause;

  CMD_CHECK(app::pushControlEvent(ctx->app, evt));
}

int l_setStop(lua_State* L) {
  auto* ctx = getLuaContext(L);

  app::ControlEvent evt{};
  evt.type = app::ControlEvent::Type::Stop;

  CMD_CHECK(app::pushControlEvent(ctx->app, evt));
}
} // namespace

void registerTransportCommands(lua_State* L) {
  lua_newtable(L);

  registerTableFunction(L, l_setBPM, rtmethod::SetBPM);
  registerTableFunction(L, l_setPlay, rtmethod::Play);
  registerTableFunction(L, l_setPause, rtmethod::Pause);
  registerTableFunction(L, l_setStop, rtmethod::Stop);

  lua_setglobal(L, rtglobal::Transport);
  addVisibleGlobal(rtglobal::Transport);
}

} // namespace lua::bindings

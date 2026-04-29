#include "lua/bindings/LuaBindings.h"

#include "app/AppParams.h"
#include "app/ControlEvents.h"

#include "dsp/Math.h"

#include <cstdio>

namespace lua::bindings {

static std::unordered_map<std::string, std::vector<std::string>> gHostFields;

namespace {
using app::pushControlEvent;
using app::params::AppParamType;

namespace evt = app::events;

void buildAppFieldIndex() {
  gHostFields.clear();
  for (const auto& def : app::params::APP_PARAM_DEFS)
    gHostFields[def.table].push_back(def.field);

  for (auto& [_, fields] : gHostFields) {
    std::sort(fields.begin(), fields.end());
    fields.erase(std::unique(fields.begin(), fields.end()), fields.end());
  }
}

// =====================
// Master
// =====================

int l_masterList(lua_State* L) {
  auto* ctx = getLuaContext(L);
  const auto& m = ctx->app->mixer;
  float threshdB = dsp::math::linearTodB(m.limiterThreshold);

  printf("master gain:  %.2f\n", m.masterGain);
  printf("limiter thr:  %.1f dB\n", threshdB);
  return CMD_SUCCESS;
}

// =======================
// Registration helpers
// =======================

int paramTableIndex(lua_State* L) {
  const char* tableName = lua_tostring(L, lua_upvalueindex(1));
  const char* field = luaL_checkstring(L, 2);

  const auto* def = app::params::findAppParamByTableField(tableName, field);
  if (!def) {
    lua_pushnil(L);
    return 1;
  }

  auto* ctx = getLuaContext(L);
  float value = app::params::getAppParamValue(ctx->app, def->id, ctx->currentTrack).value;

  if (def->type == AppParamType::Bool)
    lua_pushboolean(L, value >= 0.5f);
  else
    lua_pushnumber(L, value);

  return 1;
}

int paramTableNewIndex(lua_State* L) {
  const char* tableName = lua_tostring(L, lua_upvalueindex(1));
  const char* field = luaL_checkstring(L, 2);

  const auto* def = app::params::findAppParamByTableField(tableName, field);
  if (!def)
    return luaL_error(L, "unknown host param: %s.%s", tableName, field);

  auto* ctx = getLuaContext(L);
  float value = 0.0f;

  switch (def->type) {
  case AppParamType::Float:
    value = static_cast<float>(luaL_checknumber(L, 3));
    break;

  case AppParamType::Bool:
    luaL_checktype(L, 3, LUA_TBOOLEAN);
    value = lua_toboolean(L, 3) ? 1.0f : 0.0f;
    break;
  }

  uint8_t track = app::params::isTrackScoped(def->id) ? ctx->currentTrack : 0;
  auto evt = evt::createAppParamEvent(def->id, value, track);

  CMD_CHECK(pushControlEvent(ctx->app, evt));
}

void registerParamProxyTable(lua_State* L, const char* tableName) {
  lua_newtable(L); // proxy

  if (strcmp(tableName, "mixer") == 0) {
    registerFunction(L, l_masterList, "list");
  }

  lua_newtable(L); // metatable

  lua_pushstring(L, tableName);
  lua_pushcclosure(L, paramTableIndex, 1);
  lua_setfield(L, -2, "__index");

  lua_pushstring(L, tableName);
  lua_pushcclosure(L, paramTableNewIndex, 1);
  lua_setfield(L, -2, "__newindex");

  lua_setmetatable(L, -2); // set on proxy

  lua_setglobal(L, tableName);
  addVisibleGlobal(tableName);
}

} // namespace

// Auto-complete hints
const std::vector<std::string>* getAppParamFields(const char* table) {
  auto it = gHostFields.find(table);
  return it != gHostFields.end() ? &it->second : nullptr;
}

// =====================
// Registration
// =====================

void registerMixerBindings(lua_State* L) {
  buildAppFieldIndex();

  registerParamProxyTable(L, "mixer");
}

} // namespace lua::bindings

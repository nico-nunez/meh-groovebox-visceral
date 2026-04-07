#include "LuaState.h"
#include "lauxlib.h"
#include "lua.h"

#include "app/SynthSession.h"

#include "synth/events/Events.h"
#include "synth/params/ParamDefs.h"
#include "synth/params/ParamUtils.h"
#include "synth/preset/PresetApply.h"
#include "synth/preset/PresetIO.h"

#include "utils/KeyProcessor.h"

#include "dsp/fx/FXChain.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#define CMD_BAD_INPUT     10
#define CMD_EVENT_FAILURE 1
#define CMD_EVENT_SUCCESS 0

namespace lua::bindings {
namespace p = synth::param;

namespace preset = synth::preset;

namespace fx = dsp::fx::chain;

using app::session::pushEngineEvent;
using synth::events::EngineEvent;

namespace {

int paramGroupNewIndex(lua_State* L) {
  // stack: proxy table (1), key (2), value (3)
  const char* group = lua_tostring(L, lua_upvalueindex(1));
  const char* key = luaL_checkstring(L, 2);

  auto* ctx = getLuaContext(L);

  // Normal param path — float / bool / enum → SPSC queue
  char fullName[64];
  snprintf(fullName, sizeof(fullName), "%s.%s", group, key);

  auto paramID = p::utils::getParamIDByName(fullName);
  if (paramID == p::PARAM_COUNT) {
    luaL_error(L, "unknown param: %s.%s", group, key);
    return CMD_BAD_INPUT;
  }

  auto paramDef = p::getParamDef(paramID);

  float paramVal;

  switch (paramDef.type) {
  case p::ParamType::Float:
  case p::ParamType::Int8: {
    paramVal = static_cast<float>(luaL_checknumber(L, 3));
    break;
  }

  // Enable/Disable Item
  case p::ParamType::Bool:
    paramVal = lua_toboolean(L, 3) ? 1.0f : 0.0f;
    break;

  case p::ParamType::OscBankID:
  case p::ParamType::PhaseMode:
  case p::ParamType::NoiseType:
  case p::ParamType::FilterMode:
  case p::ParamType::DistortionType:
  case p::ParamType::Subdivision: {
    auto inputVal = luaL_checkstring(L, 3);
    auto res = p::utils::parseEnum(paramDef.type, inputVal);

    if (!res.ok) {
      printf("Unknown value: %s\n", inputVal);
      printf("%s\n", res.error);
      return CMD_BAD_INPUT;
    }
    paramVal = static_cast<float>(res.value);
    break;
  }
  }

  if (!pushParamEvent(getTrackSession(ctx), {static_cast<uint8_t>(paramID), paramVal})) {
    printf("failed to update param");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int paramGroupIndex(lua_State* L) {
  // stack: proxy table (-2), key (-1)
  const char* group = lua_tostring(L, lua_upvalueindex(1));
  const char* key = luaL_checkstring(L, 2);

  char fullName[64];
  snprintf(fullName, sizeof(fullName), "%s.%s", group, key);

  auto paramID = p::utils::getParamIDByName(fullName);
  if (paramID == p::PARAM_COUNT) {
    lua_pushnil(L);
    return 1;
  }

  auto* ctx = getLuaContext(L);

  float value = p::utils::getParamValueByID(getTrackEngine(ctx), paramID);
  lua_pushnumber(L, value);
  return 1;
}

void registerParamGroup(lua_State* L, const char* group) {
  lua_newtable(L); // proxy — stays empty forever
  lua_newtable(L); // metatable

  lua_pushstring(L, group);
  lua_pushcclosure(L, paramGroupIndex, 1); // __index, group name as upvalue
  lua_setfield(L, -2, "__index");

  lua_pushstring(L, group);
  lua_pushcclosure(L, paramGroupNewIndex, 1); // __newindex, group name as upvalue
  lua_setfield(L, -2, "__newindex");

  lua_setmetatable(L, -2);
  lua_setglobal(L, group); // _G[group] = proxy
}

void registerFXGroup(lua_State* L, const char* group, const char* key) {
  // expects the fx table on top of the stack
  lua_newtable(L);
  lua_newtable(L);

  lua_pushstring(L, group);
  lua_pushcclosure(L, paramGroupIndex, 1);
  lua_setfield(L, -2, "__index");

  lua_pushstring(L, group);
  lua_pushcclosure(L, paramGroupNewIndex, 1);
  lua_setfield(L, -2, "__newindex");

  lua_setmetatable(L, -2);
  lua_setfield(L, -2, key); // fx[key] = proxy
}

void registerFXGroups(lua_State* L) {
  lua_newtable(L); // the fx table

  registerFXGroup(L, "fx.distortion", "distortion");
  registerFXGroup(L, "fx.chorus", "chorus");
  registerFXGroup(L, "fx.phaser", "phaser");
  registerFXGroup(L, "fx.delay", "delay");
  registerFXGroup(L, "fx.reverb", "reverb");

  lua_setglobal(L, "fx");
}

void registerEnumGlobals(lua_State* L) {
  // SVF filter modes
  lua_pushnumber(L, 0.0);
  lua_setglobal(L, "lp");
  lua_pushnumber(L, 1.0);
  lua_setglobal(L, "hp");
  lua_pushnumber(L, 2.0);
  lua_setglobal(L, "bp");
  lua_pushnumber(L, 3.0);
  lua_setglobal(L, "notch");

  // Distortion types
  lua_pushnumber(L, 0.0);
  lua_setglobal(L, "soft");
  lua_pushnumber(L, 1.0);
  lua_setglobal(L, "hard");

  // Oscillator phase modes (prefixed to avoid clashing with Lua builtins)
  lua_pushnumber(L, 0.0);
  lua_setglobal(L, "phaseReset");
  lua_pushnumber(L, 1.0);
  lua_setglobal(L, "phaseFree");
  lua_pushnumber(L, 2.0);
  lua_setglobal(L, "phaseRandom");
  lua_pushnumber(L, 3.0);
  lua_setglobal(L, "phaseSpread");

  // Bank names — strings, routed through the `bank` special case in __newindex
  lua_pushstring(L, "sine");
  lua_setglobal(L, "sine");
  lua_pushstring(L, "saw");
  lua_setglobal(L, "saw");
  lua_pushstring(L, "square");
  lua_setglobal(L, "square");
  lua_pushstring(L, "triangle");
  lua_setglobal(L, "triangle");
  lua_pushstring(L, "sineToSaw");
  lua_setglobal(L, "sineToSaw");
  lua_pushstring(L, "sah");
  lua_setglobal(L, "sah");

  // Noise types
  lua_pushstring(L, "white");
  lua_setglobal(L, "white");
  lua_pushstring(L, "pink");
  lua_setglobal(L, "pink");
}

int l_modAdd(lua_State* L) {
  const char* srcName = luaL_checkstring(L, 1);
  const char* destName = luaL_checkstring(L, 2);
  float amount = (float)luaL_checknumber(L, 3);

  auto* ctx = getLuaContext(L);

  auto src = p::utils::parseModSrc(srcName);
  auto dest = p::utils::parseModDest(destName);
  if (!src.ok) {
    luaL_error(L, "%s: %s", src.error, srcName);
    return CMD_BAD_INPUT;
  }
  if (!dest.ok) {
    luaL_error(L, "%s: %s", dest.error, destName);
    return CMD_BAD_INPUT;
  }

  EngineEvent evt{};
  evt.type = EngineEvent::Type::AddModRoute;
  evt.data.addModRoute = {src.value, dest.value, amount};

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to add mod matrix route");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_modRemove(lua_State* L) {
  uint8_t index = (uint8_t)luaL_checkinteger(L, 1);

  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::RemoveModRoute;
  evt.data.removeModRoute = {index};

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to remove mod matrix route");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_modClear(lua_State* L) {
  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ClearModRoutes;

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to clear mod matrix");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_modList(lua_State* L) {
  auto* ctx = getLuaContext(L);

  p::utils::printModList(getTrackEngine(ctx));
  return CMD_EVENT_SUCCESS;
}

void registerModCommands(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, l_modAdd);
  lua_setfield(L, -2, "add");
  lua_pushcfunction(L, l_modRemove);
  lua_setfield(L, -2, "remove");
  lua_pushcfunction(L, l_modClear);
  lua_setfield(L, -2, "clear");
  lua_pushcfunction(L, l_modList);
  lua_setfield(L, -2, "list");
  lua_setglobal(L, "mod");
}

int l_fmAdd(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);
  const char* sourceName = luaL_checkstring(L, 2);
  float depth = (float)luaL_optnumber(L, 3, 1.0);

  auto* ctx = getLuaContext(L);

  auto fmCarrier = p::utils::parseEnumFM(carrierName);
  if (!fmCarrier.ok) {
    luaL_error(L, "unknown FM carrier: %s", carrierName);
    return CMD_BAD_INPUT;
  }
  if (fmCarrier.value == p::utils::FMCarrier::None) {
    luaL_error(L, "invalid FM carrier: %s", carrierName);
    return CMD_BAD_INPUT;
  }

  auto fmSource = p::utils::parseEnumFM(sourceName);
  if (!fmSource.ok) {
    luaL_error(L, "unknown FM source: %s", sourceName);
    return CMD_BAD_INPUT;
  }

  EngineEvent evt{};
  evt.type = EngineEvent::Type::AddFMRoute;
  evt.data.addFMRoute = {fmCarrier.value, fmSource.value, depth};

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to add FM route");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_fmRemove(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);
  const char* sourceName = luaL_checkstring(L, 2);

  auto* ctx = getLuaContext(L);

  auto fmCarrier = p::utils::parseEnumFM(carrierName);
  if (!fmCarrier.ok) {
    luaL_error(L, "unknown FM carrier: %s", carrierName);
    return CMD_BAD_INPUT;
  }
  if (fmCarrier.value == p::utils::FMCarrier::None) {
    luaL_error(L, "invalid FM carrier: %s", carrierName);
    return CMD_BAD_INPUT;
  }

  auto fmSource = p::utils::parseEnumFM(sourceName);
  if (!fmSource.ok) {
    luaL_error(L, "unknown FM source: %s", sourceName);
    return CMD_BAD_INPUT;
  }

  EngineEvent evt{};
  evt.type = EngineEvent::Type::RemoveFMRoute;
  evt.data.removeFMRoute = {fmCarrier.value, fmSource.value};

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to remove FM route");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_fmClear(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);
  auto* ctx = getLuaContext(L);

  auto fmCarrier = p::utils::parseEnumFM(carrierName);
  if (!fmCarrier.ok) {
    luaL_error(L, "unknown FM carrier: %s", carrierName);
    return CMD_BAD_INPUT;
  }
  if (fmCarrier.value == p::utils::FMCarrier::None) {
    luaL_error(L, "invalid FM carrier: %s", carrierName);
    return CMD_BAD_INPUT;
  }

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ClearFMRoutes;
  evt.data.clearFMRoutes = {fmCarrier.value};

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to clear FM route");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_fmList(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);
  auto* ctx = getLuaContext(L);

  auto carrier = p::utils::printFMList(getTrackEngine(ctx), carrierName);
  if (!carrier.ok) {
    luaL_error(L, "%s: %s", carrier.error, carrierName);
    return CMD_BAD_INPUT;
  }

  return CMD_EVENT_SUCCESS;
}

void registerFMCommands(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, l_fmAdd);
  lua_setfield(L, -2, "add");
  lua_pushcfunction(L, l_fmRemove);
  lua_setfield(L, -2, "remove");
  lua_pushcfunction(L, l_fmClear);
  lua_setfield(L, -2, "clear");
  lua_pushcfunction(L, l_fmList);
  lua_setfield(L, -2, "list");
  lua_setglobal(L, "fm");
}

int l_presetLoad(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);

  auto* ctx = getLuaContext(L);
  auto track = ctx->app->currentTrack;

  auto result = preset::loadPresetByName(name);
  if (!result.ok()) {
    luaL_error(L, "load failed: %s", result.error.c_str());
    return CMD_EVENT_FAILURE;
  }

  ctx->app->presetStore.slots[track] = result.preset;
  ctx->app->presetStore.valid[track] = true;

  synth::EngineEvent evt{};
  evt.type = synth::EngineEvent::Type::ApplyPreset;
  evt.data.applyPreset.preset = &ctx->app->presetStore.slots[track];

  if (!app::session::pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "engine event queue full, preset apply dropped");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_presetSave(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  auto* ctx = getLuaContext(L);

  auto p = preset::capturePreset(*getTrackEngine(ctx));
  std::string path = preset::getUserPresetsDir() + "/" + name + ".json";
  std::string err = preset::savePreset(p, path);
  if (!err.empty()) {
    luaL_error(L, "save failed: %s", err.c_str());
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_presetInit(lua_State* L) {
  auto* ctx = getLuaContext(L);
  uint8_t track = ctx->app->currentTrack;

  ctx->app->presetStore.slots[track] = preset::createInitPreset();
  ctx->app->presetStore.valid[track] = true;

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ApplyPreset;
  evt.data.applyPreset.preset = &ctx->app->presetStore.slots[track];

  if (!app::session::pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "engine event queue full, init preset apply dropped");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_presetList(lua_State*) {
  auto entries = preset::listPresets();
  for (const auto& e : entries)
    printf("  [%s] %s\n", e.isFactory ? "factory" : "user", e.name.c_str());
  return 0;
}

void registerPresetCommands(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, l_presetLoad);
  lua_setfield(L, -2, "load");
  lua_pushcfunction(L, l_presetSave);
  lua_setfield(L, -2, "save");
  lua_pushcfunction(L, l_presetInit);
  lua_setfield(L, -2, "init");
  lua_pushcfunction(L, l_presetList);
  lua_setfield(L, -2, "list");
  lua_setglobal(L, "preset");
}

int l_fxSet(lua_State* L) {
  auto* ctx = getLuaContext(L);
  int nargs = lua_gettop(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::SetFXChain;
  uint8_t& count = evt.data.setFXChain.count;
  count = 0;

  for (int i = 1; i <= nargs && count < fx::MAX_EFFECT_SLOTS; i++) {
    const char* name = luaL_checkstring(L, i);
    auto fxProc = p::utils::parseFXProcessor(name);
    if (!fxProc.ok) {
      luaL_error(L, "unknown fx processor: %s", name);
      return CMD_BAD_INPUT;
    }
    evt.data.setFXChain.processors[count++] = fxProc.value;
  }

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to set fx chain");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_fxClear(lua_State* L) {
  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ClearFXChain;

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to clear fx chain");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_fxList(lua_State* L) {
  auto* ctx = getLuaContext(L);

  fx::printFXChain(getTrackEngine(ctx)->fxChain);
  return 0;
}

void registerFXCommands(lua_State* L) {
  lua_getglobal(L, "fx"); // retrieve the existing fx table
  lua_pushcfunction(L, l_fxSet);
  lua_setfield(L, -2, "set");
  lua_pushcfunction(L, l_fxList);
  lua_setfield(L, -2, "list");
  lua_pushcfunction(L, l_fxClear);
  lua_setfield(L, -2, "clear");
  lua_pop(L, 1); // pop fx table
}

int l_signalSet(lua_State* L) {
  auto* ctx = getLuaContext(L);
  int nargs = lua_gettop(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::SetSignalChain;
  uint8_t& count = evt.data.setSignalChain.count;
  count = 0;

  for (int i = 1; i <= nargs && count < p::utils::MAX_SIGNAL_CHAIN_SLOTS; i++) {
    const char* name = luaL_checkstring(L, i);
    auto sigProc = p::utils::parseSignalProcessor(name);
    if (!sigProc.ok) {
      luaL_error(L, "%s: %s", sigProc.error, name);
      return CMD_BAD_INPUT;
    }
    evt.data.setSignalChain.processors[count++] = sigProc.value;
  }

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to set signal chain");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_signalClear(lua_State* L) {
  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ClearSignalChain;

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to clear signal chain");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_signalList(lua_State* L) {
  auto* ctx = getLuaContext(L);

  p::utils::printSignalChain(getTrackEngine(ctx));

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

void registerSignalCommands(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, l_signalSet);
  lua_setfield(L, -2, "set");
  lua_pushcfunction(L, l_signalList);
  lua_setfield(L, -2, "list");
  lua_pushcfunction(L, l_signalClear);
  lua_setfield(L, -2, "clear");
  lua_setglobal(L, "signal");
}

int l_panic(lua_State* L) {
  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::Panic;

  if (!pushEngineEvent(getTrackSession(ctx), evt)) {
    luaL_error(L, "failed to panic");
    return CMD_EVENT_FAILURE;
  }

  printf("OK\n");
  return CMD_EVENT_SUCCESS;
}

int l_params(lua_State* L) {
  auto* ctx = getLuaContext(L);

  for (int i = 0; i < synth::param::PARAM_COUNT; i++) {
    auto id = static_cast<synth::param::ParamID>(i);
    float val = p::utils::getParamValueByID(getTrackEngine(ctx), id);
    printf("%-35s %g\n", synth::param::PARAM_DEFS[i].name, val);
  }
  return 0;
}

int l_select(lua_State* L) {
  int track = (int)luaL_checkinteger(L, 1);
  auto* ctx = getLuaContext(L);

  if (track < 1 || track > (int)app::MAX_TRACKS)
    luaL_error(L, "track %d out of range (1–%d)", track, (int)app::MAX_TRACKS);
  ctx->app->currentTrack = (uint8_t)(track - 1);
  return 0;
}

int l_quit(lua_State*) {
  synth::utils::requestQuit();
  printf("Goodbye\n");
  return 0;
}

int l_clear(lua_State*) {
  system("clear");
  return 0;
}

} // anonymous namespace

void registerSynthBindings(lua_State* L, AppContext& appCtx) {

  // 1. Store context in registry
  auto* ctx = new LuaContext{};
  ctx->app = &appCtx;
  lua_pushlightuserdata(L, ctx);
  lua_setfield(L, LUA_REGISTRYINDEX, "synthctx");

  // 2. Param group proxy tables
  registerParamGroup(L, "osc1");
  registerParamGroup(L, "osc2");
  registerParamGroup(L, "osc3");
  registerParamGroup(L, "osc4");
  registerParamGroup(L, "lfo1");
  registerParamGroup(L, "lfo2");
  registerParamGroup(L, "lfo3");
  registerParamGroup(L, "noise");
  registerParamGroup(L, "ampEnv");
  registerParamGroup(L, "filterEnv");
  registerParamGroup(L, "modEnv");
  registerParamGroup(L, "svf");
  registerParamGroup(L, "ladder");
  registerParamGroup(L, "saturator");
  registerParamGroup(L, "pitchBend");
  registerParamGroup(L, "mono");
  registerParamGroup(L, "porta");
  registerParamGroup(L, "unison");
  registerParamGroup(L, "master");
  registerParamGroup(L, "tempo"); // tempo.bpm

  // 3. Nested FX param proxy tables — must come before registerFXCommands
  registerFXGroups(L);

  // 4. Enum and bank globals
  registerEnumGlobals(L);

  // 5. Command tables (see lua-command-bindings.md)
  registerModCommands(L);
  registerFMCommands(L);
  registerPresetCommands(L);
  registerFXCommands(L); // adds fx.set/list/clear to the existing fx global
  registerSignalCommands(L);

  // 6. Top-level functions (see lua-command-bindings.md)
  lua_pushcfunction(L, l_panic);
  lua_setglobal(L, "panic");
  lua_pushcfunction(L, l_params);
  lua_setglobal(L, "params");
  lua_pushcfunction(L, l_select);
  lua_setglobal(L, "select");
  lua_pushcfunction(L, l_clear);
  lua_setglobal(L, "clear");
  lua_pushcfunction(L, l_quit);
  lua_setglobal(L, "quit");
}

} // namespace lua::bindings

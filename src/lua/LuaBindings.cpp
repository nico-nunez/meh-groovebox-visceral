#include "LuaState.h"
#include "lua.h"

#include "synth/LFO.h"
#include "synth/ModMatrix.h"
#include "synth/Noise.h"
#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/PresetApply.h"
#include "synth/PresetIO.h"
#include "synth/SignalChain.h"
#include "synth/VoicePool.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

#include "app/SynthSession.h"

#include "device_io/KeyCapture.h"
#include "dsp/fx/FXChain.h"

#include <cstdio>

namespace lua::bindings {

namespace pb = synth::param::bindings;

namespace voices = synth::voices;

namespace banks = synth::wavetable::banks;
namespace osc = synth::wavetable::osc;

namespace noise = synth::noise;

namespace mm = synth::mod_matrix;
namespace sc = synth::signal_chain;

namespace preset = synth::preset;

namespace fx = dsp::fx::chain;

namespace {

int paramGroupNewIndex(lua_State* L) {
  // stack: proxy table (1), key (2), value (3)
  const char* group = lua_tostring(L, lua_upvalueindex(1));
  const char* key = luaL_checkstring(L, 2);

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  Engine* engine = ctx->engines[ctx->currentPart];

  // Special case: osc*.bank / lfo*.bank — WavetableBank pointer, not in PARAM_DEFS
  if (strcmp(key, "bank") == 0) {
    const char* bankName = luaL_checkstring(L, 3);
    if (strncmp(group, "lfo", 3) == 0) {
      auto* lfo = voices::getLFOByName(engine->voicePool, group);
      if (!lfo) {
        luaL_error(L, "unknown lfo: %s", group);
        return 0;
      }
      lfo->bank = strcmp(bankName, "sah") == 0 ? nullptr : banks::getBankByName(bankName);
    } else {
      auto* o = voices::getOscByName(engine->voicePool, group);
      if (!o) {
        luaL_error(L, "unknown osc: %s", group);
        return 0;
      }
      auto* bank = banks::getBankByName(bankName);
      if (!bank) {
        luaL_error(L, "unknown bank: %s", bankName);
        return 0;
      }
      o->bank = bank;
      printf("OK\n");
    }
    return 0;
  }

  // Special case: osc*.fmSource — shorthand for clearFMRoutes + single addFMRoute at depth 1.0
  if (strcmp(key, "fmSource") == 0) {
    const char* srcName = luaL_checkstring(L, 3);
    auto* carrier = voices::getOscByName(engine->voicePool, group);
    if (!carrier) {
      luaL_error(L, "unknown osc: %s", group);
      return 0;
    }
    osc::clearFMRoutes(*carrier);
    if (strcmp(srcName, "none") != 0) {
      auto src = osc::parseFMSource(srcName);
      if (src == osc::FMSource::None) {
        luaL_error(L, "unknown fm source: %s", srcName);
        return 0;
      }
      osc::addFMRoute(*carrier, src, 1.0f);
      printf("OK\n");
    }
    return 0;
  }

  // Special case: noise.type — NoiseType enum, not in PARAM_DEFS
  if (strcmp(group, "noise") == 0 && strcmp(key, "type") == 0) {
    const char* typeName = luaL_checkstring(L, 3);
    engine->voicePool.noise.type = noise::parseNoiseType(typeName);
    printf("OK\n");

    return 0;
  }

  // Normal param path — float / bool / enum → SPSC queue
  char fullName[64];
  snprintf(fullName, sizeof(fullName), "%s.%s", group, key);

  auto paramID = pb::getParamIDByName(fullName);
  if (paramID == synth::param::PARAM_COUNT) {
    luaL_error(L, "unknown param: %s.%s", group, key);
    return 0;
  }

  float value;
  if (lua_isboolean(L, 3))
    value = lua_toboolean(L, 3) ? 1.0f : 0.0f;
  else
    value = (float)luaL_checknumber(L, 3);

  app::session::setParam(ctx->sessions[ctx->currentPart], static_cast<uint8_t>(paramID), value);
  printf("OK\n");

  return 0;
}

int paramGroupIndex(lua_State* L) {
  // stack: proxy table (-2), key (-1)
  const char* group = lua_tostring(L, lua_upvalueindex(1));
  const char* key = luaL_checkstring(L, 2);

  char fullName[64];
  snprintf(fullName, sizeof(fullName), "%s.%s", group, key);

  auto paramID = pb::getParamIDByName(fullName);
  if (paramID == synth::param::PARAM_COUNT) {
    lua_pushnil(L);
    return 1;
  }

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  float value = pb::getParamValueByID(ctx->engines[ctx->currentPart]->paramRouter, paramID);
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

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  auto src = mm::parseModSrc(srcName);
  auto dest = mm::parseModDest(destName);
  if (src == mm::ModSrc::NoSrc) {
    luaL_error(L, "unknown mod source: %s", srcName);
    return 0;
  }
  if (dest == mm::ModDest::NoDest) {
    luaL_error(L, "unknown mod dest: %s", destName);
    return 0;
  }

  mm::addRoute(ctx->engines[ctx->currentPart]->voicePool.modMatrix, src, dest, amount);
  return 0;
}

int l_modRemove(lua_State* L) {
  uint8_t index = (uint8_t)luaL_checkinteger(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  mm::removeRoute(ctx->engines[ctx->currentPart]->voicePool.modMatrix, index);
  return 0;
}

int l_modList(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  mm::printRoutes(ctx->engines[ctx->currentPart]->voicePool.modMatrix);
  return 0;
}

int l_modClear(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  mm::clearRoutes(ctx->engines[ctx->currentPart]->voicePool.modMatrix);
  return 0;
}

void registerModCommands(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, l_modAdd);
  lua_setfield(L, -2, "add");
  lua_pushcfunction(L, l_modRemove);
  lua_setfield(L, -2, "remove");
  lua_pushcfunction(L, l_modList);
  lua_setfield(L, -2, "list");
  lua_pushcfunction(L, l_modClear);
  lua_setfield(L, -2, "clear");
  lua_setglobal(L, "mod");
}

int l_fmAdd(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);
  const char* srcName = luaL_checkstring(L, 2);
  float depth = (float)luaL_optnumber(L, 3, 1.0);

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  auto* carrier = voices::getOscByName(ctx->engines[ctx->currentPart]->voicePool, carrierName);
  if (!carrier) {
    luaL_error(L, "unknown carrier: %s", carrierName);
    return 0;
  }

  auto src = osc::parseFMSource(srcName);
  if (src == osc::FMSource::None) {
    luaL_error(L, "unknown fm source: %s", srcName);
    return 0;
  }

  osc::addFMRoute(*carrier, src, depth);
  return 0;
}

int l_fmRemove(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);
  const char* srcName = luaL_checkstring(L, 2);

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  auto* carrier = voices::getOscByName(ctx->engines[ctx->currentPart]->voicePool, carrierName);
  if (!carrier) {
    luaL_error(L, "unknown carrier: %s", carrierName);
    return 0;
  }

  osc::removeFMRoute(*carrier, osc::parseFMSource(srcName));
  return 0;
}

int l_fmClear(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  auto* carrier = voices::getOscByName(ctx->engines[ctx->currentPart]->voicePool, carrierName);
  if (!carrier) {
    luaL_error(L, "unknown carrier: %s", carrierName);
    return 0;
  }

  osc::clearFMRoutes(*carrier);
  return 0;
}

int l_fmList(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  auto* carrier = voices::getOscByName(ctx->engines[ctx->currentPart]->voicePool, carrierName);
  if (!carrier) {
    luaL_error(L, "unknown carrier: %s", carrierName);
    return 0;
  }

  osc::printFMRoutes(*carrier, carrierName);
  return 0;
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

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  auto result = preset::loadPresetByName(name);
  if (!result.ok()) {
    luaL_error(L, "load failed: %s", result.error.c_str());
    return 0;
  }
  preset::applyPreset(result.preset, *ctx->engines[ctx->currentPart]);
  return 0;
}

int l_presetSave(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);

  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  auto p = preset::capturePreset(*ctx->engines[ctx->currentPart]);
  std::string path = preset::getUserPresetsDir() + "/" + name + ".json";
  std::string err = preset::savePreset(p, path);
  if (!err.empty())
    luaL_error(L, "save failed: %s", err.c_str());
  return 0;
}

int l_presetInit(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  auto p = preset::createInitPreset();
  preset::applyPreset(p, *ctx->engines[ctx->currentPart]);
  return 0;
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
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  int nargs = lua_gettop(L);
  fx::FXProcessor procs[fx::MAX_EFFECT_SLOTS];
  uint8_t count = 0;

  for (int i = 1; i <= nargs && count < fx::MAX_EFFECT_SLOTS; i++) {
    const char* name = luaL_checkstring(L, i);
    auto proc = fx::parseFXProcessor(name);
    if (proc == fx::FXProcessor::None) {
      luaL_error(L, "unknown fx processor: %s", name);
      return 0;
    }
    procs[count++] = proc;
  }
  fx::setFXChain(ctx->engines[ctx->currentPart]->fxChain, procs, count);
  return 0;
}

int l_fxList(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  fx::printFXChain(ctx->engines[ctx->currentPart]->fxChain);
  return 0;
}

int l_fxClear(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  fx::clearFXChain(ctx->engines[ctx->currentPart]->fxChain);
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
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);

  int nargs = lua_gettop(L);
  sc::SignalProcessor procs[sc::MAX_CHAIN_SLOTS];
  uint8_t count = 0;

  for (int i = 1; i <= nargs && count < sc::MAX_CHAIN_SLOTS; i++) {
    const char* name = luaL_checkstring(L, i);
    auto proc = sc::parseSignalProcessor(name);
    if (proc == sc::SignalProcessor::None) {
      luaL_error(L, "unknown signal processor: %s", name);
      return 0;
    }
    procs[count++] = proc;
  }
  sc::setSigChain(ctx->engines[ctx->currentPart]->voicePool.signalChain, procs, count);
  return 0;
}

int l_signalList(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  sc::printSigChain(ctx->engines[ctx->currentPart]->voicePool.signalChain);
  return 0;
}

int l_signalClear(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  sc::clearSigChain(ctx->engines[ctx->currentPart]->voicePool.signalChain);
  return 0;
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
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  voices::panicVoicePool(ctx->engines[ctx->currentPart]->voicePool);
  return 0;
}

int l_params(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  Engine* engine = ctx->engines[ctx->currentPart];

  for (int i = 0; i < synth::param::PARAM_COUNT; i++) {
    auto id = static_cast<synth::param::ParamID>(i);
    float val = pb::getParamValueByID(engine->paramRouter, id);
    printf("%-35s %g\n", synth::param::PARAM_DEFS[i].name, val);
  }
  return 0;
}

int l_select(lua_State* L) {
  int part = (int)luaL_checkinteger(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, "synthctx");
  auto* ctx = static_cast<LuaContext*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  if (part < 1 || part > (int)MAX_PARTS)
    luaL_error(L, "part %d out of range (1–%d)", part, (int)MAX_PARTS);
  ctx->currentPart = (uint8_t)(part - 1);
  return 0;
}

int l_quit(lua_State*) {
  device_io::terminateKeyCaptureLoop();
  printf("Goodbye\n");
  return 0;
}

} // anonymous namespace

void registerSynthBindings(lua_State* L, Engine& engine, hSynthSession session) {

  // 1. Store context in registry
  auto* ctx = new LuaContext{};
  ctx->engines[0] = &engine;
  ctx->sessions[0] = session;
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
  lua_pushcfunction(L, l_quit);
  lua_setglobal(L, "quit");
}

} // namespace lua::bindings

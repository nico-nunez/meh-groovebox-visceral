#include "LuaBindings.h"

#include "ParamBindings.h"
#include "SequencerBindings.h"

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

namespace lua::bindings {

namespace p = synth::param;
namespace preset = synth::preset;
namespace fx = dsp::fx::chain;

using app::pushEngineEvent;
using synth::events::EngineEvent;

static std::vector<std::string> gVisibleGlobals;

void addVisibleGlobal(const char* name) {
  gVisibleGlobals.push_back(name);
}

// ===================
// Anonymous Helpers
// ===================
namespace {

// =====================
// Fx Enums
// =====================
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

  lua_pushstring(L, group);
  lua_setfield(L, -2, "__name");

  // __fields: param field names for tab completion
  lua_newtable(L);
  int fieldIdx = 0;
  size_t groupLen = strlen(group);
  for (int i = 0; i < p::PARAM_COUNT; i++) {
    const char* name = p::PARAM_DEFS[i].name;
    if (strncmp(name, group, groupLen) == 0 && name[groupLen] == '.') {
      lua_pushstring(L, name + groupLen + 1);
      lua_rawseti(L, -2, ++fieldIdx);
    }
  }
  lua_setfield(L, -2, "__fields");

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
  addVisibleGlobal("fx");
}

// ===================
// Global Enums
// ===================
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

// =========================
// Modulation (add)
// =========================
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

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to add mod matrix route");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Modulation (remove)
// =========================
int l_modRemove(lua_State* L) {
  uint8_t index = (uint8_t)luaL_checkinteger(L, 1);

  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::RemoveModRoute;
  evt.data.removeModRoute = {index};

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to remove mod matrix route");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Modulation (clear)
// =========================
int l_modClear(lua_State* L) {
  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ClearModRoutes;

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to clear mod matrix");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Modulation (print)
// =========================
int l_modList(lua_State* L) {
  auto* ctx = getLuaContext(L);

  p::utils::printModList(getTrackEngine(ctx));
  return CMD_SUCCESS;
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
  addVisibleGlobal("mod");
}

// =========================
// FM Modulation (add)
// =========================
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

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to add FM route");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// FM Modulation (remove)
// =========================
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

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to remove FM route");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// FM Modulation (clear)
// =========================
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

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to clear FM route");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// FM Modulation (print)
// =========================
int l_fmList(lua_State* L) {
  const char* carrierName = luaL_checkstring(L, 1);
  auto* ctx = getLuaContext(L);

  auto carrier = p::utils::printFMList(getTrackEngine(ctx), carrierName);
  if (!carrier.ok) {
    luaL_error(L, "%s: %s", carrier.error, carrierName);
    return CMD_BAD_INPUT;
  }

  return CMD_SUCCESS;
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
  addVisibleGlobal("fm");
}

// =========================
// Presets (load)
// =========================
int l_presetLoad(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);

  auto* ctx = getLuaContext(L);
  auto* trackRt = getTrack(ctx);

  auto result = preset::loadPresetByName(name);
  if (!result.ok()) {
    luaL_error(L, "load failed: %s", result.error.c_str());
    return CMD_FAILURE;
  }

  trackRt->preset = result.preset;
  trackRt->presetValid = true;

  synth::EngineEvent evt{};
  evt.type = synth::EngineEvent::Type::ApplyPreset;
  evt.data.applyPreset.preset = &trackRt->preset;

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "engine event queue full, preset apply dropped");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Presets (save)
// =========================
int l_presetSave(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  auto* ctx = getLuaContext(L);

  auto p = preset::capturePreset(*getTrackEngine(ctx));
  std::string path = preset::getUserPresetsDir() + "/" + name + ".json";
  std::string err = preset::savePreset(p, path);
  if (!err.empty()) {
    luaL_error(L, "save failed: %s", err.c_str());
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Presets (initialize)
// =========================
int l_presetInit(lua_State* L) {
  auto* ctx = getLuaContext(L);
  auto* trackRt = getTrack(ctx);

  trackRt->preset = preset::createInitPreset();
  trackRt->presetValid = true;

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ApplyPreset;
  evt.data.applyPreset.preset = &trackRt->preset;

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "engine event queue full, init preset apply dropped");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// ================================
// Presets (print list of presets)
// ================================
int l_presetList(lua_State*) {
  auto entries = preset::listPresets();
  for (const auto& e : entries)
    printf("  [%s] %s\n", e.isFactory ? "factory" : "user", e.name.c_str());
  return 0;
}

// ================================
// Preset (print current values)
// ================================
int l_presetDump(lua_State* L) {
  auto* ctx = getLuaContext(L);
  auto p = preset::capturePreset(*getTrackEngine(ctx));
  preset::printPreset(p); // assuming this exists
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
  lua_pushcfunction(L, l_presetDump);
  lua_setfield(L, -2, "dump");

  lua_setglobal(L, "preset");
  addVisibleGlobal("preset");
}

// =========================
// FX Chain (set)
// =========================
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

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to set fx chain");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// FX Chain (clear all)
// =========================
int l_fxClear(lua_State* L) {
  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ClearFXChain;

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to clear fx chain");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// FX Chain (print)
// =========================
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

// =========================
// Signal Chain (set)
// =========================
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

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to set signal chain");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Signal Chain (clear all)
// =========================
int l_signalClear(lua_State* L) {
  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::ClearSignalChain;

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to clear signal chain");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// =========================
// Signal Chain (print)
// =========================
int l_signalList(lua_State* L) {
  auto* ctx = getLuaContext(L);

  p::utils::printSignalChain(getTrackEngine(ctx));

  printf("OK\n");
  return CMD_SUCCESS;
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
  addVisibleGlobal("signal");
}

// =========================
// MIDI Mapping
// =========================
int l_midiSetChannelTrack(lua_State* L) {
  uint8_t channel = static_cast<uint8_t>(luaL_checkinteger(L, 1));
  uint8_t track = static_cast<uint8_t>(luaL_checkinteger(L, 2));

  if (channel > 15)
    return luaL_error(L, "channel must be 0-15");
  if (track >= app::MAX_TRACKS)
    return luaL_error(L, "track must be 0-%d", app::MAX_TRACKS - 1);

  auto* ctx = getLuaContext(L);
  ctx->app->midiChannelMap[channel] = track;
  return CMD_SUCCESS;
}

void registerMIDICommands(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, l_midiSetChannelTrack);
  lua_setfield(L, -2, "setChannelTrack");

  lua_setglobal(L, "midi");
  addVisibleGlobal("midi");
}

// =========================
// Trasnport
// =========================
int l_setBPM(lua_State* L) {
  auto* ctx = getLuaContext(L);
  float bpm = static_cast<float>(luaL_checknumber(L, 1));

  app::transport::TransportEvent action{};
  action.type = app::transport::TransportEvent::Type::SetBPM;
  action.data.setBPM.bpm = bpm;

  if (!pushTransportAction(ctx->app, action))
    return luaL_error(L, "transport queue full");

  return CMD_SUCCESS;
}

int l_setPlay(lua_State* L) {
  auto* ctx = getLuaContext(L);

  app::transport::TransportEvent action{};
  action.type = app::transport::TransportEvent::Type::Play;

  if (!pushTransportAction(ctx->app, action))
    return luaL_error(L, "transport queue full");

  return CMD_SUCCESS;
}

int l_setStop(lua_State* L) {
  auto* ctx = getLuaContext(L);

  app::transport::TransportEvent action{};
  action.type = app::transport::TransportEvent::Type::Stop;

  if (!pushTransportAction(ctx->app, action))
    return luaL_error(L, "transport queue full");

  return CMD_SUCCESS;
}

void registerTransportCommands(lua_State* L) {
  lua_newtable(L);

  lua_pushcfunction(L, l_setBPM);
  lua_setfield(L, -2, "setBPM");

  lua_pushcfunction(L, l_setPlay);
  lua_setfield(L, -2, "play");

  lua_pushcfunction(L, l_setStop);
  lua_setfield(L, -2, "stop");

  lua_setglobal(L, "transport");
  addVisibleGlobal("transport");
}

// =========================
// Panic! (stop all voices)
// =========================
int l_panic(lua_State* L) {
  auto* ctx = getLuaContext(L);

  EngineEvent evt{};
  evt.type = EngineEvent::Type::Panic;

  if (!pushEngineEvent(ctx->app, evt)) {
    luaL_error(L, "failed to panic");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
}

// ========================================
// Param (print - alt `print(group.name)`)
// ========================================
int l_get(lua_State* L) {
  const char* fullName = luaL_checkstring(L, 1);

  auto paramID = p::utils::getParamIDByName(fullName);
  if (paramID == p::PARAM_COUNT) {
    luaL_error(L, "unknown param: %s", fullName);
    return 1;
  }

  auto* ctx = getLuaContext(L);
  auto paramDef = p::getParamDef(paramID);
  float value = p::utils::getParamValueByID(getTrackEngine(ctx), paramID);

  switch (paramDef.type) {
  case p::ParamType::OscBankID:
  case p::ParamType::PhaseMode:
  case p::ParamType::NoiseType:
  case p::ParamType::FilterMode:
  case p::ParamType::DistortionType:
  case p::ParamType::Subdivision:
    printf("%s = %s\n",
           fullName,
           p::utils::enumToString(paramDef.type, static_cast<uint8_t>(value)));
    break;
  default:
    printf("%s = %g\n", fullName, value);
    break;
  }

  return CMD_SUCCESS;
}

// ====================
// Params (print list)
// ====================
int l_params(lua_State* L) {
  auto* ctx = getLuaContext(L);
  const char* filter = luaL_optstring(L, 1, nullptr);
  size_t filterLen = filter ? strlen(filter) : 0;

  for (int i = 0; i < synth::param::PARAM_COUNT; i++) {
    const char* name = synth::param::PARAM_DEFS[i].name;
    if (filter && strncmp(name, filter, filterLen) != 0)
      continue;

    auto id = static_cast<synth::param::ParamID>(i);
    float val = p::utils::getParamValueByID(getTrackEngine(ctx), id);
    auto& def = synth::param::PARAM_DEFS[i];

    switch (def.type) {
    case p::ParamType::OscBankID:
    case p::ParamType::PhaseMode:
    case p::ParamType::NoiseType:
    case p::ParamType::FilterMode:
    case p::ParamType::DistortionType:
    case p::ParamType::Subdivision:
      printf("%-35s %s\n", name, p::utils::enumToString(def.type, static_cast<uint8_t>(val)));
      break;
    default:
      printf("%-35s %g\n", name, val);
      break;
    }
  }
  return CMD_SUCCESS;
}

// =================
// Track selection
// =================
int l_select(lua_State* L) {
  int track = (int)luaL_checkinteger(L, 1);
  auto* ctx = getLuaContext(L);

  if (track < 1 || track > (int)app::MAX_TRACKS)
    luaL_error(L, "track %d out of range (1–%d)", track, (int)app::MAX_TRACKS);
  ctx->app->currentTrack = (uint8_t)(track - 1);
  return CMD_SUCCESS;
}

// =====================
// Help (print options)
// =====================
int l_help(lua_State* L) {
  const char* topic = luaL_optstring(L, 1, nullptr);

  if (!topic) {
    printf("Param groups (read/write as group.param):\n"
           "  osc1-4  lfo1-3  noise  ampEnv filterEnv modEnv\n"
           "  svf  ladder  saturator  pitchBend  mono  porta\n"
           "  unison  master  tempo\n"
           "  fx.distortion  fx.chorus  fx.phaser  fx.delay  fx.reverb\n"
           "\n"
           "Commands:\n"
           "  mod      mod.add/remove/clear/list\n"
           "  fm       fm.add/remove/clear/list\n"
           "  preset   preset.load/save/init/list\n"
           "  fx       fx.set/list/clear\n"
           "  signal   signal.set/list/clear\n"
           "\n"
           "Functions:\n"
           "  get(\"group.param\")  -- print param value\n"
           "  params([group])    -- list params (optional prefix filter)\n"
           "  panic()            -- silence all voices\n"
           "  select(n)          -- select track n (1-based)\n"
           "  clear()            -- clear terminal\n"
           "  quit()             -- exit\n"
           "\n"
           "help(\"topic\") for detail on: params mod fm preset fx signal\n");
    return CMD_SUCCESS;
  }

  if (strcmp(topic, "params") == 0) {
    printf("Reading params:  print(osc1.freq)   value = osc1.freq\n"
           "Writing params:  osc1.freq = 440    osc1.bank = \"saw\"\n"
           "List all:        params()\n"
           "List group:      params(\"osc1\")\n"
           "Print one:       get(\"osc1.freq\")\n"
           "\n"
           "Param groups: osc1 osc2 osc3 osc4 lfo1 lfo2 lfo3 noise\n"
           "              ampEnv filterEnv modEnv svf ladder saturator\n"
           "              pitchBend mono porta unison master tempo\n"
           "              fx.distortion fx.chorus fx.phaser fx.delay fx.reverb\n");

  } else if (strcmp(topic, "mod") == 0) {
    printf("mod.add(src, dest, amount)  -- add a mod route\n"
           "mod.remove(index)           -- remove route by index\n"
           "mod.clear()                 -- remove all routes\n"
           "mod.list()                  -- print all active routes\n"
           "\n"
           "Sources: ampEnv filterEnv modEnv lfo1 lfo2 lfo3\n"
           "         velocity keyTrack modWheel noise\n"
           "Dests:   osc1Pitch osc1ScanPos osc1FMDepth osc1Mix ... (osc1-4)\n"
           "         svfCutoff svfResonance ladderCutoff ladderResonance\n"
           "         lfo1Rate lfo1Amp lfo2Rate lfo2Amp lfo3Rate lfo3Amp\n");

  } else if (strcmp(topic, "fm") == 0) {
    printf("fm.add(carrier, source, depth)  -- add FM route (depth default 1.0)\n"
           "fm.remove(carrier, source)      -- remove a specific route\n"
           "fm.clear(carrier)               -- clear all routes on carrier\n"
           "fm.list(carrier)                -- list routes on carrier\n"
           "\n"
           "Carriers/sources: \"osc1\" \"osc2\" \"osc3\" \"osc4\"\n"
           "Example: fm.add(\"osc1\", \"osc2\", 0.5)  -- osc2 modulates osc1 at depth 0.5\n");

  } else if (strcmp(topic, "preset") == 0) {
    printf("preset.load(name)   -- load preset by name\n"
           "preset.save(name)   -- save current state as preset\n"
           "preset.init()       -- reset to init patch\n"
           "preset.list()       -- list all factory and user presets\n");

  } else if (strcmp(topic, "fx") == 0) {
    printf("fx.set(...)         -- set effects chain order\n"
           "fx.list()           -- print current effects chain\n"
           "fx.clear()          -- remove all effects from chain\n"
           "\n"
           "Effect names: \"distortion\" \"chorus\" \"phaser\" \"delay\" \"reverb\"\n"
           "Example: fx.set(\"distortion\", \"delay\", \"reverb\")\n"
           "\n"
           "Effect params: fx.reverb.decay = 0.8   fx.delay.time = 250\n");

  } else if (strcmp(topic, "signal") == 0) {
    printf("signal.set(...)     -- set voice signal chain order\n"
           "signal.list()       -- print current signal chain\n"
           "signal.clear()      -- remove all processors from chain\n"
           "\n"
           "Processor names: \"svf\" \"ladder\" \"saturator\"\n"
           "Example: signal.set(\"ladder\", \"saturator\")\n");

  } else {
    printf("Unknown topic: %s\nTopics: params mod fm preset fx signal\n", topic);
  }

  return CMD_SUCCESS;
}

// =================
// Clear (terminal)
// =================
int l_clear(lua_State*) {
  system("clear");
  return CMD_SUCCESS;
}

// =================
// Quit app
// =================
int l_quit(lua_State*) {
  app::utils::requestQuit();
  printf("Goodbye\n");
  return CMD_SUCCESS;
}

// =======================
// Cmd completion hints
// =======================
void finalizeCompletionMetadata() {
  std::sort(gVisibleGlobals.begin(), gVisibleGlobals.end());
  gVisibleGlobals.erase(std::unique(gVisibleGlobals.begin(), gVisibleGlobals.end()),
                        gVisibleGlobals.end());
}

} // anonymous namespace

const std::vector<std::string>& getVisibleGlobals() {
  return gVisibleGlobals;
}

void registerSynthBindings(lua_State* L, AppContext& appCtx) {

  // 1. Store context in registry
  auto* ctx = new LuaContext{};
  ctx->app = &appCtx;
  lua_pushlightuserdata(L, ctx);
  lua_setfield(L, LUA_REGISTRYINDEX, "synthctx");

  registerParamBindings(L);

  registerFXGroups(L);

  // 4. Enum and bank globals
  registerEnumGlobals(L);

  // 5. Command tables (see lua-command-bindings.md)
  registerModCommands(L);
  registerFMCommands(L);
  registerPresetCommands(L);
  registerFXCommands(L); // adds fx.set/list/clear to the existing fx global
  registerSignalCommands(L);
  registerMIDICommands(L);
  registerTransportCommands(L);
  registerSeqCommands(L);

  // 6. Top-level functions (see lua-command-bindings.md)
  lua_pushcfunction(L, l_panic);
  lua_setglobal(L, "panic");
  addVisibleGlobal("panic");

  lua_pushcfunction(L, l_params);
  lua_setglobal(L, "params");
  addVisibleGlobal("params");

  lua_pushcfunction(L, l_get);
  lua_setglobal(L, "get");
  addVisibleGlobal("get");

  lua_pushcfunction(L, l_select);
  lua_setglobal(L, "select");
  addVisibleGlobal("select");

  lua_pushcfunction(L, l_help);
  lua_setglobal(L, "help");
  addVisibleGlobal("help");

  lua_pushcfunction(L, l_clear);
  lua_setglobal(L, "clear");
  addVisibleGlobal("clear");

  lua_pushcfunction(L, l_quit);
  lua_setglobal(L, "quit");
  addVisibleGlobal("quit");

  finalizeCompletionMetadata();
}

} // namespace lua::bindings

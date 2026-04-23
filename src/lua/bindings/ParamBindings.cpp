#include "ParamBindings.h"

#include "lua/bindings/LuaBindings.h"

#include "synth/params/ParamDefs.h"
#include "synth/params/ParamUtils.h"

namespace lua::bindings {

namespace p = synth::param;

static std::unordered_map<std::string, std::vector<std::string>> gParamFields;

// ===================
// Anonymous Helpers
// ===================
namespace {

// ===================
// Param fields
// ===================
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

  addVisibleGlobal(group);
}

// =======================
// Cmd completion hints
// =======================
void finalizeCompletionMetadata() {
  for (auto& [group, fields] : gParamFields) {
    std::sort(fields.begin(), fields.end());
    fields.erase(std::unique(fields.begin(), fields.end()), fields.end());
  }
}

void buildParamFieldIndex() {
  gParamFields.clear();

  for (int i = 0; i < p::PARAM_COUNT; i++) {
    const char* name = p::PARAM_DEFS[i].name;

    const char* dot = strchr(name, '.');
    if (!dot)
      continue; // flat param like "masterGain"

    const char* secondDot = strchr(dot + 1, '.');

    std::string group = secondDot
                            ? std::string(name, secondDot) // "fx.reverb" from "fx.reverb.decay"
                            : std::string(name, dot);      // "osc1" from "osc1.bank"

    std::string field = secondDot ? std::string(secondDot + 1) : std::string(dot + 1);

    gParamFields[group].push_back(std::move(field));
  }
}

} // anonymous namespace

// ==== Completion hints ====
const std::vector<std::string>* getParamFields(const char* group) {
  auto it = gParamFields.find(group);
  return it != gParamFields.end() ? &it->second : nullptr;
}

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

  if (!pushParamEvent(ctx->app, {static_cast<uint8_t>(paramID), paramVal})) {
    printf("failed to update param");
    return CMD_FAILURE;
  }

  printf("OK\n");
  return CMD_SUCCESS;
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
  auto paramDef = p::getParamDef(paramID);
  float value = p::utils::getParamValueByID(getTrackEngine(ctx), paramID);

  switch (paramDef.type) {
  case p::ParamType::OscBankID:
  case p::ParamType::PhaseMode:
  case p::ParamType::NoiseType:
  case p::ParamType::FilterMode:
  case p::ParamType::DistortionType:
  case p::ParamType::Subdivision: {
    const char* str = p::utils::enumToString(paramDef.type, static_cast<uint8_t>(value));
    lua_pushstring(L, str);
    break;
  }
  default:
    lua_pushnumber(L, value);
    break;
  }

  return 1;
}

void registerParamBindings(lua_State* L) {
  buildParamFieldIndex();

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

  finalizeCompletionMetadata();
}

} // namespace lua::bindings

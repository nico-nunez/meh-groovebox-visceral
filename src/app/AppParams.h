#pragma once

#include "app/Types.h"
#include <cstdint>
#include <cstring>

namespace app {
struct AppContext;
}

namespace app::params {

enum class AppParamID : uint8_t {
  TrackGain,
  TrackPan,
  TrackMute,
  MasterGain,
  LimiterThresholdDB,
  Count,
};

enum class AppParamScope : uint8_t {
  Track,
  Mixer,
};

enum class AppParamType : uint8_t {
  Float,
  Bool,
};

enum class AppParamUnit : uint8_t {
  Linear,
  Db,
  Boolean,
};

struct AppParamDef {
  AppParamID id;
  const char* path;  // "track.gain", "mixer.masterGain"
  const char* table; // "track", "mixer"
  const char* field; // "gain", "masterGain"
  AppParamScope scope;
  AppParamType type;
  AppParamUnit unit;
  float min;
  float max;
  float defaultValue; // public value, not necessarily storage value
};

inline constexpr AppParamDef APP_PARAM_DEFS[] = {
    {AppParamID::TrackGain,
     "track.gain",
     "track",
     "gain",
     AppParamScope::Track,
     AppParamType::Float,
     AppParamUnit::Linear,
     0.0f,
     1.0f,
     1.0f},

    {AppParamID::TrackPan,
     "track.pan",
     "track",
     "pan",
     AppParamScope::Track,
     AppParamType::Float,
     AppParamUnit::Linear,
     -1.0f,
     1.0f,
     0.0f},

    {AppParamID::TrackMute,
     "track.mute",
     "track",
     "mute",
     AppParamScope::Track,
     AppParamType::Bool,
     AppParamUnit::Boolean,
     0.0f,
     1.0f,
     0.0f},

    {AppParamID::MasterGain,
     "mixer.masterGain",
     "mixer",
     "masterGain",
     AppParamScope::Mixer,
     AppParamType::Float,
     AppParamUnit::Linear,
     0.0f,
     1.0f,
     1.0f},

    {AppParamID::LimiterThresholdDB,
     "mixer.limiterThreshold",
     "mixer",
     "limiterThreshold",
     AppParamScope::Mixer,
     AppParamType::Float,
     AppParamUnit::Db,
     -60.0f,
     0.0f,
     -1.0f},
};

inline constexpr uint8_t HOST_PARAM_COUNT = static_cast<uint8_t>(AppParamID::Count);

inline const AppParamDef& getAppParamDef(AppParamID id) {
  return APP_PARAM_DEFS[static_cast<uint8_t>(id)];
}

inline bool isValidAppParamID(AppParamID id) {
  return static_cast<uint8_t>(id) < HOST_PARAM_COUNT;
}

inline bool isTrackScoped(AppParamID id) {
  return getAppParamDef(id).scope == AppParamScope::Track;
}

inline float clampAppParam(AppParamID id, float value) {
  const auto& def = getAppParamDef(id);
  if (value < def.min)
    return def.min;
  if (value > def.max)
    return def.max;
  return value;
}

inline const AppParamDef* findAppParamByTableField(const char* table, const char* field) {
  for (const auto& def : APP_PARAM_DEFS) {
    if (strcmp(def.table, table) == 0 && strcmp(def.field, field) == 0)
      return &def;
  }
  return nullptr;
}

inline const AppParamDef* findAppParamByPath(const char* path) {
  for (const auto& def : APP_PARAM_DEFS) {
    if (strcmp(def.path, path) == 0)
      return &def;
  }
  return nullptr;
}

VoidResult initAppParams(AppContext* ctx);
FloatResult getAppParamValue(const AppContext* ctx, AppParamID id, uint8_t track = 0);
VoidResult applyAppParam(AppContext* ctx, AppParamID id, float publicValue, uint8_t track = 0);

} // namespace app::params

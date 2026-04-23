#include "AppParams.h"

#include "app/AppContext.h"
#include "app/Constants.h"
#include "app/Types.h"

#include "dsp/Math.h"

namespace app::params {

namespace {

VoidResult validiateTrackIndex(uint8_t track) {
  if (track < MAX_TRACKS)
    return {true, nullptr};

  return {false, "track out of range"};
}

} // namespace

VoidResult applyAppParam(AppContext* ctx, AppParamID id, float publicValue, uint8_t track) {
  if (!ctx || !isValidAppParamID(id))
    return {false, "invalid param id"};

  const float value = clampAppParam(id, publicValue);
  auto res = validiateTrackIndex(track);
  if (!res.ok)
    return res;

  switch (id) {
  case AppParamID::TrackGain:
    ctx->mixer.tracks[track].gain = value;
    break;

  case AppParamID::TrackPan:
    ctx->mixer.tracks[track].pan = value;
    break;

  case AppParamID::TrackMute:
    ctx->mixer.tracks[track].enabled = value < 0.5f;
    break;

  case AppParamID::MasterGain:
    ctx->mixer.masterGain = value;
    break;

  case AppParamID::LimiterThresholdDB:
    ctx->mixer.limiterThreshold = dsp::math::dBToLinear(value);
    break;

  case AppParamID::Count:
    break;
  }

  return res;
}

FloatResult getAppParamValue(const AppContext* ctx, AppParamID id, uint8_t track) {
  FloatResult res{};

  if (!ctx) {
    res.ok = false;
    res.err = "app context not found";
    return res;
  }

  if (!isValidAppParamID(id)) {
    res.ok = false;
    res.err = "invalid param id";
    return res;
  }

  if ((res.err = validiateTrackIndex(track).err)) {
    res.ok = false;
    return res;
  }

  switch (id) {
  case AppParamID::TrackGain:
    res.value = ctx->mixer.tracks[track].gain;
    break;

  case AppParamID::TrackPan:
    return {ctx->mixer.tracks[track].pan, true, nullptr};
    break;

  case AppParamID::TrackMute:
    res.value = ctx->mixer.tracks[track].enabled ? 0.0f : 1.0f;
    break;

  case AppParamID::MasterGain:
    res.value = ctx->mixer.masterGain;
    break;

  case AppParamID::LimiterThresholdDB:
    res.value = dsp::math::linearTodB(ctx->mixer.limiterThreshold);
    break;

  case AppParamID::Count:
    break;
  }

  return res;
}

VoidResult initAppParams(AppContext* ctx) {
  VoidResult res{};
  if (!ctx) {
    res.ok = false;
    res.err = "app context not found";
    return res;
  }

  float defaultValue{};

  // ==== Initialize track params ====
  for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
    defaultValue = getAppParamDef(AppParamID::TrackGain).defaultValue;
    if ((res.err = applyAppParam(ctx, AppParamID::TrackGain, defaultValue, track).err)) {
      res.ok = false;
      return res;
    }

    defaultValue = getAppParamDef(AppParamID::TrackPan).defaultValue;
    if ((res.err = applyAppParam(ctx, AppParamID::TrackPan, defaultValue, track).err)) {
      res.ok = false;
      return res;
    }

    defaultValue = getAppParamDef(AppParamID::TrackMute).defaultValue;
    if ((res.err = applyAppParam(ctx, AppParamID::TrackMute, defaultValue, track).err)) {
      res.ok = false;
      return res;
    }
  }

  // ==== Initialize mixer params ====
  defaultValue = getAppParamDef(AppParamID::MasterGain).defaultValue;
  if ((res.err = applyAppParam(ctx, AppParamID::MasterGain, defaultValue).err)) {
    res.ok = false;
    return res;
  };

  defaultValue = getAppParamDef(AppParamID::LimiterThresholdDB).defaultValue;
  if ((res.err = applyAppParam(ctx, AppParamID::LimiterThresholdDB, defaultValue).err)) {
    res.ok = false;
    return res;
  }

  return res;
}

} // namespace app::params

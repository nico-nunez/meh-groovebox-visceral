#include "ParserHelpers.h"

namespace app::doc {

std::string mixerTarget(uint8_t trackIndex, const char* authoredField) {
  std::string target = "mixer:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  if (authoredField && authoredField[0] != '\0') {
    target += ".";
    target += authoredField;
  }
  return target;
}

bool appendMixerWrite(LuaSequencerParseContext& ctx,
                      AuthoredTrackMixerPatch& patch,
                      const AuthoredMixerParamField& field,
                      float value,
                      SourceSpan span) {
  for (const auto& existing : patch.writes) {
    if (existing.paramID != field.paramID)
      continue;
    if (existing.value == value)
      return true;
    const std::string target = mixerTarget(patch.trackIndex, field.authoredField);
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerParamDuplicateWrite,
                   "mixer param written more than once",
                   span,
                   target.c_str());
    return false;
  }
  patch.writes.push_back({field.paramID, value, &field, span});
  return true;
}

bool parseMixerScalarValue(lua_State* L,
                           int valueIndex,
                           LuaSequencerParseContext& ctx,
                           uint8_t trackIndex,
                           const AuthoredMixerParamField& field,
                           SourceSpan span,
                           float& outValue) {
  namespace ap = app::params;
  const auto& def = ap::getAppParamDef(field.paramID);
  const std::string target = mixerTarget(trackIndex, field.authoredField);

  switch (field.valueKind) {
  case DocLuaValueKind::Boolean:
    if (!lua_isboolean(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::MixerParamTypeMismatch,
                     "mixer param must be a boolean",
                     span,
                     target.c_str());
      return false;
    }
    outValue = lua_toboolean(L, valueIndex) ? 1.0f : 0.0f;
    return true;

  case DocLuaValueKind::Number:
    if (!finiteNumber(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::MixerParamTypeMismatch,
                     "mixer param must be a finite number",
                     span,
                     target.c_str());
      return false;
    }
    outValue = static_cast<float>(lua_tonumber(L, valueIndex));
    break;

  default:
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerParamTypeMismatch,
                   "unsupported mixer param value kind",
                   span,
                   target.c_str());
    return false;
  }

  if (outValue < def.min || outValue > def.max) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerParamOutOfRange,
                   "mixer param value out of range",
                   span,
                   target.c_str());
    return false;
  }

  return true;
}

bool parseMixerSettingsForTrack(lua_State* L,
                                int settingsIndex,
                                LuaSequencerParseContext& ctx,
                                uint8_t trackIndex,
                                SourceSpan span) {
  if (!lua_istable(L, settingsIndex)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerSettingsInvalidShape,
                   "MixerSettings must be a table",
                   span,
                   "mixer");
    return false;
  }

  ctx.model->hasMixerState[trackIndex] = true;
  AuthoredTrackMixerPatch& patch = ctx.model->mixerTracks[trackIndex];
  patch.hasPatch = true;
  patch.trackIndex = trackIndex;
  patch.trackSpan = span;

  const int absIndex = lua_absindex(L, settingsIndex);
  bool ok = true;

  lua_pushnil(L);
  while (lua_next(L, absIndex) != 0) {
    if (!lua_isstring(L, -2)) {
      const std::string target = mixerTarget(trackIndex, "");
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::MixerParamUnknown,
                     "mixer field key must be a string",
                     span,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const char* key = lua_tostring(L, -2);
    const auto* field = findAuthoredTrackMixerParamField(key);
    if (!field) {
      const std::string target = mixerTarget(trackIndex, key);
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::MixerParamUnknown,
                     "unknown or deferred mixer param",
                     span,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    float value = 0.0f;
    if (parseMixerScalarValue(L, lua_absindex(L, -1), ctx, trackIndex, *field, span, value))
      ok = appendMixerWrite(ctx, patch, *field, value, span) && ok;
    else
      ok = false;

    lua_pop(L, 1);
  }

  return ok;
}

int l_captureMixer(lua_State* L) {
  auto* ctx = static_cast<LuaSequencerParseContext*>(lua_touserdata(L, lua_upvalueindex(1)));
  if (!ctx)
    return luaL_error(L, "missing authored document parse context");

  const SourceSpan span = currentLuaCallSpan(L);

  if (!lua_isinteger(L, 1)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerTrackInvalidIndex,
                   "mixer track index must be an integer",
                   span,
                   "mixer");
    return 0;
  }

  const int trackNumber = static_cast<int>(lua_tointeger(L, 1));
  if (trackNumber < 1 || trackNumber > static_cast<int>(app::MAX_TRACKS)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerTrackInvalidIndex,
                   "mixer track index out of range",
                   span,
                   "mixer");
    return 0;
  }

  if (!lua_istable(L, 2)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::MixerSettingsInvalidShape,
                   "mixer settings must be a table",
                   span,
                   "mixer");
    return 0;
  }

  const uint8_t trackIndex = static_cast<uint8_t>(trackNumber - 1);
  parseMixerSettingsForTrack(L, 2, *ctx, trackIndex, span);
  return 0;
}
} // namespace app::doc

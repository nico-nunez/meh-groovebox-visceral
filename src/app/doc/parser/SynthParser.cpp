#include "ParserHelpers.h"

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/metadata/DocMetadata.h"

#include "lua.h"
#include "synth/ModMatrix.h"
#include "synth/WavetableOsc.h"
#include "synth/params/ParamUtils.h"

namespace app::doc {

namespace {
using synth::wavetable::osc::FMCarrier;
using synth::wavetable::osc::FMSource;

std::string synthTarget(uint8_t trackIndex, const char* authoredPath) {
  std::string target = "synth:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  if (authoredPath && authoredPath[0] != '\0') {
    target += ".";
    target += authoredPath;
  }
  return target;
}

bool parseModRoutesSection(lua_State* L,
                           int tableIndex,
                           LuaSequencerParseContext& ctx,
                           AuthoredTrackSynthPatch& patch,
                           SourceSpan span) {
  patch.hasModRoutes = true;

  if (!lua_istable(L, tableIndex)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthModRouteInvalidShape,
                   "modMatrix must be a table",
                   span,
                   synthTarget(patch.trackIndex, "modMatrix").c_str());
    return false;
  }

  const int absIndex = lua_absindex(L, tableIndex);
  const int count = static_cast<int>(lua_rawlen(L, absIndex));

  if (count > synth::mod_matrix::MAX_MOD_ROUTES) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthModRouteCapacityExceeded,
                   "too many modMatrix routes",
                   span,
                   synthTarget(patch.trackIndex, "modMatrix").c_str());
    return false;
  }

  bool ok = true;
  for (int i = 1; i <= count; ++i) {
    lua_rawgeti(L, absIndex, i);
    if (!lua_istable(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthModRouteInvalidShape,
                     "each modMatrix route must be a table",
                     span,
                     synthTarget(patch.trackIndex, "modMatrix").c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const int routeIndex = lua_absindex(L, -1);

    // src
    lua_getfield(L, routeIndex, "src");
    if (!lua_isstring(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthModRouteInvalidSrc,
                     "modMatrix route src must be a string",
                     span,
                     synthTarget(patch.trackIndex, "modMatrix.src").c_str());
      lua_pop(L, 2);
      ok = false;
      continue;
    }
    const auto src = synth::mod_matrix::parseModSrc(lua_tostring(L, -1));
    lua_pop(L, 1);

    if (src == synth::mod_matrix::ModSrc::NoSrc) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthModRouteInvalidSrc,
                     "unknown modMatrix route src",
                     span,
                     synthTarget(patch.trackIndex, "modMatrix.src").c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    // dest
    lua_getfield(L, routeIndex, "dest");
    if (!lua_isstring(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthModRouteInvalidDest,
                     "modMatrix route dest must be a string",
                     span,
                     synthTarget(patch.trackIndex, "modMatrix.dest").c_str());
      lua_pop(L, 2);
      ok = false;
      continue;
    }
    const auto dest = synth::mod_matrix::parseModDest(lua_tostring(L, -1));
    lua_pop(L, 1);

    if (dest == synth::mod_matrix::ModDest::NoDest) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthModRouteInvalidDest,
                     "unknown modMatrix route dest",
                     span,
                     synthTarget(patch.trackIndex, "modMatrix.dest").c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    // amount
    lua_getfield(L, routeIndex, "amount");
    if (!finiteNumber(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthModRouteInvalidShape,
                     "modMatrix route amount must be a finite number",
                     span,
                     synthTarget(patch.trackIndex, "modMatrix.amount").c_str());
      lua_pop(L, 2);
      ok = false;
      continue;
    }
    const float amount = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    patch.modRoutes.push_back({src, dest, amount, span});
    lua_pop(L, 1); // pop route table
  }

  return ok;
}

bool parseFMRoutesSection(lua_State* L,
                          int tableIndex,
                          LuaSequencerParseContext& ctx,
                          AuthoredTrackSynthPatch& patch,
                          SourceSpan span) {
  patch.hasFMRoutes = true;

  if (!lua_istable(L, tableIndex)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthFMRouteInvalidShape,
                   "fmRoutes must be a table",
                   span,
                   synthTarget(patch.trackIndex, "fmRoutes").c_str());
    return false;
  }

  const int absIndex = lua_absindex(L, tableIndex);
  const int count = static_cast<int>(lua_rawlen(L, absIndex));
  const int maxRoutes = synth::preset::NUM_OSCS * synth::preset::NUM_OSCS;

  if (count > maxRoutes) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthFMRouteCapacityExceeded,
                   "too many fm routes",
                   span,
                   synthTarget(patch.trackIndex, "fmRoutes").c_str());
    return false;
  }

  bool ok = true;
  for (int i = 1; i <= count; ++i) {
    lua_rawgeti(L, absIndex, i);
    if (!lua_istable(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthFMRouteInvalidShape,
                     "each fm route must be a table",
                     span,
                     synthTarget(patch.trackIndex, "fmRoutes").c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const int routeIndex = lua_absindex(L, -1);

    // carrier
    lua_getfield(L, routeIndex, "carrier");
    if (!lua_isstring(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthFMRouteInvalidCarrier,
                     "fm route carrier must be a string (\"osc1\"–\"osc4\")",
                     span,
                     synthTarget(patch.trackIndex, "fmRoutes.carrier").c_str());
      lua_pop(L, 2);
      ok = false;
      continue;
    }
    const auto carrierSrc = synth::wavetable::osc::parseFMSource(lua_tostring(L, -1));
    lua_pop(L, 1);

    if (carrierSrc == synth::wavetable::osc::FMSource::None) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthFMRouteInvalidCarrier,
                     "unknown fm route carrier",
                     span,
                     synthTarget(patch.trackIndex, "fmRoutes.carrier").c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    // mod
    lua_getfield(L, routeIndex, "mod");
    if (!lua_isstring(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthFMRouteInvalidMod,
                     "fm route mod must be a string (\"osc1\"–\"osc4\")",
                     span,
                     synthTarget(patch.trackIndex, "fmRoutes.mod").c_str());
      lua_pop(L, 2);
      ok = false;
      continue;
    }
    const auto modSrc = synth::wavetable::osc::parseFMSource(lua_tostring(L, -1));
    lua_pop(L, 1);

    if (modSrc == synth::wavetable::osc::FMSource::None) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthFMRouteInvalidMod,
                     "unknown fm route mod",
                     span,
                     synthTarget(patch.trackIndex, "fmRoutes.mod").c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    if (carrierSrc == modSrc) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthFMRouteSelfMod,
                     "fm route carrier and mod must differ",
                     span,
                     synthTarget(patch.trackIndex, "fmRoutes").c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    // depth
    lua_getfield(L, routeIndex, "depth");
    if (!finiteNumber(L, -1)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthFMRouteInvalidShape,
                     "fm route depth must be a finite number",
                     span,
                     synthTarget(patch.trackIndex, "fmRoutes.depth").c_str());
      lua_pop(L, 2);
      ok = false;
      continue;
    }
    const float depth = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    // FMSource enum is 1-based (None=0, Osc1=1...); store as 0-based indices in AuthoredFMRoute
    const FMCarrier carrierIdx = carrierSrc;
    const FMSource modulatorIdx = modSrc;
    patch.fmRoutes.push_back({carrierIdx, modulatorIdx, depth, span});
    lua_pop(L, 1); // pop route table
  }

  return ok;
}

bool parseSignalChainSection(lua_State* L,
                             int tableIndex,
                             LuaSequencerParseContext& ctx,
                             AuthoredTrackSynthPatch& patch,
                             SourceSpan span) {
  patch.hasSignalChain = true;

  if (!lua_istable(L, tableIndex)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthSignalChainInvalidShape,
                   "signalChain must be a table",
                   span,
                   synthTarget(patch.trackIndex, "signalChain").c_str());
    return false;
  }

  const int absIndex = lua_absindex(L, tableIndex);
  const int count = static_cast<int>(lua_rawlen(L, absIndex));

  if (count > synth::signal_chain::MAX_CHAIN_SLOTS) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthSignalChainCapacityExceeded,
                   "too many signal chain processors",
                   span,
                   synthTarget(patch.trackIndex, "signalChain").c_str());
    return false;
  }

  bool ok = true;
  for (int i = 1; i <= count; ++i) {
    lua_rawgeti(L, absIndex, i);
    if (lua_type(L, -1) != LUA_TSTRING) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthSignalChainInvalidShape,
                     "signal chain entry must be a string",
                     span,
                     synthTarget(patch.trackIndex, "signalChain").c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const auto proc = synth::signal_chain::parseSignalProcessor(lua_tostring(L, -1));
    lua_pop(L, 1);

    if (proc == synth::signal_chain::SignalProcessor::None) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthSignalChainUnknownProcessor,
                     "unknown signal chain processor",
                     span,
                     synthTarget(patch.trackIndex, "signalChain").c_str());
      ok = false;
      continue;
    }

    bool isDuplicate = false;
    for (const auto& existing : patch.signalChain) {
      if (existing == proc) {
        isDuplicate = true;
        break;
      }
    }
    if (isDuplicate) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthSignalChainDuplicate,
                     "duplicate signal chain processor",
                     span,
                     synthTarget(patch.trackIndex, "signalChain").c_str());
      ok = false;
      continue;
    }

    patch.signalChain.push_back(proc);
  }

  return ok;
}

bool appendSynthWrite(LuaSequencerParseContext& ctx,
                      AuthoredTrackSynthPatch& patch,
                      const AuthoredSynthParamField& field,
                      float value,
                      SourceSpan span) {
  for (const auto& existing : patch.writes) {
    if (existing.paramID != field.paramID)
      continue;

    if (existing.value == value)
      return true;

    const std::string target = synthTarget(patch.trackIndex, field.authoredPath);
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthParamDuplicateWrite,
                   "synth param written more than once",
                   span,
                   target.c_str());
    return false;
  }

  patch.writes.push_back({field.paramID, value, &field, span});
  return true;
}

bool parseSynthScalarValue(lua_State* L,
                           int valueIndex,
                           LuaSequencerParseContext& ctx,
                           uint8_t trackIndex,
                           const AuthoredSynthParamField& field,
                           SourceSpan span,
                           float& outValue) {
  const auto& def = synth::param::PARAM_DEFS[field.paramID];
  const std::string target = synthTarget(trackIndex, field.authoredPath);

  switch (field.valueKind) {
  case DocLuaValueKind::Boolean:
    if (!lua_isboolean(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamTypeMismatch,
                     "synth param must be a boolean",
                     span,
                     target.c_str());
      return false;
    }
    outValue = lua_toboolean(L, valueIndex) ? 1.0f : 0.0f;
    return true;

  case DocLuaValueKind::Integer:
    if (!lua_isinteger(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamTypeMismatch,
                     "synth param must be an integer",
                     span,
                     target.c_str());
      return false;
    }
    outValue = static_cast<float>(lua_tointeger(L, valueIndex));
    break;

  case DocLuaValueKind::Number:
    if (!finiteNumber(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamTypeMismatch,
                     "synth param must be a finite number",
                     span,
                     target.c_str());
      return false;
    }
    outValue = static_cast<float>(lua_tonumber(L, valueIndex));
    break;

  case DocLuaValueKind::String: {
    if (!lua_isstring(L, valueIndex)) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamTypeMismatch,
                     "synth enum param must be a string",
                     span,
                     target.c_str());
      return false;
    }
    const char* token = lua_tostring(L, valueIndex);
    auto parsed = synth::param::utils::parseEnum(def.type, token);
    if (!parsed.ok) {
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamEnumUnknown,
                     parsed.error ? parsed.error : "unknown synth enum token",
                     span,
                     target.c_str());
      return false;
    }
    outValue = static_cast<float>(parsed.value);
    break;
  }

  default:
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthParamTypeMismatch,
                   "unsupported synth param value kind",
                   span,
                   target.c_str());
    return false;
  }

  if (outValue < def.min || outValue > def.max) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthParamOutOfRange,
                   "synth param value out of range",
                   span,
                   target.c_str());
    return false;
  }

  return true;
}

bool parseSynthGroup(lua_State* L,
                     int tableIndex,
                     LuaSequencerParseContext& ctx,
                     AuthoredTrackSynthPatch& patch,
                     const std::string& prefix,
                     SourceSpan span) {
  const int absTableIndex = lua_absindex(L, tableIndex);
  bool ok = true;

  lua_pushnil(L);
  while (lua_next(L, absTableIndex) != 0) {
    if (!lua_isstring(L, -2)) {
      const std::string target = synthTarget(patch.trackIndex, prefix.c_str());
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamUnknown,
                     "synth field key must be a string",
                     span,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    const char* key = lua_tostring(L, -2);

    if (prefix.empty() &&
        (std::strcmp(key, "modMatrix") == 0 || std::strcmp(key, "fmRoutes") == 0 ||
         std::strcmp(key, "signalChain") == 0)) {
      lua_pop(L, 1);
      continue;
    }

    std::string path = prefix.empty() ? key : prefix + "." + key;

    if (lua_istable(L, -1)) {
      ok = parseSynthGroup(L, lua_absindex(L, -1), ctx, patch, path, span) && ok;
      lua_pop(L, 1);
      continue;
    }

    const auto* field = findAuthoredSynthParamField(path.c_str());
    if (!field) {
      const std::string target = synthTarget(patch.trackIndex, path.c_str());
      pushDiagnostic(ctx,
                     DiagnosticSource::Validator,
                     docdiag::SynthParamUnknown,
                     "unknown or deferred synth param",
                     span,
                     target.c_str());
      lua_pop(L, 1);
      ok = false;
      continue;
    }

    float value = 0.0f;
    if (parseSynthScalarValue(L, lua_absindex(L, -1), ctx, patch.trackIndex, *field, span, value))
      ok = appendSynthWrite(ctx, patch, *field, value, span) && ok;
    else
      ok = false;

    lua_pop(L, 1);
  }

  return ok;
}

} // namespace

bool parseSynthSettingsForTrack(lua_State* L,
                                int settingsIndex,
                                LuaSequencerParseContext& ctx,
                                uint8_t trackIndex,
                                SourceSpan span) {
  if (!lua_istable(L, settingsIndex)) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthSettingsInvalidShape,
                   "SynthSettings must be a table",
                   span,
                   "synth");
    return false;
  }

  ctx.model->hasSynthState[trackIndex] = true;
  AuthoredTrackSynthPatch& patch = ctx.model->synthTracks[trackIndex];
  patch.hasPatch = true;
  patch.trackIndex = trackIndex;
  patch.trackSpan = span;

  bool ok = true;

  lua_getfield(L, settingsIndex, "modMatrix");
  if (!lua_isnil(L, -1))
    ok = parseModRoutesSection(L, lua_absindex(L, -1), ctx, patch, span) && ok;
  lua_pop(L, 1);

  lua_getfield(L, settingsIndex, "fmRoutes");
  if (!lua_isnil(L, -1))
    ok = parseFMRoutesSection(L, lua_absindex(L, -1), ctx, patch, span) && ok;
  lua_pop(L, 1);

  lua_getfield(L, settingsIndex, "signalChain");
  if (!lua_isnil(L, -1))
    ok = parseSignalChainSection(L, lua_absindex(L, -1), ctx, patch, span) && ok;
  lua_pop(L, 1);

  // Scalar params — parseSynthGroup skips "modMatrix", "fmRoutes", "signalChain" at top level
  return parseSynthGroup(L, settingsIndex, ctx, patch, "", span) && ok;
}

int l_captureSynth(lua_State* L) {
  auto* ctx = static_cast<LuaSequencerParseContext*>(lua_touserdata(L, lua_upvalueindex(1)));
  if (!ctx)
    return luaL_error(L, "missing authored document parse context");

  const SourceSpan span = currentLuaCallSpan(L);

  if (!lua_isinteger(L, 1)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthTrackInvalidIndex,
                   "synth track index must be an integer",
                   span,
                   "synth");
    return 0;
  }

  const int trackNumber = static_cast<int>(lua_tointeger(L, 1));
  if (trackNumber < 1 || trackNumber > static_cast<int>(app::MAX_TRACKS)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthTrackInvalidIndex,
                   "synth track index out of range",
                   span,
                   "synth");
    return 0;
  }

  if (!lua_istable(L, 2)) {
    pushDiagnostic(*ctx,
                   DiagnosticSource::Validator,
                   docdiag::SynthSettingsInvalidShape,
                   "synth settings must be a table",
                   span,
                   "synth");
    return 0;
  }

  const uint8_t trackIndex = static_cast<uint8_t>(trackNumber - 1);
  parseSynthSettingsForTrack(L, 2, *ctx, trackIndex, span);
  return 0;
}

} // namespace app::doc

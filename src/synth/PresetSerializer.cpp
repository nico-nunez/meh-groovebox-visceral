#include "PresetSerializer.h"

#include "synth/FXChain.h"
#include "synth/ModMatrix.h"
#include "synth/ParamDefs.h"
#include "synth/Preset.h"
#include "synth/Tempo.h"

#include "dsp/FX/Distortion.h"
#include "json/Json.h"

#include <cstdint>
#include <cstdio>
#include <string>

namespace synth::preset {

namespace osc = wavetable::osc;
namespace banks = wavetable::banks;

using JsonValue = json::Value;

// ============================================================
// Serialize helpers
// ============================================================

namespace {

struct JsonGroup {
  const char* prefix; // param name prefix, e.g. "osc1." (deserialized)
  const char* parent; // JSON parent key (nullptr = root-level)
  const char* key;    // JSON key within parent (nullptr = no sub-key)
};

constexpr JsonGroup JSON_GROUPS[] = {
    {"osc1.", "oscillators", OSC_KEYS[0]},
    {"osc2.", "oscillators", OSC_KEYS[1]},
    {"osc3.", "oscillators", OSC_KEYS[2]},
    {"osc4.", "oscillators", OSC_KEYS[3]},
    {"noise.", "oscillators", "noise"},
    {"ampEnv.", "envelopes", "ampEnv"},
    {"filterEnv.", "envelopes", "filterEnv"},
    {"modEnv.", "envelopes", "modEnv"},
    {"svf.", "filters", "svf"},
    {"ladder.", "filters", "ladder"},
    {"saturator.", "voice", "saturator"},
    {"lfo1.", "lfos", LFO_KEYS[0]},
    {"lfo2.", "lfos", LFO_KEYS[1]},
    {"lfo3.", "lfos", LFO_KEYS[2]},
    {"pitchBend.", "voice", "pitchBend"},
    {"mono.", "voice", "mono"},
    {"porta.", "voice", "portamento"},
    {"unison.", "voice", "unison"},
    {"fx.distortion.", "fx", "distortion"},
    {"fx.chorus.", "fx", "chorus"},
    {"fx.phaser.", "fx", "phaser"},
    {"fx.delay.", "fx", "delay"},
    {"fx.reverb.", "fx", "reverb"},
};

const JsonGroup* findGroupForParam(const char* paramName) {
  for (auto& group : JSON_GROUPS)
    if (std::strncmp(paramName, group.prefix, std::strlen(group.prefix)) == 0)
      return &group;

  printf("[Warning] group not found for paramName: %s\n", paramName);
  return nullptr;
}

// Navigate to (or create) the target JSON object for a group during serialization.
//
// TODO(nico): is null, null, root even necessary?
// parent | key  | result
// -------|------|-------
// null   | null | root itself         (e.g. master.gain → root directly)
// null   | "x"  | root["x"]           (not currently used)
// "p"    | null | root["p"]           (not currently used)
// "p"    | "x"  | root["p"]["x"]      (most groups)
//
JsonValue& getOrCreateJsonTarget(JsonValue& root, const JsonGroup* group) {
  if (!group->parent && !group->key)
    return root;

  if (!group->parent)
    return root.getOrCreate(group->key);

  if (!group->key)
    return root.getOrCreate(group->parent);

  return root.getOrCreate(group->parent).getOrCreate(group->key);
}

// Navigate to the target JSON object for a group during deserialization (read-only).
// Returns an invalid/null Value if the path doesn't exist in the JSON.
const JsonValue& resolveJsonObject(const JsonValue& root, const JsonGroup* group) {
  static const JsonValue null{};

  if (!group->parent && !group->key)
    return root;

  if (!group->parent)
    return root.has(group->key) ? root[group->key] : null;

  if (!group->key)
    return root.has(group->parent) ? root[group->parent] : null;

  if (!root.has(group->parent))
    return null;

  const auto& parent = root[group->parent];
  return parent.has(group->key) ? parent[group->key] : null;
}

// ========== Clamp Helpers ==============

// Clamp float to [min, max]. If clamped, append warning.
float clampWarnFloat(float val,
                     float min,
                     float max,
                     const char* fieldName,
                     std::vector<std::string>& warnings) {
  if (val < min) {
    warnings.push_back(std::string(fieldName) + ": clamped " + std::to_string(val) + " to min " +
                       std::to_string(min));
    return min;
  }
  if (val > max) {
    warnings.push_back(std::string(fieldName) + ": clamped " + std::to_string(val) + " to max " +
                       std::to_string(max));
    return max;
  }
  return val;
}

int8_t clampWarnInt8(int8_t val,
                     int8_t min,
                     int8_t max,
                     const char* fieldName,
                     std::vector<std::string>& warnings) {
  if (val < min) {
    warnings.push_back(std::string(fieldName) + ": clamped " + std::to_string(val) + " to min " +
                       std::to_string(min));
    return min;
  }
  if (val > max) {
    warnings.push_back(std::string(fieldName) + ": clamped " + std::to_string(val) + " to max " +
                       std::to_string(max));
    return max;
  }
  return val;
}

} // anonymous namespace

// ============================================================
// Serialize Preset
// ============================================================

std::string serializePreset(const Preset& p) {
  auto root = JsonValue::object();
  root.set("version", JsonValue::number(p.version));

  {
    auto meta = JsonValue::object();
    meta.set("name", JsonValue::string(p.metadata.name.c_str()));
    meta.set("author", JsonValue::string(p.metadata.author.c_str()));
    meta.set("category", JsonValue::string(p.metadata.category.c_str()));
    meta.set("description", JsonValue::string(p.metadata.description.c_str()));
    root.set("metadata", std::move(meta));
  }

  root.set("bpm", JsonValue::number(p.paramValues[param::BPM]));

  // All numeric (X-macro) params
  for (int i = 0; i < param::PARAM_COUNT - 1; i++) {
    const auto& def = param::PARAM_DEFS[i];
    float value = p.paramValues[i];

    const JsonGroup* group = findGroupForParam(def.name);
    if (!group)
      continue;

    JsonValue& target = getOrCreateJsonTarget(root, group);
    const char* fieldName = def.name + strlen(group->prefix);

    switch (def.type) {
    case param::ParamType::Float:
      target.set(fieldName, JsonValue::number(value));
      break;

    case param::ParamType::Int8:
      target.set(fieldName, JsonValue::number(static_cast<int8_t>(std::round(value))));
      break;

    case param::ParamType::Bool:
      target.set(fieldName, JsonValue::boolean(value >= 0.5f));
      break;

    default:
      break;
    }
  }

  // Enum fields: convert to strings and insert into the JSON objects
  for (int i = 0; i < NUM_OSCS; i++) {
    auto& oscObj = root.getOrCreate("oscillators").getOrCreate(OSC_KEYS[i]);
    oscObj.set("bank", JsonValue::string(banks::bankIDToString(p.oscBanks[i])));
    oscObj.set("fmSource", JsonValue::string(osc::fmSourceToString(p.oscFmSources[i])));
  }

  root.getOrCreate("oscillators")
      .getOrCreate("noise")
      .set("type", JsonValue::string(noise::noiseTypeToString(p.noiseType)));

  // SVF mode: overwrite the int written by the param loop with the string form
  root.getOrCreate("filters").getOrCreate("svf").set("mode",
                                                     JsonValue::string(
                                                         filters::svfModeToString(p.svfMode)));

  for (int i = 0; i < NUM_LFOS; i++) {
    root.getOrCreate("lfos")
        .getOrCreate(LFO_KEYS[i])
        .set("bank", JsonValue::string(banks::bankIDToString(p.lfoBanks[i])));
  }

  for (int i = 0; i < NUM_LFOS; i++) {
    root.getOrCreate("lfos")
        .getOrCreate(LFO_KEYS[i])
        .set("subdivision", JsonValue::string(tempo::subdivisionToString(p.lfoSubdivisions[i])));
  }

  // Mod matrix: create array of mappings
  {
    auto arr = JsonValue::array();
    for (uint8_t i = 0; i < p.modMatrixCount; i++) {
      const auto& route = p.modMatrix[i];
      auto modRoot = JsonValue::object();
      modRoot.set("source", JsonValue::string(route.source.c_str()));
      modRoot.set("destination", JsonValue::string(route.destination.c_str()));
      modRoot.set("amount", JsonValue::number(route.amount));
      arr.push(std::move(modRoot));
    }
    root.set("modMatrix", std::move(arr));
  }

  // Signal chain: create array preserving order of Signal Processors within chain
  {
    auto arr = JsonValue::array();
    for (const auto& proc : p.signalChain) {
      if (proc == SignalProcessor::None)
        break;
      arr.push(JsonValue::string(signal_chain::signalProcessorToString(proc)));
    }
    root.set("signalChain", std::move(arr));
  }

  // FX chain
  {
    namespace dist = dsp::fx::distortion;

    auto arr = JsonValue::array();
    for (uint8_t i = 0; i < p.fxChainLength; i++) {
      if (p.fxChain[i] == FXProcessor::None)
        break;
      arr.push(JsonValue::string(fxProcessorToString(p.fxChain[i])));
    }
    root.set("fxChain", std::move(arr));

    // handle enum
    root.getOrCreate("fx")
        .getOrCreate("distortion")
        .set("type",
             JsonValue::string(dist::distortionTypeToString(static_cast<dist::DistortionType>(
                 static_cast<uint8_t>(std::round(p.paramValues[param::FX_DISTORTION_TYPE]))))));
  }

  return json::serialize(root);
}

// ============================================================
// deserializePreset
// ============================================================

DeserializeResult deserializePreset(const std::string& jsonStr) {
  DeserializeResult result;

  auto parsed = json::parse(jsonStr);
  if (!parsed.ok()) {
    result.error = "JSON parse error: " + parsed.error;
    return result;
  }

  const auto& root = parsed.value;
  if (!root.isObject()) {
    result.error = "Expected root JSON object";
    return result;
  }

  Preset& p = result.preset;
  p = createInitPreset(); // start from defaults so missing fields get init values

  // Version
  if (root.has("version")) {
    p.version = static_cast<uint32_t>(root["version"].asInt());
    if (p.version > CURRENT_PRESET_VERSION)
      result.warnings.push_back("Preset version newer than supported");
    // TODO(nico): version migration
  }

  // Parse Metadata
  {
    const auto& meta = root["metadata"];
    if (meta.isObject()) {
      if (meta.has("name"))
        p.metadata.name = meta["name"].asString();
      if (meta.has("author"))
        p.metadata.author = meta["author"].asString();
      if (meta.has("category"))
        p.metadata.category = meta["category"].asString();
      if (meta.has("description"))
        p.metadata.description = meta["description"].asString();
    }
  }

  if (root.has("bpm")) {
    auto& def = param::PARAM_DEFS[param::ParamID::BPM];
    p.paramValues[param::BPM] =
        clampWarnFloat(root["bpm"].asFloat(), def.min, def.max, "bpm", result.warnings);
  }

  // All numeric (X-macro) params
  for (int i = 0; i < param::PARAM_COUNT - 1; i++) {
    const auto& def = param::PARAM_DEFS[i];

    const JsonGroup* group = findGroupForParam(def.name);
    if (!group) {
      std::string warn = "Group not found for param: ";
      result.warnings.push_back(warn + def.name);
      continue;
    }

    const JsonValue& jsonObj = resolveJsonObject(root, group);
    const char* fieldName = def.name + strlen(group->prefix);

    if (!jsonObj.isObject() || !jsonObj.has(fieldName)) {
      std::string warn = "jsonObj does not contain fieldName: ";
      result.warnings.push_back(warn + fieldName);
      continue;
    }

    switch (def.type) {
    case param::ParamType::Float:
      p.paramValues[i] =
          clampWarnFloat(jsonObj[fieldName].asFloat(), def.min, def.max, def.name, result.warnings);
      break;

    case param::ParamType::Int8: {
      p.paramValues[i] = clampWarnInt8(jsonObj[fieldName].asInt8(),
                                       static_cast<int8_t>(def.min),
                                       static_cast<int8_t>(def.max),
                                       def.name,
                                       result.warnings);
      break;
    }
    case param::ParamType::Bool:
      p.paramValues[i] = jsonObj[fieldName].asBool() ? 1.0f : 0.0f;
      break;

    default:
      break;
    }
  }

  // ==== Enum fields: parse strings to enums ====

  if (root.has("oscillators") && root["oscillators"].isObject()) {
    for (int i = 0; i < NUM_OSCS; i++) {
      const auto& oscObj = root["oscillators"][OSC_KEYS[i]];
      if (!oscObj.isObject())
        continue;

      if (oscObj.has("bank")) {
        auto id = banks::parseBankID(oscObj["bank"].asString().c_str());
        if (id == BankID::Unknown) {
          result.warnings.push_back(std::string(OSC_KEYS[i]) + ".bank: unknown, using sine");
          id = BankID::Sine;
        }
        p.oscBanks[i] = id;
      }

      if (oscObj.has("fmSource"))
        p.oscFmSources[i] = osc::parseFMSource(oscObj["fmSource"].asString().c_str());
    }
  }

  if (root.has("oscillators") && root["oscillators"].has("noise")) {
    const auto& n = root["oscillators"]["noise"];

    if (n.has("type")) {
      auto t = noise::parseNoiseType(n["type"].asString().c_str());

      if (t == NoiseType::Unknown) {
        result.warnings.push_back("noise.type: unknown, using white");
        t = NoiseType::White;
      }
      p.noiseType = t;
    }
  }

  // SVF mode: update both the enum field and paramValues[SVF_MODE] — they must stay in sync
  if (root.has("filters") && root["filters"].has("svf")) {
    const auto& svf = root["filters"]["svf"];

    if (svf.has("mode")) {
      auto mode = filters::parseSVFMode(svf["mode"].asString().c_str());

      if (mode == filters::SVFMode::Unknown) {
        result.warnings.push_back("svf.mode: unknown, using lp");
        mode = SVFMode::LP;
      }

      p.svfMode = mode;
      p.paramValues[param::SVF_MODE] = static_cast<float>(mode);
    }
  }

  if (root.has("lfos") && root["lfos"].isObject()) {
    for (int i = 0; i < NUM_LFOS; i++) {
      const auto& lfoObj = root["lfos"][LFO_KEYS[i]];

      if (!lfoObj.isObject())
        continue;

      if (lfoObj.has("bank")) {
        auto id = banks::parseBankID(lfoObj["bank"].asString().c_str());

        if (id == BankID::Unknown) {
          result.warnings.push_back(std::string(LFO_KEYS[i]) + ".bank: unknown, using sine");
          id = BankID::Sine;
        }

        p.lfoBanks[i] = id;
      }

      if (lfoObj.has("subdivision")) {
        auto sub = tempo::parseSubdivision(lfoObj["subdivision"].asString().c_str());

        if (std::strcmp(lfoObj["subdivision"].asString().c_str(),
                        tempo::subdivisionToString(sub)) != 0)
          result.warnings.push_back(std::string(LFO_KEYS[i]) + ".subdivision: unknown, using 1/4");

        p.lfoSubdivisions[i] = sub;
      }
    }
  }

  // Mod matrix: increment modMatrixCount as valid routes are parsed, cap at MAX_MOD_ROUTES
  {
    const auto& mm = root["modMatrix"];
    if (mm.isArray()) {
      for (size_t i = 0; i < mm.size() && p.modMatrixCount < mod_matrix::MAX_MOD_ROUTES; i++) {
        const auto& route = mm.at(i);
        if (!route.isObject())
          continue;

        ModRoutePreset r;
        r.source = route["source"].asString();
        r.destination = route["destination"].asString();

        // Validate source & destination against ModMatrix mapping tables
        bool validSrc = false;
        for (const auto& m : mod_matrix::modSrcMappings) {
          if (r.source == m.name) {
            validSrc = true;
            break;
          }
        }
        if (!validSrc) {
          result.warnings.push_back("modMatrix[" + std::to_string(i) + "]: unknown source '" +
                                    r.source + "', skipping route");
          continue;
        }

        bool validDest = false;
        for (const auto& m : mod_matrix::modDestMappings) {
          if (r.destination == m.name) {
            validDest = true;
            break;
          }
        }
        if (!validDest) {
          result.warnings.push_back("modMatrix[" + std::to_string(i) + "]: unknown destination '" +
                                    r.destination + "', skipping route");
          continue;
        }

        // Valid souce & destination: clamp value
        r.amount = clampWarnFloat(route["amount"].asFloat(),
                                  -24.0f,
                                  24.0f,
                                  ("modMatrix[" + std::to_string(i) + "].amount").c_str(),
                                  result.warnings);

        p.modMatrix[p.modMatrixCount++] = std::move(r);
      }
    }
  }

  // Signal chain: parse strings to SignalProcessor enums
  {
    const auto& chain = root["signalChain"];
    if (chain.isArray()) {
      size_t writeIdx = 0;

      for (size_t i = 0; i < chain.size() && writeIdx < signal_chain::MAX_CHAIN_SLOTS; i++) {
        auto proc = signal_chain::parseSignalProcessor(chain.at(i).asString().c_str());
        if (proc == SignalProcessor::None) {
          result.warnings.push_back("signalChain[" + std::to_string(i) +
                                    "]: unknown processor, skipping");
          continue;
        }

        p.signalChain[writeIdx++] = proc;
      }

      // Ensure unused slots are cleared / set to None
      for (size_t i = writeIdx; i < signal_chain::MAX_CHAIN_SLOTS; i++)
        p.signalChain[i] = SignalProcessor::None;
    }
  }

  // FX chain
  {
    using namespace fx_chain;
    using namespace dsp::fx;

    const auto& chain = root["fxChain"];
    if (chain.isArray()) {
      size_t writeIdx = 0;
      for (size_t i = 0; i < chain.size() && writeIdx < MAX_EFFECT_SLOTS; i++) {
        auto proc = parseFXProcessor(chain.at(i).asString().c_str());
        if (proc == FXProcessor::None) {
          result.warnings.push_back("effectsChain[" + std::to_string(i) + "]: unknown, skipping");
          continue;
        }
        p.fxChain[writeIdx++] = proc;
      }
      p.fxChainLength = static_cast<uint8_t>(writeIdx);
      for (size_t i = writeIdx; i < MAX_EFFECT_SLOTS; i++)
        p.fxChain[i] = FXProcessor::None;
    }

    if (root.has("fx") && root["fx"].has("distortion")) {
      const auto& dist = root["fx"]["distortion"];

      if (dist.has("type")) {
        auto type = distortion::parseDistortionType(dist["type"].asString().c_str());

        if (type == distortion::DistortionType::Unknown) {
          result.warnings.push_back("distortion.type: unknown, using soft");
          type = distortion::DistortionType::Soft;
        }

        p.paramValues[param::FX_DISTORTION_TYPE] = static_cast<float>(type);
      }
    }
  }
  return result;
}

} // namespace synth::preset

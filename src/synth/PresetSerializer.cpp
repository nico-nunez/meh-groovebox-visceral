#include "PresetSerializer.h"

#include "ModMatrix.h"
#include "ParamRanges.h"

#include "json/Json.h"

namespace synth::preset {

// ============================================================
// Serialize helpers
// ============================================================

namespace {

json::Value serializeOsc(const OscPreset& osc) {
  auto obj = json::Value::object();
  obj.set("bank", json::Value::string(osc.bank.c_str()));
  obj.set("scanPos", json::Value::number(osc.scanPos));
  obj.set("mixLevel", json::Value::number(osc.mixLevel));
  obj.set("fmDepth", json::Value::number(osc.fmDepth));
  obj.set("fmRatio", json::Value::number(osc.fmRatio));
  obj.set("fmSource", json::Value::string(osc.fmSource.c_str()));
  obj.set("octaveOffset", json::Value::number(osc.octaveOffset));
  obj.set("detuneAmount", json::Value::number(osc.detuneAmount));
  obj.set("enabled", json::Value::boolean(osc.enabled));
  return obj;
}

json::Value serializeEnvelope(const EnvelopePreset& env) {
  auto obj = json::Value::object();
  obj.set("attackMs", json::Value::number(env.attackMs));
  obj.set("decayMs", json::Value::number(env.decayMs));
  obj.set("sustainLevel", json::Value::number(env.sustainLevel));
  obj.set("releaseMs", json::Value::number(env.releaseMs));
  obj.set("attackCurve", json::Value::number(env.attackCurve));
  obj.set("decayCurve", json::Value::number(env.decayCurve));
  obj.set("releaseCurve", json::Value::number(env.releaseCurve));
  return obj;
}

json::Value serializeLFO(const LFOPreset& lfo) {
  auto obj = json::Value::object();
  obj.set("bank", json::Value::string(lfo.bank.c_str()));
  obj.set("rate", json::Value::number(lfo.rate));
  obj.set("amplitude", json::Value::number(lfo.amplitude));
  obj.set("retrigger", json::Value::boolean(lfo.retrigger));
  return obj;
}

} // anonymous namespace

// ============================================================
// serializePreset
// ============================================================

std::string serializePreset(const Preset& p) {
  auto root = json::Value::object();

  // Version
  root.set("version", json::Value::number(p.version));

  // Metadata
  {
    auto meta = json::Value::object();
    meta.set("name", json::Value::string(p.metadata.name.c_str()));
    meta.set("author", json::Value::string(p.metadata.author.c_str()));
    meta.set("category", json::Value::string(p.metadata.category.c_str()));
    meta.set("description", json::Value::string(p.metadata.description.c_str()));
    root.set("metadata", std::move(meta));
  }

  // Oscillators
  {
    auto oscs = json::Value::object();
    oscs.set("osc1", serializeOsc(p.osc1));
    oscs.set("osc2", serializeOsc(p.osc2));
    oscs.set("osc3", serializeOsc(p.osc3));
    oscs.set("osc4", serializeOsc(p.osc4));
    root.set("oscillators", std::move(oscs));
  }

  // Noise
  {
    auto n = json::Value::object();
    n.set("mixLevel", json::Value::number(p.noise.mixLevel));
    n.set("type", json::Value::string(p.noise.type.c_str()));
    n.set("enabled", json::Value::boolean(p.noise.enabled));
    root.set("noise", std::move(n));
  }

  // Envelopes
  {
    auto envs = json::Value::object();
    envs.set("ampEnv", serializeEnvelope(p.ampEnv));
    envs.set("filterEnv", serializeEnvelope(p.filterEnv));
    envs.set("modEnv", serializeEnvelope(p.modEnv));
    root.set("envelopes", std::move(envs));
  }

  // Filters
  {
    auto filters = json::Value::object();

    auto svf = json::Value::object();
    svf.set("mode", json::Value::string(p.svf.mode.c_str()));
    svf.set("cutoff", json::Value::number(p.svf.cutoff));
    svf.set("resonance", json::Value::number(p.svf.resonance));
    svf.set("enabled", json::Value::boolean(p.svf.enabled));
    filters.set("svf", std::move(svf));

    auto ladder = json::Value::object();
    ladder.set("cutoff", json::Value::number(p.ladder.cutoff));
    ladder.set("resonance", json::Value::number(p.ladder.resonance));
    ladder.set("drive", json::Value::number(p.ladder.drive));
    ladder.set("enabled", json::Value::boolean(p.ladder.enabled));
    filters.set("ladder", std::move(ladder));

    root.set("filters", std::move(filters));
  }

  // Saturator
  {
    auto sat = json::Value::object();
    sat.set("drive", json::Value::number(p.saturator.drive));
    sat.set("mix", json::Value::number(p.saturator.mix));
    sat.set("enabled", json::Value::boolean(p.saturator.enabled));
    root.set("saturator", std::move(sat));
  }

  // LFOs
  {
    auto lfos = json::Value::object();
    lfos.set("lfo1", serializeLFO(p.lfo1));
    lfos.set("lfo2", serializeLFO(p.lfo2));
    lfos.set("lfo3", serializeLFO(p.lfo3));
    root.set("lfos", std::move(lfos));
  }

  // Mod matrix
  {
    auto arr = json::Value::array();
    for (const auto& route : p.modMatrix) {
      auto r = json::Value::object();
      r.set("source", json::Value::string(route.source.c_str()));
      r.set("destination", json::Value::string(route.destination.c_str()));
      r.set("amount", json::Value::number(route.amount));
      arr.push(std::move(r));
    }
    root.set("modMatrix", std::move(arr));
  }

  // Signal chain
  {
    auto arr = json::Value::array();
    for (const auto& proc : p.signalChain) {
      arr.push(json::Value::string(proc.c_str()));
    }
    root.set("signalChain", std::move(arr));
  }

  // Mono
  {
    auto m = json::Value::object();
    m.set("enabled", json::Value::boolean(p.mono.enabled));
    m.set("legato", json::Value::boolean(p.mono.legato));
    root.set("mono", std::move(m));
  }

  // Portamento
  {
    auto porta = json::Value::object();
    porta.set("time", json::Value::number(p.porta.time));
    porta.set("legato", json::Value::boolean(p.porta.legato));
    porta.set("enabled", json::Value::boolean(p.porta.enabled));
    root.set("portamento", std::move(porta));
  }

  // Unison
  {
    auto u = json::Value::object();
    u.set("voices", json::Value::number(p.unison.voices));
    u.set("detune", json::Value::number(p.unison.detune));
    u.set("spread", json::Value::number(p.unison.spread));
    u.set("enabled", json::Value::boolean(p.unison.enabled));
    root.set("unison", std::move(u));
  }

  // Global
  {
    auto g = json::Value::object();
    g.set("pitchBendRange", json::Value::number(p.global.pitchBendRange));
    root.set("global", std::move(g));
  }

  return json::serialize(root);
}

namespace {

// Clamp float to [min, max]. If clamped, append warning.
float clampWarn(float val,
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

// Validate a string field against a list of known values.
// Returns the string if valid, or fallback + warning if not.
std::string validateEnum(const std::string& val,
                         const char* const validValues[],
                         size_t count,
                         const char* fallback,
                         const char* fieldName,
                         std::vector<std::string>& warnings) {
  for (size_t i = 0; i < count; i++) {
    if (val == validValues[i])
      return val;
  }
  warnings.push_back(std::string(fieldName) + ": unknown value '" + val + "', using '" + fallback +
                     "'");
  return fallback;
}

// Known enum values (used by validateEnum)
const char* const BANK_NAMES[] = {"sine", "saw", "square", "triangle", "sine_to_saw"};
const char* const LFO_BANK_NAMES[] = {"sine", "saw", "square", "triangle", "sine_to_saw", "sah"};
const char* const FM_SOURCE_NAMES[] = {"none", "osc1", "osc2", "osc3", "osc4"};
const char* const SVF_MODE_NAMES[] = {"lp", "hp", "bp", "notch"};
const char* const NOISE_TYPE_NAMES[] = {"white", "pink"};
const char* const SIGNAL_PROC_NAMES[] = {"svf", "ladder", "saturator"};

// Mod source/dest validation uses the mapping tables from ModMatrix.h,
// so we validate those inline in deserializeModMatrix.

OscPreset deserializeOsc(const json::Value& obj,
                         const char* oscName,
                         const OscPreset& defaults,
                         std::vector<std::string>& warnings) {
  using namespace param::ranges;
  OscPreset osc = defaults;

  if (!obj.isObject())
    return osc;

  std::string prefix = std::string(oscName) + ".";

  if (obj.has("bank")) {
    osc.bank = validateEnum(obj["bank"].asString(),
                            BANK_NAMES,
                            5,
                            "sine",
                            (prefix + "bank").c_str(),
                            warnings);
  }
  if (obj.has("scanPos"))
    osc.scanPos = clampWarn(obj["scanPos"].asFloat(),
                            osc::SCAN_POS_MIN,
                            osc::SCAN_POS_MAX,
                            (prefix + "scanPos").c_str(),
                            warnings);
  if (obj.has("mixLevel"))
    osc.mixLevel = clampWarn(obj["mixLevel"].asFloat(),
                             osc::MIX_LEVEL_MIN,
                             osc::MIX_LEVEL_MAX,
                             (prefix + "mixLevel").c_str(),
                             warnings);
  if (obj.has("fmDepth"))
    osc.fmDepth = clampWarn(obj["fmDepth"].asFloat(),
                            osc::FM_DEPTH_MIN,
                            osc::FM_DEPTH_MAX,
                            (prefix + "fmDepth").c_str(),
                            warnings);
  if (obj.has("fmRatio"))
    osc.fmRatio = clampWarn(obj["fmRatio"].asFloat(),
                            osc::FM_RATIO_MIN,
                            osc::FM_RATIO_MAX,
                            (prefix + "fmRatio").c_str(),
                            warnings);
  if (obj.has("fmSource")) {
    osc.fmSource = validateEnum(obj["fmSource"].asString(),
                                FM_SOURCE_NAMES,
                                5,
                                "none",
                                (prefix + "fmSource").c_str(),
                                warnings);
  }
  if (obj.has("octaveOffset")) {
    int8_t oct = obj["octaveOffset"].asInt8();
    if (oct < osc::OCTAVE_MIN || oct > osc::OCTAVE_MAX) {
      warnings.push_back(prefix + "octaveOffset: clamped " + std::to_string(oct));
      oct = oct < osc::OCTAVE_MIN ? osc::OCTAVE_MIN : osc::OCTAVE_MAX;
    }
    osc.octaveOffset = oct;
  }
  if (obj.has("detuneAmount"))
    osc.detuneAmount = clampWarn(obj["detuneAmount"].asFloat(),
                                 osc::DETUNE_MIN,
                                 osc::DETUNE_MAX,
                                 (prefix + "detuneAmount").c_str(),
                                 warnings);
  if (obj.has("enabled"))
    osc.enabled = obj["enabled"].asBool();

  return osc;
}

EnvelopePreset deserializeEnvelope(const json::Value& obj,
                                   const char* envName,
                                   std::vector<std::string>& warnings) {
  using namespace synth::param::ranges;
  EnvelopePreset env;

  if (!obj.isObject())
    return env;

  std::string prefix = std::string(envName) + ".";

  if (obj.has("attackMs"))
    env.attackMs = clampWarn(obj["attackMs"].asFloat(),
                             env::TIME_MIN,
                             env::TIME_MAX,
                             (prefix + "attackMs").c_str(),
                             warnings);
  if (obj.has("decayMs"))
    env.decayMs = clampWarn(obj["decayMs"].asFloat(),
                            env::TIME_MIN,
                            env::TIME_MAX,
                            (prefix + "decayMs").c_str(),
                            warnings);
  if (obj.has("sustainLevel"))
    env.sustainLevel = clampWarn(obj["sustainLevel"].asFloat(),
                                 env::SUSTAIN_MIN,
                                 env::SUSTAIN_MAX,
                                 (prefix + "sustainLevel").c_str(),
                                 warnings);
  if (obj.has("releaseMs"))
    env.releaseMs = clampWarn(obj["releaseMs"].asFloat(),
                              env::TIME_MIN,
                              env::TIME_MAX,
                              (prefix + "releaseMs").c_str(),
                              warnings);
  if (obj.has("attackCurve"))
    env.attackCurve = clampWarn(obj["attackCurve"].asFloat(),
                                env::CURVE_MIN,
                                env::CURVE_MAX,
                                (prefix + "attackCurve").c_str(),
                                warnings);
  if (obj.has("decayCurve"))
    env.decayCurve = clampWarn(obj["decayCurve"].asFloat(),
                               env::CURVE_MIN,
                               env::CURVE_MAX,
                               (prefix + "decayCurve").c_str(),
                               warnings);
  if (obj.has("releaseCurve"))
    env.releaseCurve = clampWarn(obj["releaseCurve"].asFloat(),
                                 env::CURVE_MIN,
                                 env::CURVE_MAX,
                                 (prefix + "releaseCurve").c_str(),
                                 warnings);

  return env;
}

LFOPreset deserializeLFO(const json::Value& obj,
                         const char* lfoName,
                         std::vector<std::string>& warnings) {
  using namespace synth::param::ranges;
  LFOPreset lfo;

  if (!obj.isObject())
    return lfo;

  std::string prefix = std::string(lfoName) + ".";

  if (obj.has("bank")) {
    lfo.bank = validateEnum(obj["bank"].asString(),
                            LFO_BANK_NAMES,
                            6,
                            "sine",
                            (prefix + "bank").c_str(),
                            warnings);
  }
  if (obj.has("rate"))
    lfo.rate = clampWarn(obj["rate"].asFloat(),
                         lfo::RATE_MIN,
                         lfo::RATE_MAX,
                         (prefix + "rate").c_str(),
                         warnings);
  if (obj.has("amplitude"))
    lfo.amplitude = clampWarn(obj["amplitude"].asFloat(),
                              lfo::AMPLITUDE_MIN,
                              lfo::AMPLITUDE_MAX,
                              (prefix + "amplitude").c_str(),
                              warnings);
  if (obj.has("retrigger"))
    lfo.retrigger = obj["retrigger"].asBool();

  return lfo;
}

} // anonymous namespace

// ============================================================
// deserializePreset
// ============================================================

DeserializeResult deserializePreset(const std::string& jsonStr) {
  using namespace synth::param::ranges;

  DeserializeResult result;

  // Parse JSON
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

  // Start from init preset — missing fields get init defaults
  Preset& p = result.preset;
  p = createInitPreset();

  // Version
  if (root.has("version")) {
    p.version = static_cast<uint32_t>(root["version"].asInt());
    if (p.version > CURRENT_PRESET_VERSION) {
      result.warnings.push_back("Preset version " + std::to_string(p.version) +
                                " is newer than supported version " +
                                std::to_string(CURRENT_PRESET_VERSION));
    }
  }

  // Metadata
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

  // Oscillators
  {
    const auto& oscs = root["oscillators"];
    if (oscs.isObject()) {
      // Init defaults: osc1 enabled, osc2-4 disabled
      OscPreset disabledOsc;
      disabledOsc.enabled = false;
      disabledOsc.mixLevel = 0.0f;

      p.osc1 = deserializeOsc(oscs["osc1"], "osc1", OscPreset{}, result.warnings);
      p.osc2 = deserializeOsc(oscs["osc2"], "osc2", disabledOsc, result.warnings);
      p.osc3 = deserializeOsc(oscs["osc3"], "osc3", disabledOsc, result.warnings);
      p.osc4 = deserializeOsc(oscs["osc4"], "osc4", disabledOsc, result.warnings);
    }
  }

  // Noise
  {
    const auto& n = root["noise"];
    if (n.isObject()) {
      if (n.has("mixLevel"))
        p.noise.mixLevel = clampWarn(n["mixLevel"].asFloat(),
                                     osc::noise::MIX_LEVEL_MIN,
                                     osc::noise::MIX_LEVEL_MAX,
                                     "noise.mixLevel",
                                     result.warnings);
      if (n.has("type"))
        p.noise.type = validateEnum(n["type"].asString(),
                                    NOISE_TYPE_NAMES,
                                    2,
                                    "white",
                                    "noise.type",
                                    result.warnings);
      if (n.has("enabled"))
        p.noise.enabled = n["enabled"].asBool();
    }
  }

  // Envelopes
  {
    const auto& envs = root["envelopes"];
    if (envs.isObject()) {
      p.ampEnv = deserializeEnvelope(envs["ampEnv"], "ampEnv", result.warnings);
      p.filterEnv = deserializeEnvelope(envs["filterEnv"], "filterEnv", result.warnings);
      p.modEnv = deserializeEnvelope(envs["modEnv"], "modEnv", result.warnings);
    }
  }

  // Filters
  {
    const auto& filters = root["filters"];
    if (filters.isObject()) {
      const auto& svf = filters["svf"];
      if (svf.isObject()) {
        if (svf.has("mode"))
          p.svf.mode = validateEnum(svf["mode"].asString(),
                                    SVF_MODE_NAMES,
                                    4,
                                    "lp",
                                    "svf.mode",
                                    result.warnings);
        if (svf.has("cutoff"))
          p.svf.cutoff = clampWarn(svf["cutoff"].asFloat(),
                                   filter::CUTOFF_MIN,
                                   filter::CUTOFF_MAX,
                                   "svf.cutoff",
                                   result.warnings);
        if (svf.has("resonance"))
          p.svf.resonance = clampWarn(svf["resonance"].asFloat(),
                                      filter::RESONANCE_MIN,
                                      filter::RESONANCE_MAX,
                                      "svf.resonance",
                                      result.warnings);
        if (svf.has("enabled"))
          p.svf.enabled = svf["enabled"].asBool();
      }

      const auto& ladder = filters["ladder"];
      if (ladder.isObject()) {
        if (ladder.has("cutoff"))
          p.ladder.cutoff = clampWarn(ladder["cutoff"].asFloat(),
                                      filter::CUTOFF_MIN,
                                      filter::CUTOFF_MAX,
                                      "ladder.cutoff",
                                      result.warnings);
        if (ladder.has("resonance"))
          p.ladder.resonance = clampWarn(ladder["resonance"].asFloat(),
                                         filter::RESONANCE_MIN,
                                         filter::RESONANCE_MAX,
                                         "ladder.resonance",
                                         result.warnings);
        if (ladder.has("drive"))
          p.ladder.drive = clampWarn(ladder["drive"].asFloat(),
                                     filter::DRIVE_MIN,
                                     filter::DRIVE_MAX,
                                     "ladder.drive",
                                     result.warnings);
        if (ladder.has("enabled"))
          p.ladder.enabled = ladder["enabled"].asBool();
      }
    }
  }

  // Saturator
  {
    const auto& sat = root["saturator"];
    if (sat.isObject()) {
      if (sat.has("drive"))
        p.saturator.drive = clampWarn(sat["drive"].asFloat(),
                                      saturator::DRIVE_MIN,
                                      saturator::DRIVE_MAX,
                                      "saturator.drive",
                                      result.warnings);
      if (sat.has("mix"))
        p.saturator.mix = clampWarn(sat["mix"].asFloat(),
                                    saturator::MIX_MIN,
                                    saturator::MIX_MAX,
                                    "saturator.mix",
                                    result.warnings);
      if (sat.has("enabled"))
        p.saturator.enabled = sat["enabled"].asBool();
    }
  }

  // LFOs
  {
    const auto& lfos = root["lfos"];
    if (lfos.isObject()) {
      p.lfo1 = deserializeLFO(lfos["lfo1"], "lfo1", result.warnings);
      p.lfo2 = deserializeLFO(lfos["lfo2"], "lfo2", result.warnings);
      p.lfo3 = deserializeLFO(lfos["lfo3"], "lfo3", result.warnings);
    }
  }

  // Mod matrix
  {
    const auto& mm = root["modMatrix"];
    p.modMatrix.clear();
    if (mm.isArray()) {
      for (size_t i = 0; i < mm.size(); i++) {
        const auto& route = mm.at(i);
        if (!route.isObject())
          continue;

        ModRoutePreset r;
        r.source = route["source"].asString();
        r.destination = route["destination"].asString();
        r.amount = route["amount"].asFloat();
        // Conservative global clamp — tightest per-dest clamping happens in PresetApply
        constexpr float MOD_AMOUNT_ABS_MAX = 24.0f; // pitch mod is the widest range
        r.amount = clampWarn(r.amount,
                             -MOD_AMOUNT_ABS_MAX,
                             MOD_AMOUNT_ABS_MAX,
                             ("modMatrix[" + std::to_string(i) + "].amount").c_str(),
                             result.warnings);

        // Validate source — use the mapping tables from ModMatrix.h
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

        // Validate destination
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

        p.modMatrix.push_back(std::move(r));
      }
    }
  }

  // Signal chain
  {
    const auto& chain = root["signalChain"];
    if (chain.isArray()) {
      p.signalChain.clear();
      for (size_t i = 0; i < chain.size(); i++) {
        const std::string& name = chain.at(i).asString();
        // Validate
        bool valid = false;
        for (const char* const proc : SIGNAL_PROC_NAMES) {
          if (name == proc) {
            valid = true;
            break;
          }
        }
        if (valid) {
          p.signalChain.push_back(name);
        } else {
          result.warnings.push_back("signalChain[" + std::to_string(i) + "]: unknown processor '" +
                                    name + "', skipping");
        }
      }
    }
  }

  // Mono
  {
    const auto& m = root["mono"];
    if (m.isObject()) {
      if (m.has("enabled"))
        p.mono.enabled = m["enabled"].asBool();
      if (m.has("legato"))
        p.mono.legato = m["legato"].asBool();
    }
  }

  // Portamento
  {
    const auto& porta = root["portamento"];
    if (porta.isObject()) {
      if (porta.has("time"))
        p.porta.time = clampWarn(porta["time"].asFloat(),
                                 porta::TIME_MIN,
                                 porta::TIME_MAX,
                                 "portamento.time",
                                 result.warnings);
      if (porta.has("legato"))
        p.porta.legato = porta["legato"].asBool();
      if (porta.has("enabled"))
        p.porta.enabled = porta["enabled"].asBool();
    }
  }

  // Unison
  {
    const auto& u = root["unison"];
    if (u.isObject()) {
      if (u.has("voices")) {
        int8_t v = u["voices"].asInt8();
        if (v < unison::VOICES_MIN || v > unison::VOICES_MAX) {
          result.warnings.push_back("unison.voices: clamped " + std::to_string(v));
          v = v < unison::VOICES_MIN ? unison::VOICES_MIN : unison::VOICES_MAX;
        }
        p.unison.voices = v;
      }
      if (u.has("detune"))
        p.unison.detune = clampWarn(u["detune"].asFloat(),
                                    unison::DETUNE_MIN,
                                    unison::DETUNE_MAX,
                                    "unison.detune",
                                    result.warnings);
      if (u.has("spread"))
        p.unison.spread = clampWarn(u["spread"].asFloat(),
                                    unison::SPREAD_MIN,
                                    unison::SPREAD_MAX,
                                    "unison.spread",
                                    result.warnings);
      if (u.has("enabled"))
        p.unison.enabled = u["enabled"].asBool();
    }
  }

  // Global
  {
    const auto& g = root["global"];
    if (g.isObject()) {
      if (g.has("pitchBendRange"))
        p.global.pitchBendRange = clampWarn(g["pitchBendRange"].asFloat(),
                                            pitch::BEND_RANGE_MIN,
                                            pitch::BEND_RANGE_MAX,
                                            "global.pitchBendRange",
                                            result.warnings);
    }
  }

  return result;
}
} // namespace synth::preset

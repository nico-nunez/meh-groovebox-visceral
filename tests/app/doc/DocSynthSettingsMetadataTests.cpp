#include "TestRunner.h"

#include "app/doc/DocMetadata.h"
#include "app/doc/DocSynthSettingsMetadata.h"
#include "synth/params/ParamDefs.h"
#include "synth/params/ParamUtils.h"

#include <cstring>
#include <set>
#include <string>

namespace {

bool strEq(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

bool hasField(const char* authoredPath) {
  return app::doc::findAuthoredSynthParamField(authoredPath) != nullptr;
}

const app::doc::AuthoredSynthParamField* requireField(const char* authoredPath) {
  const auto* field = app::doc::findAuthoredSynthParamField(authoredPath);
  CHECK((std::string("field ") + authoredPath).c_str(), field != nullptr);
  return field;
}

} // namespace

static void test_authored_synth_mapping_has_first_slice_representatives() {
  TEST("authored_synth_mapping_has_first_slice_representatives");

  const auto fields = app::doc::authoredSynthParamFields();
  CHECK("fields nonempty", !fields.empty());

  CHECK("osc1 bank", hasField("osc1.bank"));
  CHECK("osc1 mix alias", hasField("osc1.mix"));
  CHECK("osc4 fixed freq", hasField("osc4.fixedFreq"));
  CHECK("noise type", hasField("noise.type"));
  CHECK("amp env attack", hasField("ampEnv.attack"));
  CHECK("svf cutoff", hasField("svf.cutoff"));
  CHECK("ladder drive", hasField("ladder.drive"));
  CHECK("mono enabled", hasField("mono.enabled"));
  CHECK("porta time", hasField("porta.time"));
  CHECK("unison voices", hasField("unison.voices"));
  CHECK("master gain alias", hasField("master.gain"));
  CHECK("fx delay mix", hasField("fx.delay.mix"));
  CHECK("fx reverb enabled", hasField("fx.reverb.enabled"));
}

static void test_authored_synth_mapping_is_subset_not_all_params() {
  TEST("authored_synth_mapping_is_subset_not_all_params");

  const auto fields = app::doc::authoredSynthParamFields();

  CHECK("subset smaller than PARAM_COUNT", fields.size < synth::param::PARAM_COUNT);
  CHECK("lfo deferred", !hasField("lfo1.rate"));
  CHECK("mod matrix deferred", !hasField("modMatrix"));
  CHECK("signal chain deferred", !hasField("signalChain"));
  CHECK("deep reverb decay deferred", !hasField("fx.reverb.decay"));
  CHECK("envelope curve deferred", !hasField("ampEnv.attackCurve"));
}

static void test_authored_synth_aliases_map_to_canonical_params() {
  TEST("authored_synth_aliases_map_to_canonical_params");

  const auto* oscMix = requireField("osc1.mix");
  const auto* oscScan = requireField("osc1.scan");
  const auto* masterGain = requireField("master.gain");
  const auto* ampAttack = requireField("ampEnv.attack");

  CHECK("osc mix canonical", oscMix && strEq(oscMix->canonicalParam, "osc1.mixLevel"));
  CHECK("osc mix id", oscMix && oscMix->paramID == synth::param::OSC1_MIX_LEVEL);
  CHECK("osc scan canonical", oscScan && strEq(oscScan->canonicalParam, "osc1.scanPos"));
  CHECK("osc scan id", oscScan && oscScan->paramID == synth::param::OSC1_SCAN_POS);
  CHECK("master gain canonical", masterGain && strEq(masterGain->canonicalParam, "masterGain"));
  CHECK("master gain id", masterGain && masterGain->paramID == synth::param::MASTER_GAIN);
  CHECK("amp attack canonical", ampAttack && strEq(ampAttack->canonicalParam, "ampEnv.attackMs"));
  CHECK("amp attack id", ampAttack && ampAttack->paramID == synth::param::AMP_ENV_ATTACK);
}

static void test_authored_synth_all_canonical_params_resolve() {
  TEST("authored_synth_all_canonical_params_resolve");

  for (const auto& field : app::doc::authoredSynthParamFields()) {
    const auto resolved = synth::param::utils::getParamIDByName(field.canonicalParam);
    CHECK((std::string("resolve ") + field.canonicalParam).c_str(),
          resolved != synth::param::PARAM_UNKNOWN);
    CHECK((std::string("id matches ") + field.canonicalParam).c_str(), resolved == field.paramID);
  }
}

static void test_authored_synth_value_kinds_match_param_types() {
  TEST("authored_synth_value_kinds_match_param_types");

  for (const auto& field : app::doc::authoredSynthParamFields()) {
    const auto& def = synth::param::PARAM_DEFS[field.paramID];
    const auto expected = app::doc::authoredSynthValueKindForParamType(def.type);
    CHECK((std::string("value kind ") + field.authoredPath).c_str(), field.valueKind == expected);
  }
}

static void test_authored_synth_uses_string_first_enums() {
  TEST("authored_synth_uses_string_first_enums");

  const auto* bank = requireField("osc1.bank");
  const auto* noise = requireField("noise.type");
  const auto* mode = requireField("svf.mode");

  CHECK("bank string", bank && bank->valueKind == app::doc::DocLuaValueKind::String);
  CHECK("noise string", noise && noise->valueKind == app::doc::DocLuaValueKind::String);
  CHECK("filter mode string", mode && mode->valueKind == app::doc::DocLuaValueKind::String);
}

static void test_authored_synth_uses_bool_and_integer_kinds() {
  TEST("authored_synth_uses_bool_and_integer_kinds");

  const auto* enabled = requireField("osc1.enabled");
  const auto* fixed = requireField("osc1.fixed");
  const auto* voices = requireField("unison.voices");
  const auto* octave = requireField("osc1.octave");

  CHECK("enabled boolean", enabled && enabled->valueKind == app::doc::DocLuaValueKind::Boolean);
  CHECK("fixed boolean", fixed && fixed->valueKind == app::doc::DocLuaValueKind::Boolean);
  CHECK("voices integer", voices && voices->valueKind == app::doc::DocLuaValueKind::Integer);
  CHECK("octave integer", octave && octave->valueKind == app::doc::DocLuaValueKind::Integer);
}

static void test_authored_synth_mapping_has_unique_authored_paths_and_params() {
  TEST("authored_synth_mapping_has_unique_authored_paths_and_params");

  std::set<std::string> authoredPaths{};
  std::set<std::string> canonicalParams{};
  bool uniqueAuthored = true;
  bool uniqueCanonical = true;

  for (const auto& field : app::doc::authoredSynthParamFields()) {
    if (!authoredPaths.insert(field.authoredPath).second)
      uniqueAuthored = false;
    if (!canonicalParams.insert(field.canonicalParam).second)
      uniqueCanonical = false;
  }

  CHECK("unique authored paths", uniqueAuthored);
  CHECK("unique canonical params", uniqueCanonical);
}

static void test_authored_synth_lookup_by_canonical_param() {
  TEST("authored_synth_lookup_by_canonical_param");

  const auto* mix = app::doc::findAuthoredSynthParamFieldByCanonicalParam("osc1.mixLevel");
  const auto* master = app::doc::findAuthoredSynthParamFieldByCanonicalParam("masterGain");
  const auto* lfo = app::doc::findAuthoredSynthParamFieldByCanonicalParam("lfo1.rate");

  CHECK("mix found", mix && strEq(mix->authoredPath, "osc1.mix"));
  CHECK("master found", master && strEq(master->authoredPath, "master.gain"));
  CHECK("deferred lfo absent", lfo == nullptr);
}

void runDocSynthSettingsMetadataTests() {
  SUITE("DocSynthSettingsMetadata");
  test_authored_synth_mapping_has_first_slice_representatives();
  test_authored_synth_mapping_is_subset_not_all_params();
  test_authored_synth_aliases_map_to_canonical_params();
  test_authored_synth_all_canonical_params_resolve();
  test_authored_synth_value_kinds_match_param_types();
  test_authored_synth_uses_string_first_enums();
  test_authored_synth_uses_bool_and_integer_kinds();
  test_authored_synth_mapping_has_unique_authored_paths_and_params();
  test_authored_synth_lookup_by_canonical_param();
}

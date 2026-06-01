#include "TestRunner.h"

#include "app/doc/metadata/DocMetadata.h"
#include "app/doc/metadata/DocSynthSettingsMetadata.h"
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
  CHECK("osc1 mixLevel", hasField("osc1.mixLevel"));
  CHECK("osc4 fixed freq", hasField("osc4.fixedFreq"));
  CHECK("lfo1 bank", hasField("lfo1.bank"));
  CHECK("lfo2 bank", hasField("lfo2.bank"));
  CHECK("lfo3 bank", hasField("lfo3.bank"));
  CHECK("noise type", hasField("noise.type"));
  CHECK("amp env attack", hasField("ampEnv.attack"));
  CHECK("mod env attack", hasField("modEnv.attack"));
  CHECK("filter env attack", hasField("filterEnv.attack"));
  CHECK("svf cutoff", hasField("svf.cutoff"));
  CHECK("ladder drive", hasField("ladder.drive"));
  CHECK("saturator drive", hasField("saturator.drive"));
  CHECK("mono enabled", hasField("mono.enabled"));
  CHECK("porta time", hasField("porta.time"));
  CHECK("unison voices", hasField("unison.voices"));
  CHECK("master gain", hasField("master.gain"));
  CHECK("fx distortion drive", hasField("fx.distortion.drive"));
  CHECK("fx distortion enabled", hasField("fx.distortion.enabled"));
  CHECK("fx chorus depth", hasField("fx.chorus.depth"));
  CHECK("fx chorus enabled", hasField("fx.chorus.enabled"));
  CHECK("fx phaser stages", hasField("fx.phaser.stages"));
  CHECK("fx phaser enabled", hasField("fx.phaser.enabled"));
  CHECK("fx delay time", hasField("fx.delay.time"));
  CHECK("fx delay enabled", hasField("fx.delay.enabled"));
  CHECK("fx reverb preDelay", hasField("fx.reverb.preDelay"));
  CHECK("fx reverb enabled", hasField("fx.reverb.enabled"));
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
  const auto* octaveOffset = requireField("osc1.octaveOffset");

  CHECK("enabled boolean", enabled && enabled->valueKind == app::doc::DocLuaValueKind::Boolean);
  CHECK("fixed boolean", fixed && fixed->valueKind == app::doc::DocLuaValueKind::Boolean);
  CHECK("voices integer", voices && voices->valueKind == app::doc::DocLuaValueKind::Integer);
  CHECK("octaveOffset integer",
        octaveOffset && octaveOffset->valueKind == app::doc::DocLuaValueKind::Integer);
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

  const auto* mixLevel = app::doc::findAuthoredSynthParamFieldByCanonicalParam("osc1.mixLevel");
  const auto* master = app::doc::findAuthoredSynthParamFieldByCanonicalParam("master.gain");
  const auto* lfo = app::doc::findAuthoredSynthParamFieldByCanonicalParam("lfo1.rate");

  CHECK("mixLevel found", mixLevel && strEq(mixLevel->authoredPath, "osc1.mixLevel"));
  CHECK("master found", master && strEq(master->authoredPath, "master.gain"));
  CHECK("lfo found", lfo && strEq(lfo->authoredPath, "lfo1.rate"));
}

void runDocSynthSettingsMetadataTests() {
  SUITE("DocSynthSettingsMetadata");
  test_authored_synth_mapping_has_first_slice_representatives();
  test_authored_synth_all_canonical_params_resolve();
  test_authored_synth_value_kinds_match_param_types();
  test_authored_synth_uses_string_first_enums();
  test_authored_synth_uses_bool_and_integer_kinds();
  test_authored_synth_mapping_has_unique_authored_paths_and_params();
  test_authored_synth_lookup_by_canonical_param();
}

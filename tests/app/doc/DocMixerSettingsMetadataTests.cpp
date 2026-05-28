#include "TestRunner.h"

#include "app/AppParams.h"
#include "app/doc/metadata/DocMetadata.h"
#include "app/doc/metadata/DocMixerSettingsMetadata.h"

#include <string>

namespace {

bool hasField(const char* authoredField) {
  return app::doc::findAuthoredTrackMixerParamField(authoredField) != nullptr;
}

} // namespace

static void test_mixer_param_fields_count_is_3() {
  TEST("mixer_param_fields_count_is_3");
  auto fields = app::doc::authoredTrackMixerParamFields();
  CHECK("count", fields.size == 3);
}

static void test_mixer_param_fields_not_empty() {
  TEST("mixer_param_fields_not_empty");
  auto fields = app::doc::authoredTrackMixerParamFields();
  CHECK("non-null data", fields.data != nullptr);
}

static void test_mixer_param_fields_gain_entry() {
  TEST("mixer_param_fields_gain_entry");
  const auto* f = app::doc::findAuthoredTrackMixerParamField("gain");
  CHECK("found", f != nullptr);
  CHECK("paramID", f && f->paramID == app::params::AppParamID::TrackGain);
  CHECK("valueKind", f && f->valueKind == app::doc::DocLuaValueKind::Number);
}

static void test_mixer_param_fields_pan_entry() {
  TEST("mixer_param_fields_pan_entry");
  const auto* f = app::doc::findAuthoredTrackMixerParamField("pan");
  CHECK("found", f != nullptr);
  CHECK("paramID", f && f->paramID == app::params::AppParamID::TrackPan);
  CHECK("valueKind", f && f->valueKind == app::doc::DocLuaValueKind::Number);
}

static void test_mixer_param_fields_mute_entry() {
  TEST("mixer_param_fields_mute_entry");
  const auto* f = app::doc::findAuthoredTrackMixerParamField("mute");
  CHECK("found", f != nullptr);
  CHECK("paramID", f && f->paramID == app::params::AppParamID::TrackMute);
  CHECK("valueKind", f && f->valueKind == app::doc::DocLuaValueKind::Boolean);
}

static void test_find_authored_track_mixer_param_field_known() {
  TEST("find_authored_track_mixer_param_field_known");
  const auto* f = app::doc::findAuthoredTrackMixerParamField("gain");
  CHECK("found", f != nullptr);
}

static void test_find_authored_track_mixer_param_field_unknown_returns_null() {
  TEST("find_authored_track_mixer_param_field_unknown_returns_null");
  CHECK("masterGain null", app::doc::findAuthoredTrackMixerParamField("masterGain") == nullptr);
  CHECK("limiterThreshold null",
        app::doc::findAuthoredTrackMixerParamField("limiterThreshold") == nullptr);
  CHECK("empty string null", app::doc::findAuthoredTrackMixerParamField("") == nullptr);
  CHECK("volume null", app::doc::findAuthoredTrackMixerParamField("volume") == nullptr);
}

static void test_find_authored_track_mixer_param_field_global_fields_absent() {
  TEST("find_authored_track_mixer_param_field_global_fields_absent");
  CHECK("masterGain absent", !hasField("masterGain"));
  CHECK("limiterThreshold absent", !hasField("limiterThreshold"));
}

static void test_mixer_param_fields_valuekind_matches_param_type() {
  TEST("mixer_param_fields_valuekind_matches_param_type");
  namespace ap = app::params;
  for (const auto& f : app::doc::authoredTrackMixerParamFields()) {
    const auto& def = ap::getAppParamDef(f.paramID);
    const auto expected = app::doc::authoredMixerValueKindForParamType(def.type);
    CHECK(f.authoredField, f.valueKind == expected);
  }
}

static void test_mixer_param_fields_all_are_track_scoped() {
  TEST("mixer_param_fields_all_are_track_scoped");
  namespace ap = app::params;
  for (const auto& f : app::doc::authoredTrackMixerParamFields())
    CHECK(f.authoredField, ap::isTrackScoped(f.paramID));
}

static void test_mixer_param_fields_no_duplicate_authored_field() {
  TEST("mixer_param_fields_no_duplicate_authored_field");
  auto fields = app::doc::authoredTrackMixerParamFields();
  bool unique = true;
  for (std::size_t i = 0; i < fields.size; ++i) {
    for (std::size_t j = i + 1; j < fields.size; ++j) {
      if (std::string(fields.data[i].authoredField) == std::string(fields.data[j].authoredField))
        unique = false;
    }
  }
  CHECK("unique authoredField", unique);
}

static void test_mixer_param_fields_no_duplicate_param_id() {
  TEST("mixer_param_fields_no_duplicate_param_id");
  auto fields = app::doc::authoredTrackMixerParamFields();
  bool unique = true;
  for (std::size_t i = 0; i < fields.size; ++i) {
    for (std::size_t j = i + 1; j < fields.size; ++j) {
      if (fields.data[i].paramID == fields.data[j].paramID)
        unique = false;
    }
  }
  CHECK("unique paramID", unique);
}

static void test_authored_mixer_valuekind_for_float() {
  TEST("authored_mixer_valuekind_for_float");
  using namespace app::doc;
  using namespace app::params;
  CHECK("float",
        authoredMixerValueKindForParamType(AppParamType::Float) == DocLuaValueKind::Number);
}

static void test_authored_mixer_valuekind_for_bool() {
  TEST("authored_mixer_valuekind_for_bool");
  using namespace app::doc;
  using namespace app::params;
  CHECK("bool", authoredMixerValueKindForParamType(AppParamType::Bool) == DocLuaValueKind::Boolean);
}

void runDocMixerSettingsMetadataTests() {
  SUITE("DocMixerSettingsMetadata");
  test_mixer_param_fields_count_is_3();
  test_mixer_param_fields_not_empty();
  test_mixer_param_fields_gain_entry();
  test_mixer_param_fields_pan_entry();
  test_mixer_param_fields_mute_entry();
  test_find_authored_track_mixer_param_field_known();
  test_find_authored_track_mixer_param_field_unknown_returns_null();
  test_find_authored_track_mixer_param_field_global_fields_absent();
  test_mixer_param_fields_valuekind_matches_param_type();
  test_mixer_param_fields_all_are_track_scoped();
  test_mixer_param_fields_no_duplicate_authored_field();
  test_mixer_param_fields_no_duplicate_param_id();
  test_authored_mixer_valuekind_for_float();
  test_authored_mixer_valuekind_for_bool();
}

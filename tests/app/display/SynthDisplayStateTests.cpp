#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/display/DisplayState.h"
#include "app/display/SynthDisplayState.h"

#include "synth/Engine.h"
#include "synth/params/ParamDefs.h"

namespace {

void setParam(synth::Engine& engine, synth::param::ParamID id, float value) {
  engine.params[static_cast<int>(id)] = value;
}

bool containsParam(synth::param::ParamID id) {
  const auto* ids = app::display::curatedSynthParamIDs();
  for (uint8_t i = 0; i < app::display::curatedSynthParamCount(); ++i) {
    if (ids[i] == id)
      return true;
  }
  return false;
}

} // namespace

static void test_curated_param_list_has_exact_capacity() {
  TEST("curated_param_list_has_exact_capacity");

  CHECK("count 64", app::display::curatedSynthParamCount() == 64);
}

static void test_curated_param_list_contains_locked_groups() {
  TEST("curated_param_list_contains_locked_groups");

  CHECK("osc1 enabled", containsParam(synth::param::OSC1_ENABLED));
  CHECK("osc4 fixed freq", containsParam(synth::param::OSC4_FIXED_FREQ));
  CHECK("noise enabled", containsParam(synth::param::NOISE_ENABLED));
  CHECK("svf cutoff", containsParam(synth::param::SVF_CUTOFF));
  CHECK("ladder drive", containsParam(synth::param::LADDER_DRIVE));
  CHECK("amp attack", containsParam(synth::param::AMP_ENV_ATTACK));
  CHECK("unison voices", containsParam(synth::param::UNISON_VOICES));
  CHECK("master gain", containsParam(synth::param::MASTER_GAIN));
  CHECK("reverb enabled", containsParam(synth::param::FX_REVERB_ENABLED));
}

static void test_curated_param_list_excludes_deferred_groups() {
  TEST("curated_param_list_excludes_deferred_groups");

  CHECK("osc phase excluded", !containsParam(synth::param::OSC1_PHASE_MODE));
  CHECK("lfo excluded", !containsParam(synth::param::LFO1_RATE));
  CHECK("filter env excluded", !containsParam(synth::param::FILTER_ENV_ATTACK));
  CHECK("mod env excluded", !containsParam(synth::param::MOD_ENV_ATTACK));
  CHECK("porta excluded", !containsParam(synth::param::PORTA_TIME));
  CHECK("fx detailed excluded", !containsParam(synth::param::FX_DELAY_TIME));
}

static void test_curated_param_list_has_no_duplicates() {
  TEST("curated_param_list_has_no_duplicates");

  bool seen[synth::param::PARAM_COUNT]{};
  const auto* ids = app::display::curatedSynthParamIDs();

  for (uint8_t i = 0; i < app::display::curatedSynthParamCount(); ++i) {
    const int index = static_cast<int>(ids[i]);
    CHECK("valid id", index >= 0 && index < synth::param::PARAM_COUNT);
    CHECK("not duplicate", !seen[index]);
    seen[index] = true;
  }
}

static void test_fill_synth_runtime_telemetry_copies_note_count_and_params() {
  TEST("fill_synth_runtime_telemetry_copies_note_count_and_params");

  synth::Engine engine{};
  engine.noteCount = 3;
  setParam(engine, synth::param::OSC1_MIX_LEVEL, 0.75f);
  setParam(engine, synth::param::SVF_CUTOFF, 1234.0f);
  setParam(engine, synth::param::MASTER_GAIN, 1.5f);

  app::display::SynthRuntimeTelemetry telemetry{};
  app::display::fillSynthRuntimeTelemetry(telemetry, engine);

  CHECK("note count", telemetry.noteCount == 3);
  CHECK("param count", telemetry.paramCount == app::display::DISPLAY_SYNTH_PARAM_CAPACITY);

  const auto snapshot = app::display::makeSynthSummarySnapshot(telemetry);
  const auto* oscMix = app::display::findSynthParam(snapshot, synth::param::OSC1_MIX_LEVEL);
  const auto* cutoff = app::display::findSynthParam(snapshot, synth::param::SVF_CUTOFF);
  const auto* master = app::display::findSynthParam(snapshot, synth::param::MASTER_GAIN);

  CHECK("osc mix found", oscMix != nullptr);
  CHECK("osc mix value", oscMix && oscMix->value == 0.75f);
  CHECK("cutoff found", cutoff != nullptr);
  CHECK("cutoff value", cutoff && cutoff->value == 1234.0f);
  CHECK("master found", master != nullptr);
  CHECK("master value", master && master->value == 1.5f);
}

static void test_fill_synth_runtime_telemetry_resets_output() {
  TEST("fill_synth_runtime_telemetry_resets_output");

  synth::Engine engine{};
  app::display::SynthRuntimeTelemetry telemetry{};
  telemetry.noteCount = 99;
  telemetry.paramCount = 1;
  telemetry.params[0].id = synth::param::PARAM_UNKNOWN;

  app::display::fillSynthRuntimeTelemetry(telemetry, engine);

  CHECK("note count reset", telemetry.noteCount == 0);
  CHECK("param count filled", telemetry.paramCount == app::display::DISPLAY_SYNTH_PARAM_CAPACITY);
  CHECK("first id valid", telemetry.params[0].id != synth::param::PARAM_UNKNOWN);
}

static void test_make_synth_summary_snapshot_pairs_param_defs() {
  TEST("make_synth_summary_snapshot_pairs_param_defs");

  app::display::SynthRuntimeTelemetry telemetry{};
  telemetry.noteCount = 4;
  telemetry.paramCount = 2;
  telemetry.params[0].id = synth::param::OSC1_ENABLED;
  telemetry.params[0].value = 1.0f;
  telemetry.params[1].id = synth::param::MASTER_GAIN;
  telemetry.params[1].value = 0.5f;

  const auto snapshot = app::display::makeSynthSummarySnapshot(telemetry);

  CHECK("note count", snapshot.noteCount == 4);
  CHECK("param count", snapshot.paramCount == 2);
  CHECK("def 0", snapshot.params[0].def == &synth::param::PARAM_DEFS[synth::param::OSC1_ENABLED]);
  CHECK("def 1", snapshot.params[1].def == &synth::param::PARAM_DEFS[synth::param::MASTER_GAIN]);
  CHECK("name 0", snapshot.params[0].def && snapshot.params[0].def->name != nullptr);
}

static void test_find_synth_param_returns_null_for_missing_param() {
  TEST("find_synth_param_returns_null_for_missing_param");

  app::display::SynthSummarySnapshot snapshot{};
  snapshot.paramCount = 1;
  snapshot.params[0].id = synth::param::MASTER_GAIN;

  CHECK("missing null",
        app::display::findSynthParam(snapshot, synth::param::OSC1_ENABLED) == nullptr);
}

static void test_display_runtime_telemetry_includes_synth_params() {
  TEST("display_runtime_telemetry_includes_synth_params");

  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);
  app->tracks[0].engine.noteCount = 2;
  setParam(app->tracks[0].engine, synth::param::OSC1_MIX_LEVEL, 0.33f);

  const auto runtime = app::display::makeDisplayRuntimeTelemetry(*app);
  const auto synth = app::display::makeSynthSummarySnapshot(runtime.tracks[0].synth);
  const auto* oscMix = app::display::findSynthParam(synth, synth::param::OSC1_MIX_LEVEL);

  CHECK("note count", runtime.tracks[0].synth.noteCount == 2);
  CHECK("param count",
        runtime.tracks[0].synth.paramCount == app::display::DISPLAY_SYNTH_PARAM_CAPACITY);
  CHECK("osc mix found", oscMix != nullptr);
  CHECK("osc mix value", oscMix && oscMix->value == 0.33f);
  test::destroyAppContext(app);
}

void runSynthDisplayStateTests() {
  SUITE("SynthDisplayState");
  test_curated_param_list_has_exact_capacity();
  test_curated_param_list_contains_locked_groups();
  test_curated_param_list_excludes_deferred_groups();
  test_curated_param_list_has_no_duplicates();
  test_fill_synth_runtime_telemetry_copies_note_count_and_params();
  test_fill_synth_runtime_telemetry_resets_output();
  test_make_synth_summary_snapshot_pairs_param_defs();
  test_find_synth_param_returns_null_for_missing_param();
  test_display_runtime_telemetry_includes_synth_params();
}

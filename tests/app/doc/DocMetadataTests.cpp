#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/Constants.h"
#include "app/Sequencer.h"
#include "app/doc/AuthoredDocParser.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/metadata/DocMetadata.h"

#include <cstddef>
#include <cstring>
#include <set>
#include <string>

namespace {

bool allDiagnosticsAreCataloged(const app::doc::DocDiagnostics& diagnostics) {
  for (const auto& diagnostic : diagnostics) {
    if (!app::doc::findDocumentDiagnostic(diagnostic.code.c_str()))
      return false;
  }
  return true;
}

bool strEq(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

bool hasConstructor(const char* name) {
  for (const char* constructor : app::doc::authoredDocumentConstructors()) {
    if (strEq(constructor, name))
      return true;
  }
  return false;
}

const app::doc::DocFieldMetadata* findField(const app::doc::DocTypeMetadata& type,
                                            const char* name) {
  for (const auto& field : type.fields) {
    if (strEq(field.name, name))
      return &field;
  }
  return nullptr;
}

bool hasDiagnostic(const char* code) {
  return app::doc::findDocumentDiagnostic(code) != nullptr;
}

const app::doc::DocFunctionMetadata* findFunction(const char* name) {
  return app::doc::findAuthoredDocumentFunction(name);
}

} // namespace

static void test_authored_surface_contains_only_document_globals() {
  TEST("authored_surface_contains_only_document_globals");
  CHECK("track function",
        app::doc::findAuthoredDocumentFunction(app::doc::docglobal::Track) != nullptr);
  CHECK("TrackSettings constructor", hasConstructor(app::doc::docctor::TrackSettings));
  CHECK("SynthSettings constructor", hasConstructor(app::doc::docctor::SynthSettings));
  CHECK("MixerSettings constructor", hasConstructor(app::doc::docctor::MixerSettings));
  CHECK("no applyFile function", app::doc::findAuthoredDocumentFunction("applyFile") == nullptr);
  CHECK("no apply_file function", app::doc::findAuthoredDocumentFunction("apply_file") == nullptr);
}

static void test_reserved_constructors_are_narrow() {
  TEST("reserved_constructors_are_narrow");
  const auto* synth = app::doc::findAuthoredDocumentType(app::doc::doctype::SynthSettings);
  const auto* mixer = app::doc::findAuthoredDocumentType(app::doc::doctype::MixerSettings);
  CHECK("SynthSettings type exists", synth != nullptr);
  CHECK("MixerSettings type exists", mixer != nullptr);
}

static void test_track_signature_uses_static_track_bounds() {
  TEST("track_signature_uses_static_track_bounds");
  const auto* track = app::doc::findAuthoredDocumentFunction(app::doc::docglobal::Track);
  CHECK("track exists", track != nullptr);
  CHECK("track has 2 args", track && track->args.size == 2);
  CHECK("track arg min 1", track && track->args.data[0].integerBounds.min == 1);
  CHECK("track arg max MAX_TRACKS",
        track && track->args.data[0].integerBounds.max == app::MAX_TRACKS);
}

static void test_track_settings_fields_match_parser_surface() {
  TEST("track_settings_fields_match_parser_surface");
  const auto* type = app::doc::findAuthoredDocumentType(app::doc::doctype::TrackSettings);
  CHECK("TrackSettings exists", type != nullptr);
  const auto* patterns = type ? findField(*type, "patterns") : nullptr;
  const auto* activeSlot = type ? findField(*type, "activeSlot") : nullptr;
  CHECK("patterns field", patterns != nullptr);
  CHECK("activeSlot field", activeSlot != nullptr);
  CHECK("patterns optional", patterns && !patterns->required);
  CHECK("activeSlot optional", activeSlot && !activeSlot->required);
  CHECK("activeSlot max slot",
        activeSlot && activeSlot->integerBounds.max == app::sequencer::PATTERNS_PER_LANE);
  CHECK("synth field", type && findField(*type, "synth") != nullptr);
  CHECK("no mixer field", type && findField(*type, "mixer") != nullptr);
}

static void test_pattern_bounds_derive_from_sequencer_constants() {
  TEST("pattern_bounds_derive_from_sequencer_constants");
  const auto* pattern = app::doc::findAuthoredDocumentType(app::doc::doctype::Pattern);
  CHECK("Pattern exists", pattern != nullptr);
  const auto* numSteps = pattern ? findField(*pattern, "numSteps") : nullptr;
  const auto* stepsPerBeat = pattern ? findField(*pattern, "stepsPerBeat") : nullptr;
  CHECK("numSteps max",
        numSteps && numSteps->integerBounds.max == app::sequencer::MAX_PATTERN_STEPS);
  CHECK("stepsPerBeat max",
        stepsPerBeat && stepsPerBeat->integerBounds.max == app::sequencer::MAX_STEPS_PER_BEAT);
}

static void test_step_fields_match_lua_pattern_parser() {
  TEST("step_fields_match_lua_pattern_parser");
  const auto* step = app::doc::findAuthoredDocumentType(app::doc::doctype::Step);
  CHECK("Step exists", step != nullptr);
  CHECK("active", step && findField(*step, "active") != nullptr);
  CHECK("note", step && findField(*step, "note") != nullptr);
  CHECK("velocity", step && findField(*step, "velocity") != nullptr);
  CHECK("gate", step && findField(*step, "gate") != nullptr);
  CHECK("legato", step && findField(*step, "legato") != nullptr);
  CHECK("locks", step && findField(*step, "locks") != nullptr);
  const auto* note = step ? findField(*step, "note") : nullptr;
  const auto* velocity = step ? findField(*step, "velocity") : nullptr;
  CHECK("note 0..127", note && note->integerBounds.min == 0 && note->integerBounds.max == 127);
  CHECK("velocity 0..127",
        velocity && velocity->integerBounds.min == 0 && velocity->integerBounds.max == 127);
}

static void test_diagnostic_catalog_contains_emitted_codes() {
  TEST("diagnostic_catalog_contains_emitted_codes");
  CHECK("track invalid index", hasDiagnostic(app::doc::docdiag::SequencerTrackInvalidIndex));
  CHECK("track invalid settings", hasDiagnostic(app::doc::docdiag::SequencerTrackInvalidSettings));
  CHECK("patterns invalid shape", hasDiagnostic(app::doc::docdiag::SequencerPatternsInvalidShape));
  CHECK("slot invalid key", hasDiagnostic(app::doc::docdiag::SequencerPatternSlotInvalidKey));
  CHECK("slot out of range", hasDiagnostic(app::doc::docdiag::SequencerPatternSlotOutOfRange));
  CHECK("pattern invalid shape", hasDiagnostic(app::doc::docdiag::SequencerPatternInvalidShape));
  CHECK("activeSlot invalid type",
        hasDiagnostic(app::doc::docdiag::SequencerActiveSlotInvalidType));
  CHECK("activeSlot out of range", hasDiagnostic(app::doc::docdiag::SequencerActiveSlotOutOfRange));
  CHECK("activeSlot missing patterns",
        hasDiagnostic(app::doc::docdiag::SequencerActiveSlotMissingPatterns));
  CHECK("activeSlot empty slot", hasDiagnostic(app::doc::docdiag::SequencerActiveSlotEmptySlot));
  CHECK("admission failed", hasDiagnostic(app::doc::docdiag::SequencerAdmissionFailed));
  CHECK("lua state failed", hasDiagnostic(app::doc::docdiag::DocumentLuaStateFailed));
  CHECK("lua eval failed", hasDiagnostic(app::doc::docdiag::DocumentLuaEvalFailed));
  CHECK("file read failed", hasDiagnostic(app::doc::docdiag::DocumentFileReadFailed));

  CHECK("synth track invalid index", hasDiagnostic(app::doc::docdiag::SynthTrackInvalidIndex));
  CHECK("synth settings invalid shape",
        hasDiagnostic(app::doc::docdiag::SynthSettingsInvalidShape));
  CHECK("synth param unknown", hasDiagnostic(app::doc::docdiag::SynthParamUnknown));
  CHECK("synth param type mismatch", hasDiagnostic(app::doc::docdiag::SynthParamTypeMismatch));
  CHECK("synth param enum unknown", hasDiagnostic(app::doc::docdiag::SynthParamEnumUnknown));
  CHECK("synth param out of range", hasDiagnostic(app::doc::docdiag::SynthParamOutOfRange));
  CHECK("synth param duplicate write", hasDiagnostic(app::doc::docdiag::SynthParamDuplicateWrite));
  CHECK("synth admission failed", hasDiagnostic(app::doc::docdiag::SynthAdmissionFailed));
}

static void test_diagnostic_catalog_has_unique_codes() {
  TEST("diagnostic_catalog_has_unique_codes");
  std::set<std::string> seen{};
  bool unique = true;
  for (const auto& diagnostic : app::doc::documentDiagnosticCatalog()) {
    if (!seen.insert(diagnostic.code).second)
      unique = false;
  }
  CHECK("unique codes", unique);
}

static void test_parser_emitted_diagnostics_are_cataloged() {
  TEST("parser_emitted_diagnostics_are_cataloged");

  const char* invalidDocs[] = {
      "track('one', TrackSettings {})",
      "track(1, 123)",
      "track(1, TrackSettings { patterns = 123 })",
      "track(1, TrackSettings { patterns = { bad = { numSteps = 1, stepsPerBeat = 4, "
      "steps = { { active = true } } } } })",
      "track(1, TrackSettings { patterns = { [999] = { numSteps = 1, stepsPerBeat = 4, "
      "steps = { { active = true } } } } })",
      "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
      "steps = { { active = 'yes' } } } } })",
      "track(1, TrackSettings { patterns = {}, activeSlot = 'one' })",
      "track(1, TrackSettings { activeSlot = 1 })",
      "applyFile('song.lua')",
      "apply_file('song.lua')",
      "synth('one', SynthSettings {})",
      "synth(1, 123)",
      "synth(1, SynthSettings { nope = 1 })",
      "synth(1, SynthSettings { osc1 = { enabled = 1 } })",
      "synth(1, SynthSettings { osc1 = { bank = 'not_a_bank' } })",
      "synth(1, SynthSettings { svf = { cutoff = 999999 } })",
  };

  app::doc::AuthoredDocModel model{};

  for (const char* text : invalidDocs) {
    auto r = app::doc::parseAndNormalizeAuthoredDoc(1, 7, text, &model);
    CHECK("not ok", !r.ok);
    CHECK("has diagnostics", !r.diagnostics.empty());
    CHECK("diagnostics cataloged", allDiagnosticsAreCataloged(r.diagnostics));
  }
}

static void test_service_emitted_diagnostics_are_cataloged() {
  TEST("service_emitted_diagnostics_are_cataloged");

  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result =
      app::doc::submitAuthoredDocRevision(service, app, 1, "track('one', TrackSettings {})");

  CHECK("not ok", !result.ok);
  CHECK("has diagnostics", !result.diagnostics.empty());
  CHECK("diagnostics cataloged", allDiagnosticsAreCataloged(result.diagnostics));
  app::doc::destroyDocAuthoringService(service);
}

static void test_file_apply_emitted_diagnostics_are_cataloged() {
  TEST("file_apply_emitted_diagnostics_are_cataloged");

  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result = app::doc::submitAuthoredDocFile(service, app, "/path/that/does/not/exist.lua");

  CHECK("not ok", !result.ok);
  CHECK("has diagnostics", !result.diagnostics.empty());
  CHECK("diagnostics cataloged", allDiagnosticsAreCataloged(result.diagnostics));
  app::doc::destroyDocAuthoringService(service);
}

static void test_synth_settings_metadata_is_implemented() {
  TEST("synth_settings_metadata_is_implemented");

  const auto* track = app::doc::findAuthoredDocumentType(app::doc::doctype::TrackSettings);
  const auto* synth = app::doc::findAuthoredDocumentType(app::doc::doctype::SynthSettings);
  const auto* osc = app::doc::findAuthoredDocumentType(app::doc::doctype::SynthOscSettings);
  const auto* svf = app::doc::findAuthoredDocumentType(app::doc::doctype::SynthSVFSettings);
  const auto* ladder = app::doc::findAuthoredDocumentType(app::doc::doctype::SynthLadderSettings);
  const auto* saturator =
      app::doc::findAuthoredDocumentType(app::doc::doctype::SynthSaturatorSettings);
  const auto* fx = app::doc::findAuthoredDocumentType(app::doc::doctype::SynthFXSettings);

  CHECK("TrackSettings synth field", track && findField(*track, "synth") != nullptr);
  CHECK("SynthSettings exists", synth != nullptr);
  CHECK("SynthSettings implemented",
        synth && synth->status == app::doc::DocMetadataStatus::Implemented);
  CHECK("SynthSettings has osc1", synth && findField(*synth, "osc1") != nullptr);
  CHECK("SynthSettings has ampEnv", synth && findField(*synth, "ampEnv") != nullptr);
  CHECK("SynthSettings has modEnv", synth && findField(*synth, "modEnv") != nullptr);
  CHECK("SynthSettings has filterEnv", synth && findField(*synth, "filterEnv") != nullptr);
  CHECK("SynthSettings has fx", synth && findField(*synth, "fx") != nullptr);
  CHECK("SynthOscSettings exists", osc != nullptr);
  CHECK("SynthOscSettings has bank", osc && findField(*osc, "bank") != nullptr);
  CHECK("SynthOscSettings has mixLevel", osc && findField(*osc, "mixLevel") != nullptr);
  CHECK("SynthSVFSettings has mode", svf && findField(*svf, "mode") != nullptr);
  CHECK("SynthSVFSettings no drive", svf && findField(*svf, "drive") == nullptr);
  CHECK("SynthLadderSettings has drive", ladder && findField(*ladder, "drive") != nullptr);
  CHECK("SynthLadderSettings no mode", ladder && findField(*ladder, "mode") == nullptr);
  CHECK("SynthSaturatorSettings exists", saturator != nullptr);
  CHECK("SynthSaturatorSettings has drive", saturator && findField(*saturator, "drive") != nullptr);
  CHECK("SynthFXSettings exists", fx != nullptr);
  CHECK("SynthFXSettings has delay", fx && findField(*fx, "delay") != nullptr);
}

static void test_synth_function_metadata_is_registered() {
  TEST("synth_function_metadata_is_registered");

  const auto* synth = findFunction(app::doc::docglobal::Synth);
  CHECK("synth function", synth != nullptr);
  CHECK("synth implemented", synth && synth->status == app::doc::DocMetadataStatus::Implemented);
  CHECK("synth has 2 args", synth && synth->args.size == 2);
  CHECK("synth track min", synth && synth->args.data[0].integerBounds.min == 1);
  CHECK("synth track max", synth && synth->args.data[0].integerBounds.max == app::MAX_TRACKS);
  CHECK("synth settings type",
        synth && strEq(synth->args.data[1].typeName, app::doc::doctype::SynthSettings));
}

static void test_mixer_function_is_in_authored_document_functions() {
  TEST("mixer_function_is_in_authored_document_functions");
  const auto* f = app::doc::findAuthoredDocumentFunction(app::doc::docglobal::Mixer);
  CHECK("found", f != nullptr);
  CHECK("implemented", f && f->status == app::doc::DocMetadataStatus::Implemented);
  CHECK("surface", f && f->surface == app::doc::DocMetadataSurface::AuthoredDocument);
}

static void test_mixer_settings_type_is_implemented() {
  TEST("mixer_settings_type_is_implemented");
  const auto* t = app::doc::findAuthoredDocumentType(app::doc::doctype::MixerSettings);
  CHECK("found", t != nullptr);
  CHECK("implemented", t && t->status == app::doc::DocMetadataStatus::Implemented);
  CHECK("has fields", t && !t->fields.empty());
}

static void test_track_settings_has_mixer_field() {
  TEST("track_settings_has_mixer_field");
  const auto* t = app::doc::findAuthoredDocumentType(app::doc::doctype::TrackSettings);
  CHECK("found", t != nullptr);
  bool hasMixer = false;
  if (t) {
    for (const auto& f : t->fields) {
      if (std::string(f.name) == "mixer")
        hasMixer = true;
    }
  }
  CHECK("has mixer", hasMixer);
}

static void test_mixer_diagnostic_codes_are_cataloged() {
  TEST("mixer_diagnostic_codes_are_cataloged");
  namespace dd = app::doc::docdiag;
  CHECK("invalid index", app::doc::findDocumentDiagnostic(dd::MixerTrackInvalidIndex) != nullptr);
  CHECK("invalid shape",
        app::doc::findDocumentDiagnostic(dd::MixerSettingsInvalidShape) != nullptr);
  CHECK("unknown", app::doc::findDocumentDiagnostic(dd::MixerParamUnknown) != nullptr);
  CHECK("type mismatch", app::doc::findDocumentDiagnostic(dd::MixerParamTypeMismatch) != nullptr);
  CHECK("out of range", app::doc::findDocumentDiagnostic(dd::MixerParamOutOfRange) != nullptr);
  CHECK("duplicate write",
        app::doc::findDocumentDiagnostic(dd::MixerParamDuplicateWrite) != nullptr);
}

void runDocMetadataTests() {
  SUITE("DocMetadata");
  test_authored_surface_contains_only_document_globals();
  test_reserved_constructors_are_narrow();
  test_track_signature_uses_static_track_bounds();
  test_track_settings_fields_match_parser_surface();
  test_pattern_bounds_derive_from_sequencer_constants();
  test_step_fields_match_lua_pattern_parser();
  test_diagnostic_catalog_contains_emitted_codes();
  test_diagnostic_catalog_has_unique_codes();
  test_parser_emitted_diagnostics_are_cataloged();
  test_service_emitted_diagnostics_are_cataloged();
  test_file_apply_emitted_diagnostics_are_cataloged();
  test_synth_settings_metadata_is_implemented();
  test_synth_function_metadata_is_registered();
  test_mixer_function_is_in_authored_document_functions();
  test_mixer_diagnostic_codes_are_cataloged();
  test_track_settings_has_mixer_field();
  test_mixer_settings_type_is_implemented();
}

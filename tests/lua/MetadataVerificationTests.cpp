#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/AppParams.h"
#include "app/Constants.h"
#include "app/Sequencer.h"
#include "app/doc/DocMetadata.h"
#include "lua/LuaRuntimeMetadata.h"
#include "lua/bindings/LuaBindings.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr const char* kAuthoredStubPath =
    "generated/luals/authored_document/meh_groovebox_authored.lua";
constexpr const char* kRuntimeStubPath = "generated/luals/runtime_lua/meh_groovebox_runtime.lua";

std::string readRequiredFile(const char* path) {
  std::ifstream in(path, std::ios::binary);
  CHECK((std::string("open ") + path).c_str(), static_cast<bool>(in));
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

bool strEq(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

void checkContains(const std::string& text, const std::string& needle, const char* label) {
  CHECK(label, contains(text, needle));
}

void checkNotContains(const std::string& text, const std::string& needle, const char* label) {
  CHECK(label, !contains(text, needle));
}

bool hasAuthoredFunction(const char* name) {
  return app::doc::findAuthoredDocumentFunction(name) != nullptr;
}

bool hasAuthoredConstructor(const char* name) {
  for (const auto& ctor : app::doc::authoredDocumentConstructors()) {
    if (strEq(ctor, name))
      return true;
  }
  return false;
}

bool hasRuntimeGlobal(const char* name) {
  return lua::findRuntimeLuaGlobal(name) != nullptr;
}

const lua::RuntimeLuaFunctionMetadata* findMethod(const lua::RuntimeLuaTableMetadata& table,
                                                  const char* name) {
  for (const auto& method : table.methods) {
    if (strEq(method.name, name))
      return &method;
  }
  return nullptr;
}

bool luaGlobalIs(lua_State* L, const char* name, int luaType) {
  lua_getglobal(L, name);
  const bool ok = lua_type(L, -1) == luaType;
  lua_pop(L, 1);
  return ok;
}

bool luaGlobalIsNil(lua_State* L, const char* name) {
  return luaGlobalIs(L, name, LUA_TNIL);
}

bool luaTableHasFunction(lua_State* L, const char* tableName, const char* methodName) {
  lua_getglobal(L, tableName);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return false;
  }
  lua_getfield(L, -1, methodName);
  const bool ok = lua_isfunction(L, -1);
  lua_pop(L, 2);
  return ok;
}

bool containsProxyField(const std::vector<lua::RuntimeLuaProxyFieldMetadata>& fields,
                        const char* table,
                        const char* field) {
  for (const auto& item : fields) {
    if (item.table == table && item.field == field)
      return true;
  }
  return false;
}

std::string number(int value) {
  return std::to_string(value);
}

struct LuaFixture {
  app::AppContext app{};
  lua_State* L = nullptr;

  LuaFixture() {
    L = luaL_newstate();
    luaL_openlibs(L);
    lua::bindings::registerSynthBindings(L, app);
  }

  ~LuaFixture() {
    if (L)
      lua_close(L);
  }
};

} // namespace

static void test_canonical_apply_file_only_exists_in_runtime_surface() {
  TEST("canonical_apply_file_only_exists_in_runtime_surface");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  CHECK("runtime metadata has applyFile", hasRuntimeGlobal(lua::rtglobal::ApplyFile));
  CHECK("runtime metadata no apply_file", !hasRuntimeGlobal("apply_file"));
  CHECK("authored metadata no applyFile", !hasAuthoredFunction("applyFile"));
  CHECK("authored metadata no apply_file", !hasAuthoredFunction("apply_file"));

  checkContains(runtime, "function applyFile(", "runtime stub applyFile");
  checkNotContains(runtime, "apply_file", "runtime stub no apply_file");
  checkNotContains(authored, "applyFile", "authored stub no applyFile");
  checkNotContains(authored, "apply_file", "authored stub no apply_file");
}

static void test_authored_constructors_only_exist_in_authored_surface() {
  TEST("authored_constructors_only_exist_in_authored_surface");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  CHECK("TrackSettings metadata", hasAuthoredConstructor(app::doc::docctor::TrackSettings));
  CHECK("SynthSettings metadata", hasAuthoredConstructor(app::doc::docctor::SynthSettings));
  CHECK("MixerSettings metadata", hasAuthoredConstructor(app::doc::docctor::MixerSettings));

  CHECK("runtime no TrackSettings", !hasRuntimeGlobal(app::doc::docctor::TrackSettings));
  CHECK("runtime no SynthSettings", !hasRuntimeGlobal(app::doc::docctor::SynthSettings));
  CHECK("runtime no MixerSettings", !hasRuntimeGlobal(app::doc::docctor::MixerSettings));

  checkContains(authored, "function TrackSettings(", "authored TrackSettings");
  checkContains(authored, "function SynthSettings(", "authored SynthSettings");
  checkContains(authored, "function MixerSettings(", "authored MixerSettings");
  checkNotContains(runtime, "function TrackSettings(", "runtime no TrackSettings");
  checkNotContains(runtime, "function SynthSettings(", "runtime no SynthSettings");
  checkNotContains(runtime, "function MixerSettings(", "runtime no MixerSettings");
}

static void test_document_track_and_runtime_seq_track_do_not_collapse() {
  TEST("document_track_and_runtime_seq_track_do_not_collapse");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  CHECK("authored metadata track", hasAuthoredFunction(app::doc::docglobal::Track));
  CHECK("runtime metadata no top-level track", !hasRuntimeGlobal(app::doc::docglobal::Track));

  const auto* seq = lua::findRuntimeLuaTable(lua::rtglobal::Seq);
  CHECK("runtime seq table", seq != nullptr);
  CHECK("runtime seq.track metadata", seq && findMethod(*seq, lua::rtmethod::Track) != nullptr);

  checkContains(authored, "function track(", "authored function track");
  checkContains(runtime, "function seq.track(", "runtime seq.track");
  checkNotContains(runtime, "function track(", "runtime no top-level track");

  CHECK("authored metadata synth", hasAuthoredFunction(app::doc::docglobal::Synth));
  checkContains(authored, "function synth(", "authored function synth");
  checkNotContains(runtime, "function synth(", "runtime no top-level synth");
}

static void test_diagnostic_catalog_covers_parser_service_codes() {
  TEST("diagnostic_catalog_covers_parser_service_codes");
  const char* codes[] = {
      app::doc::docdiag::SequencerTrackInvalidIndex,
      app::doc::docdiag::SequencerTrackInvalidSettings,
      app::doc::docdiag::SequencerPatternsInvalidShape,
      app::doc::docdiag::SequencerPatternSlotInvalidKey,
      app::doc::docdiag::SequencerPatternSlotOutOfRange,
      app::doc::docdiag::SequencerPatternInvalidShape,
      app::doc::docdiag::SequencerActiveSlotInvalidType,
      app::doc::docdiag::SequencerActiveSlotOutOfRange,
      app::doc::docdiag::SequencerActiveSlotMissingPatterns,
      app::doc::docdiag::SequencerActiveSlotEmptySlot,
      app::doc::docdiag::SequencerAdmissionFailed,
      app::doc::docdiag::DocumentLuaStateFailed,
      app::doc::docdiag::DocumentLuaEvalFailed,
      app::doc::docdiag::DocumentFileReadFailed,
      app::doc::docdiag::SynthTrackInvalidIndex,
      app::doc::docdiag::SynthSettingsInvalidShape,
      app::doc::docdiag::SynthParamUnknown,
      app::doc::docdiag::SynthParamTypeMismatch,
      app::doc::docdiag::SynthParamEnumUnknown,
      app::doc::docdiag::SynthParamOutOfRange,
      app::doc::docdiag::SynthParamDuplicateWrite,
      app::doc::docdiag::SynthAdmissionFailed,
  };

  for (const char* code : codes) {
    CHECK((std::string("diagnostic catalog has ") + code).c_str(),
          app::doc::findDocumentDiagnostic(code) != nullptr);
  }
}

static void test_static_bounds_match_generated_stub_comments() {
  TEST("static_bounds_match_generated_stub_comments");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  const auto* track = app::doc::findAuthoredDocumentFunction(app::doc::docglobal::Track);
  CHECK("authored track function", track != nullptr);
  CHECK("authored track min", track && track->args.data[0].integerBounds.min == 1);
  CHECK("authored track max", track && track->args.data[0].integerBounds.max == app::MAX_TRACKS);

  const auto* seq = lua::findRuntimeLuaTable(lua::rtglobal::Seq);
  const auto* seqTrack = seq ? findMethod(*seq, lua::rtmethod::Track) : nullptr;
  CHECK("runtime seq.track", seqTrack != nullptr);
  CHECK("runtime seq.track max",
        seqTrack && seqTrack->args.data[0].integerBounds.max == app::MAX_TRACKS);

  const auto* midi = lua::findRuntimeLuaTable(lua::rtglobal::Midi);
  const auto* channel = midi ? findMethod(*midi, lua::rtmethod::Channel) : nullptr;
  CHECK("midi.channel", channel != nullptr);
  CHECK("midi.channel max",
        channel && channel->args.data[0].integerBounds.max == app::MAX_MIDI_CHANNELS);

  const auto* seqTrackType = lua::findRuntimeLuaUserdataType(lua::rttype::SeqTrack);
  const auto* replacePattern =
      seqTrackType ? findMethod(*seqTrackType, lua::rtmethod::ReplacePattern) : nullptr;
  const auto* setNumSteps =
      seqTrackType ? findMethod(*seqTrackType, lua::rtmethod::SetNumSteps) : nullptr;
  CHECK("replacePattern slot max",
        replacePattern &&
            replacePattern->args.data[0].integerBounds.max == app::sequencer::PATTERNS_PER_LANE);
  CHECK("setNumSteps max",
        setNumSteps &&
            setNumSteps->args.data[0].integerBounds.max == app::sequencer::MAX_PATTERN_STEPS);

  checkContains(authored, "1.." + number(app::MAX_TRACKS), "authored track bound");
  checkContains(authored, "1.." + number(app::sequencer::PATTERNS_PER_LANE), "authored slot bound");
  checkContains(authored, "1.." + number(app::sequencer::MAX_PATTERN_STEPS), "authored step bound");
  checkContains(authored,
                "1.." + number(app::sequencer::MAX_STEPS_PER_BEAT),
                "authored steps per beat bound");
  checkContains(authored, number(app::sequencer::MAX_LOCKS_PER_STEP), "authored lock bound");
  checkContains(runtime, "function seq.track(", "runtime seq.track rendered");
}

static void test_generated_outputs_have_expected_meta_names() {
  TEST("generated_outputs_have_expected_meta_names");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  checkContains(authored, "---@meta meh_groovebox_authored", "authored meta");
  checkContains(runtime, "---@meta meh_groovebox_runtime", "runtime meta");
}

static void test_generated_outputs_are_marked_do_not_edit() {
  TEST("generated_outputs_are_marked_do_not_edit");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  checkContains(authored, "@generated from C++ metadata", "authored generated");
  checkContains(runtime, "@generated from C++ metadata", "runtime generated");
  checkContains(authored, "Do not edit", "authored do not edit");
  checkContains(runtime, "Do not edit", "runtime do not edit");
  checkNotContains(authored, "Generated at", "authored no timestamp");
  checkNotContains(runtime, "Generated at", "runtime no timestamp");
  checkNotContains(authored, "/Users/", "authored no absolute path");
  checkNotContains(runtime, "/Users/", "runtime no absolute path");
}

static void test_param_proxy_generation_has_representative_fields() {
  TEST("param_proxy_generation_has_representative_fields");

  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  std::vector<lua::RuntimeLuaProxyFieldMetadata> engineFields{};
  lua::collectRuntimeLuaEngineParamProxyFields(engineFields);
  CHECK("engine proxy metadata nonempty", !engineFields.empty());

  std::vector<lua::RuntimeLuaProxyFieldMetadata> appFields{};
  lua::collectRuntimeLuaAppParamProxyFields(appFields);
  CHECK("app proxy metadata nonempty", !appFields.empty());

  const bool engineRepresentative = containsProxyField(engineFields, "osc1", "freq") ||
                                    containsProxyField(engineFields, "osc1", "bank") ||
                                    containsProxyField(engineFields, "fx.reverb", "decay") ||
                                    containsProxyField(engineFields, "fx.reverb", "mix");
  CHECK("representative engine proxy metadata", engineRepresentative);

  const bool appRepresentative = containsProxyField(appFields, "mixer", "gain") ||
                                 containsProxyField(appFields, "mixer", "masterGain");
  CHECK("representative app proxy metadata", appRepresentative);

  if (engineRepresentative) {
    checkContains(runtime, "osc1", "runtime has osc1 proxy or class");
  }
  if (appRepresentative) {
    checkContains(runtime, "mixer", "runtime has mixer proxy or table");
  }
}

static void test_runtime_visible_globals_match_metadata() {
  TEST("runtime_visible_globals_match_metadata");

  LuaFixture fixture{};
  CHECK("lua state", fixture.L != nullptr);

  for (const auto& global : lua::runtimeLuaGlobals()) {
    if (global.kind == lua::RuntimeLuaSymbolKind::ProxyTable)
      continue;

    if (global.function) {
      CHECK((std::string("global function ") + global.name).c_str(),
            luaGlobalIs(fixture.L, global.name, LUA_TFUNCTION));
    } else if (global.table) {
      CHECK((std::string("global table ") + global.name).c_str(),
            luaGlobalIs(fixture.L, global.name, LUA_TTABLE));
    } else {
      CHECK((std::string("global exists ") + global.name).c_str(),
            !luaGlobalIs(fixture.L, global.name, LUA_TNIL));
    }
  }

  CHECK("applyFile function", luaGlobalIs(fixture.L, lua::rtglobal::ApplyFile, LUA_TFUNCTION));
  CHECK("apply_file nil", luaGlobalIsNil(fixture.L, "apply_file"));
  CHECK("TrackSettings nil", luaGlobalIsNil(fixture.L, app::doc::docctor::TrackSettings));
  CHECK("SynthSettings nil", luaGlobalIsNil(fixture.L, app::doc::docctor::SynthSettings));
  CHECK("MixerSettings nil", luaGlobalIsNil(fixture.L, app::doc::docctor::MixerSettings));
  CHECK("top-level track nil", luaGlobalIsNil(fixture.L, app::doc::docglobal::Track));

  CHECK("transport.play",
        luaTableHasFunction(fixture.L, lua::rtglobal::Transport, lua::rtmethod::Play));
  CHECK("transport.stop",
        luaTableHasFunction(fixture.L, lua::rtglobal::Transport, lua::rtmethod::Stop));
  CHECK("midi.routes", luaTableHasFunction(fixture.L, lua::rtglobal::Midi, lua::rtmethod::Routes));
  CHECK("seq.track", luaTableHasFunction(fixture.L, lua::rtglobal::Seq, lua::rtmethod::Track));
  CHECK("preset.list", luaTableHasFunction(fixture.L, lua::rtglobal::Preset, lua::rtmethod::List));
}

static void test_document_synth_and_runtime_param_surfaces_do_not_collapse() {
  TEST("document_synth_and_runtime_param_surfaces_do_not_collapse");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  CHECK("authored metadata synth", hasAuthoredFunction(app::doc::docglobal::Synth));
  CHECK("runtime metadata no top-level synth", !hasRuntimeGlobal(app::doc::docglobal::Synth));

  checkContains(authored, "function synth(", "authored function synth");
  checkNotContains(runtime, "function synth(", "runtime no top-level synth");
  checkContains(authored, "function SynthSettings(", "authored SynthSettings");
  checkNotContains(runtime, "function SynthSettings(", "runtime no SynthSettings");
}
static void test_mixer_admission_diagnostic_is_cataloged() {
  TEST("mixer_admission_diagnostic_is_cataloged");
  const auto* d = app::doc::findDocumentDiagnostic(app::doc::docdiag::MixerAdmissionFailed);
  CHECK("found", d != nullptr);
  CHECK("severity", d && d->severity == app::doc::DiagnosticSeverity::Error);
  CHECK("source", d && d->source == app::doc::DiagnosticSource::GrooveboxAdmission);
}

static void test_all_mixer_diagnostic_codes_are_cataloged() {
  TEST("all_mixer_diagnostic_codes_are_cataloged");
  namespace dd = app::doc::docdiag;
  CHECK("invalid index", app::doc::findDocumentDiagnostic(dd::MixerTrackInvalidIndex) != nullptr);
  CHECK("invalid shape",
        app::doc::findDocumentDiagnostic(dd::MixerSettingsInvalidShape) != nullptr);
  CHECK("unknown", app::doc::findDocumentDiagnostic(dd::MixerParamUnknown) != nullptr);
  CHECK("type mismatch", app::doc::findDocumentDiagnostic(dd::MixerParamTypeMismatch) != nullptr);
  CHECK("out of range", app::doc::findDocumentDiagnostic(dd::MixerParamOutOfRange) != nullptr);
  CHECK("duplicate write",
        app::doc::findDocumentDiagnostic(dd::MixerParamDuplicateWrite) != nullptr);
  CHECK("admission failed", app::doc::findDocumentDiagnostic(dd::MixerAdmissionFailed) != nullptr);
}
void runMetadataVerificationTests() {
  SUITE("LuaMetadataVerification");
  test_canonical_apply_file_only_exists_in_runtime_surface();
  test_authored_constructors_only_exist_in_authored_surface();
  test_document_track_and_runtime_seq_track_do_not_collapse();
  test_diagnostic_catalog_covers_parser_service_codes();
  test_static_bounds_match_generated_stub_comments();
  test_generated_outputs_have_expected_meta_names();
  test_generated_outputs_are_marked_do_not_edit();
  test_param_proxy_generation_has_representative_fields();
  test_runtime_visible_globals_match_metadata();
  test_document_synth_and_runtime_param_surfaces_do_not_collapse();
  test_mixer_admission_diagnostic_is_cataloged();
  test_all_mixer_diagnostic_codes_are_cataloged();
}

#include "TestRunner.h"

#include "app/Constants.h"
#include "app/Sequencer.h"

#include <fstream>
#include <sstream>
#include <string>

namespace {

constexpr const char* kAuthoredStubPath =
    "generated/luals/authored_document/meh_groovebox_authored.lua";
constexpr const char* kRuntimeStubPath = "generated/luals/runtime_lua/meh_groovebox_runtime.lua";
constexpr const char* kGeneratedReadmePath = "generated/luals/README.md";

std::string readRequiredFile(const char* path) {
  std::ifstream in(path, std::ios::binary);
  auto str = std::string("open ") + path;
  CHECK(str.c_str(), static_cast<bool>(in));
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
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

std::string number(int value) {
  return std::to_string(value);
}

} // namespace

static void test_authored_stub_contains_document_surface() {
  TEST("authored_stub_contains_document_surface");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  checkContains(authored, "---@meta meh_groovebox_authored", "meta name");
  checkContains(authored, "function track(", "document track");
  checkContains(authored, "function TrackSettings(", "TrackSettings");
  checkContains(authored, "function SynthSettings(", "SynthSettings");
  checkContains(authored, "function MixerSettings(", "MixerSettings");
  checkContains(authored, "---@class", "class declarations");
}

static void test_authored_stub_excludes_runtime_surface() {
  TEST("authored_stub_excludes_runtime_surface");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  checkNotContains(authored, "applyFile", "no applyFile");
  checkNotContains(authored, "apply_file", "no apply_file");
  checkNotContains(authored, "transport", "no transport");
  checkNotContains(authored, "seq =", "no seq table");
  checkNotContains(authored, "function seq.", "no seq methods");
}

static void test_runtime_stub_contains_runtime_surface() {
  TEST("runtime_stub_contains_runtime_surface");

  const std::string runtime = readRequiredFile(kRuntimeStubPath);
  checkContains(runtime, "---@meta meh_groovebox_runtime", "meta name");
  checkContains(runtime, "function applyFile(", "applyFile");
  checkContains(runtime, "transport", "transport");
  checkContains(runtime, "midi", "midi");
  checkContains(runtime, "seq", "seq");
  checkContains(runtime, "preset", "preset");
  checkContains(runtime, "mod", "mod");
  checkContains(runtime, "fm", "fm");
  checkContains(runtime, "fx", "fx");
  checkContains(runtime, "signal", "signal");
  checkContains(runtime, "mixer", "mixer");
}

static void test_runtime_stub_excludes_authored_surface() {
  TEST("runtime_stub_excludes_authored_surface");

  const std::string runtime = readRequiredFile(kRuntimeStubPath);
  checkNotContains(runtime, "apply_file", "no apply_file");
  checkNotContains(runtime, "function TrackSettings(", "no TrackSettings");
  checkNotContains(runtime, "function SynthSettings(", "no SynthSettings");
  checkNotContains(runtime, "function MixerSettings(", "no MixerSettings");
  checkNotContains(runtime, "function track(", "no document track");
}

static void test_generated_stubs_have_no_timestamps() {
  TEST("generated_stubs_have_no_timestamps");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);

  checkNotContains(authored, "Generated at", "authored no Generated at");
  checkNotContains(runtime, "Generated at", "runtime no Generated at");
  checkNotContains(authored, "generated at", "authored no generated at");
  checkNotContains(runtime, "generated at", "runtime no generated at");
  checkNotContains(authored, "/Users/", "authored no absolute user path");
  checkNotContains(runtime, "/Users/", "runtime no absolute user path");
  checkNotContains(authored, "0x", "authored no pointer-looking values");
  checkNotContains(runtime, "0x", "runtime no pointer-looking values");
}

static void test_generated_stubs_are_nonempty() {
  TEST("generated_stubs_are_nonempty");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  const std::string runtime = readRequiredFile(kRuntimeStubPath);
  const std::string readme = readRequiredFile(kGeneratedReadmePath);

  CHECK("authored meaningful size", authored.size() > 300);
  CHECK("runtime meaningful size", runtime.size() > 300);
  CHECK("readme meaningful size", readme.size() > 100);
}

static void test_authored_stub_mentions_static_bounds() {
  TEST("authored_stub_mentions_static_bounds");

  const std::string authored = readRequiredFile(kAuthoredStubPath);
  checkContains(authored, "1.." + number(app::MAX_TRACKS), "track bounds");
  checkContains(authored, "1.." + number(app::sequencer::PATTERNS_PER_LANE), "pattern slot bounds");
  checkContains(authored, "1.." + number(app::sequencer::MAX_PATTERN_STEPS), "pattern step bounds");
  checkContains(authored,
                "1.." + number(app::sequencer::MAX_STEPS_PER_BEAT),
                "steps per beat bounds");
  checkContains(authored, number(app::sequencer::MAX_LOCKS_PER_STEP), "lock count bound");
}

static void test_generated_readme_documents_commands() {
  TEST("generated_readme_documents_commands");

  const std::string readme = readRequiredFile(kGeneratedReadmePath);
  checkContains(readme, "make luals-stubs", "update command");
  checkContains(readme, "make check-luals-stubs", "check command");
  checkContains(readme, "scripts/run_tests.sh", "full gate command");
  checkContains(readme, "Do not edit", "do not edit");
  checkNotContains(readme, "Python", "no Python direction");
  checkNotContains(readme, ".json", "no JSON metadata direction");
}

void runLuaLSStubGenerationTests() {
  SUITE("LuaLSStubGeneration");
  test_authored_stub_contains_document_surface();
  test_authored_stub_excludes_runtime_surface();
  test_runtime_stub_contains_runtime_surface();
  test_runtime_stub_excludes_authored_surface();
  test_generated_stubs_have_no_timestamps();
  test_generated_stubs_are_nonempty();
  test_authored_stub_mentions_static_bounds();
  test_generated_readme_documents_commands();
}

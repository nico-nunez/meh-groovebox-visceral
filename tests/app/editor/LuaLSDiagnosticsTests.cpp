#include "TestRunner.h"

#include "app/editor/AuthoredDocEditor.h"
#include "app/editor/LuaLSDiagnostics.h"

#include <string>
#include <vector>

namespace {

bool containsDiagnostic(const std::vector<app::editor::LuaLSDiagnostic>& diagnostics,
                        const char* code,
                        uint32_t line,
                        uint32_t column) {
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.code == code && diagnostic.line == line && diagnostic.column == column)
      return true;
  }
  return false;
}

} // namespace

static void test_parse_luals_pretty_output_extracts_undefined_globals() {
  TEST("parse_luals_pretty_output_extracts_undefined_globals");

  const std::string output =
      "Initializing ...\n"
      "buffer.lua:1:1 [Warning] Undefined global `applyFile`. (undefined-global)\n"
      "    applyFile(\"song.lua\")\n"
      "    ^^^^^^^^^\n"
      "buffer.lua:2:1 [Warning] Undefined global `transport`. (undefined-global)\n"
      "    transport.play()\n"
      "    ^^^^^^^^^\n"
      "Diagnosis complete, 2 problems found\n";

  const auto diagnostics = app::editor::parseLuaLSPrettyOutput(output);

  CHECK("two diagnostics", diagnostics.size() == 2);
  CHECK("applyFile undefined", containsDiagnostic(diagnostics, "undefined-global", 1, 1));
  CHECK("transport undefined", containsDiagnostic(diagnostics, "undefined-global", 2, 1));
}

static void test_parse_luals_pretty_output_ignores_non_fixture_lines() {
  TEST("parse_luals_pretty_output_ignores_non_fixture_lines");

  const std::string output = "Diagnosis completed, no problems found\n"
                             "other.lua:1:1 [Warning] Undefined global `x`. (undefined-global)\n";

  const auto diagnostics = app::editor::parseLuaLSPrettyOutput(output);

  CHECK("no diagnostics", diagnostics.empty());
}

static void test_authored_luals_library_root_is_authored_only() {
  TEST("authored_luals_library_root_is_authored_only");

  const std::string root = app::editor::authoredLuaLSLibraryRoot();

  CHECK("authored root", root.find("generated/authored_document") != std::string::npos);
  CHECK("not runtime root", root.find("runtime_lua") == std::string::npos);
}

static void test_mark_buffer_edited_marks_luals_pending() {
  TEST("mark_buffer_edited_marks_luals_pending");

  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.text = "applyFile(\"song.lua\")";
  app::editor::markBufferTextChanged(editor);

  CHECK("edit serial incremented", editor.editSerial == 1);
  CHECK("luals pending", editor.luals.status == app::editor::LanguageServiceStatus::Pending);
}

static void test_parse_luals_pretty_output_accepts_absolute_buffer_path() {
  TEST("parse_luals_pretty_output_accepts_absolute_buffer_path");

  const std::string output = "/private/tmp/meh-groovebox-luals-editor/buffer.lua:1:1 [Warning] "
                             "Undefined global `applyFile`. (undefined-global)\n"
                             "    applyFile(\"song.lua\")\n"
                             "    ^^^^^^^^^\n"
                             "Diagnosis complete, 1 problems found\n";

  const auto diagnostics = app::editor::parseLuaLSPrettyOutput(output);

  CHECK("one diagnostic", diagnostics.size() == 1);
  CHECK("applyFile undefined", containsDiagnostic(diagnostics, "undefined-global", 1, 1));
}

void runLuaLSDiagnosticsTests() {
  SUITE("LuaLSDiagnostics");
  test_parse_luals_pretty_output_extracts_undefined_globals();
  test_parse_luals_pretty_output_ignores_non_fixture_lines();
  test_authored_luals_library_root_is_authored_only();
  test_mark_buffer_edited_marks_luals_pending();
  test_parse_luals_pretty_output_accepts_absolute_buffer_path();
}

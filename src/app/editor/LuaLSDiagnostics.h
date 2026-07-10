#pragma once

#include "app/editor/AuthoredDocEditor.h"

#include <string>
#include <vector>

namespace app::editor {

struct LuaLSRunResult {
  bool lualsFound = false;
  bool commandSucceeded = false;
  std::string message{};
  std::vector<LuaLSDiagnostic> diagnostics{};
};

std::string authoredLuaLSLibraryRoot();

std::string findLuaLanguageServerBinary();
std::vector<LuaLSDiagnostic> parseLuaLSPrettyOutput(const std::string& output);

LuaLSRunResult runLuaLSDiagnosticsForText(const std::string& text);

void maybeStartLuaLSDiagnostics(AuthoredDocEditorState& editor);
void collectFinishedLuaLSDiagnostics(AuthoredDocEditorState& editor);

} // namespace app::editor

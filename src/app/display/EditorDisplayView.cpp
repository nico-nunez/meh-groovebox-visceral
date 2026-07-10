#include "EditorDisplayView.h"

#include "app/AppContext.h"
#include "app/display/ExtEditorDisplayView.h"
#include "app/doc/DocDiagnostics.h"
#include "app/editor/AuthoredDocEditor.h"
#include "app/editor/LuaLSDiagnostics.h"

#include "imgui.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace app::display {

namespace {
using editor::LanguageServiceStatus;

constexpr std::size_t kPathCapacity = 1024;

struct EditorUiScratch {
  char pathBuffer[kPathCapacity]{};
  bool initialized = false;
};

EditorUiScratch gScratch{};

static int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto* text = static_cast<std::string*>(data->UserData);
    text->resize(static_cast<size_t>(data->BufTextLen));
    data->Buf = text->data();
    data->BufSize = static_cast<int>(text->capacity());
  }
  return 0;
}

const char* severityLabel(app::doc::DiagnosticSeverity severity) {
  switch (severity) {
  case app::doc::DiagnosticSeverity::Error:
    return "error";
  case app::doc::DiagnosticSeverity::Warning:
    return "warning";
  case app::doc::DiagnosticSeverity::Info:
    return "info";
  }
  return "unknown";
}

std::size_t countBufferLines(const char* text) {
  std::size_t lines = 1;
  for (const char* cursor = text; *cursor != '\0'; ++cursor) {
    if (*cursor == '\n')
      ++lines;
  }
  return lines;
}

float lineNumberGutterWidth(std::size_t lineCount) {
  char lastLine[32]{};
  std::snprintf(lastLine, sizeof(lastLine), "%zu", lineCount);
  return ImGui::CalcTextSize(lastLine).x + ImGui::GetStyle().FramePadding.x * 2.0f;
}

void drawLineNumberGutter(std::size_t lineCount, float gutterWidth, float editorHeight) {
  ImGui::BeginChild("AuthoredDocumentLineNumbers",
                    ImVec2(gutterWidth, editorHeight),
                    false,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  const float lineH = ImGui::GetTextLineHeight();
  const float paddingY = ImGui::GetStyle().FramePadding.y;

  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
  for (std::size_t line = 1; line <= lineCount; ++line) {
    char label[32]{};
    std::snprintf(label, sizeof(label), "%zu", line);

    ImGui::SetCursorPosY(paddingY + static_cast<float>(line - 1) * lineH);
    const float labelWidth = ImGui::CalcTextSize(label).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + gutterWidth - labelWidth -
                         ImGui::GetStyle().FramePadding.x);
    ImGui::TextUnformatted(label);
  }
  ImGui::PopStyleColor();

  ImGui::EndChild();
}

void forceScratchPath(const std::string& path) {
  std::snprintf(gScratch.pathBuffer, sizeof(gScratch.pathBuffer), "%s", path.c_str());
}

void drawStatusLine(const AuthoredDocEditorState& editor) {
  ImGui::Text("revision %u  last applied %u  %s",
              editor.buffer.applyRevision,
              editor.buffer.lastAppliedRevision,
              editor.buffer.dirty ? "dirty" : "clean");

  if (!editor.fileMessage.text.empty())
    ImGui::TextUnformatted(editor.fileMessage.text.c_str());
  if (!editor.applyMessage.text.empty())
    ImGui::TextUnformatted(editor.applyMessage.text.c_str());
}

void drawDiagnosticList(AuthoredDocEditorState& editor) {
  ImGui::SeparatorText("Backend diagnostics");

  if (editor.backendDiagnostics.empty()) {
    ImGui::TextDisabled("No backend diagnostics");
    return;
  }

  for (std::size_t i = 0; i < editor.backendDiagnostics.size(); ++i) {
    const auto& diagnostic = editor.backendDiagnostics[i];
    ImGui::PushID(static_cast<int>(i));
    const bool hasSpan = diagnostic.span.line != 0;
    if (!hasSpan)
      ImGui::BeginDisabled();

    const std::string label = std::string(severityLabel(diagnostic.severity)) + " " +
                              diagnostic.code + "  line " + std::to_string(diagnostic.span.line) +
                              ":" + std::to_string(diagnostic.span.column);

    if (ImGui::Selectable(label.c_str(), false) && hasSpan)
      requestDiagnosticJump(editor, diagnostic);

    if (!hasSpan)
      ImGui::EndDisabled();

    if (!diagnostic.message.empty())
      ImGui::TextWrapped("%s", diagnostic.message.c_str());
    if (!diagnostic.relatedTarget.empty())
      ImGui::TextDisabled("%s", diagnostic.relatedTarget.c_str());

    ImGui::PopID();
  }
}

void consumeJumpRequest(AuthoredDocEditorState& editor) {
  if (!editor.jumpRequest.pending)
    return;

  // InputTextMultiline does not expose stable public caret positioning.
  // Slice 1 satisfies click-to-jump by scrolling the editor child to the line.
  // If a later text widget exposes caret control, keep this request shape and
  // upgrade the implementation behind it.
  const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
  const float line =
      editor.jumpRequest.line > 0 ? static_cast<float>(editor.jumpRequest.line - 1) : 0.0f;
  ImGui::SetScrollY(line * lineHeight);
  clearDiagnosticJump(editor);
}

// ====================
// LuaLS Diagnostics
// ====================
const char* lualsStatusLabel(LanguageServiceStatus status) {
  switch (status) {
  case LanguageServiceStatus::Unavailable:
    return "unavailable";
  case LanguageServiceStatus::Idle:
    return "idle";
  case LanguageServiceStatus::Pending:
    return "pending";
  case LanguageServiceStatus::Running:
    return "running";
  case LanguageServiceStatus::Succeeded:
    return "clean";
  case LanguageServiceStatus::Failed:
    return "diagnostics";
  }
  return "unknown";
}

ImVec4 diagnosticColor(const std::string& severity) {
  if (severity == "Error")
    return ImVec4(0.95f, 0.25f, 0.20f, 1.0f);
  if (severity == "Warning")
    return ImVec4(0.95f, 0.70f, 0.20f, 1.0f);
  if (severity == "Information" || severity == "Info")
    return ImVec4(0.35f, 0.65f, 1.0f, 1.0f);
  if (severity == "Hint")
    return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
  return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

void drawLuaLSDiagnostics(const AuthoredDocEditorState& editor) {
  ImGui::SeparatorText("LuaLS diagnostics");
  ImGui::Text("LuaLS: %s", lualsStatusLabel(editor.luals.status));
  if (!editor.luals.message.empty())
    ImGui::TextDisabled("%s", editor.luals.message.c_str());

  if (editor.luals.diagnostics.empty()) {
    ImGui::TextDisabled("No LuaLS diagnostics");
    return;
  }

  for (std::size_t i = 0; i < editor.luals.diagnostics.size(); ++i) {
    const auto& diagnostic = editor.luals.diagnostics[i];
    ImGui::PushID(static_cast<int>(i));

    const ImVec4 color = diagnosticColor(diagnostic.severity);
    ImGui::TextColored(color,
                       "%s line %u:%u",
                       diagnostic.severity.c_str(),
                       diagnostic.line,
                       diagnostic.column);
    ImGui::SameLine();
    ImGui::TextUnformatted(diagnostic.message.c_str());

    ImGui::PopID();
  }
}
} // namespace

void drawEditorSelector(EditorRuntime& editor) {
  ImGui::SeparatorText("Current Editor");
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

  ImGui::BeginDisabled(editor.internalEditor);
  if (ImGui::Button("Internal")) {
    editor.internalEditor = true;
  }
  ImGui::EndDisabled();

  ImGui::SameLine();
  ImGui::BeginDisabled(!editor.internalEditor);
  if (ImGui::Button("External")) {
    editor.internalEditor = false;
  }
  ImGui::EndDisabled();

  ImGui::PopStyleVar();
}

void drawFileControls(AppContext& app) {
  EditorRuntime& editor = app.editor;

  if (!gScratch.initialized) {
    forceScratchPath(editor.authoredEditor.buffer.filePath);
    gScratch.initialized = true;
  }

  ImGui::SeparatorText("Current File");
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

  ImGui::InputText("Path", gScratch.pathBuffer, sizeof(gScratch.pathBuffer));

  if (ImGui::Button("New")) {
    newBlankDocument(editor.authoredEditor);
    forceScratchPath(editor.authoredEditor.buffer.filePath);
  }
  ImGui::SameLine();
  if (ImGui::Button("New Template")) {
    newTemplateDocument(editor.authoredEditor);
    forceScratchPath(editor.authoredEditor.buffer.filePath);
  }
  ImGui::SameLine();
  if (ImGui::Button("Open")) {
    if (loadDocument(editor.authoredEditor, gScratch.pathBuffer)) {
      forceScratchPath(editor.authoredEditor.buffer.filePath);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Save")) {
    if (!editor.authoredEditor.buffer.filePath.empty())
      saveDocument(editor.authoredEditor);
    else
      saveDocumentAs(editor.authoredEditor, gScratch.pathBuffer);
    forceScratchPath(editor.authoredEditor.buffer.filePath);
  }
  ImGui::SameLine();
  if (ImGui::Button("Save As")) {
    saveDocumentAs(editor.authoredEditor, gScratch.pathBuffer);
    forceScratchPath(editor.authoredEditor.buffer.filePath);
  }
  ImGui::SameLine();
  if (ImGui::Button("Load Demo")) {
    // Loads into the buffer without adopting the demo file as the current
    // path, so a later "Save" can't clobber the bundled example asset.
    if (loadDocument(editor.authoredEditor, app.grooveboxPaths.demoFile.c_str())) {
      editor.authoredEditor.buffer.filePath.clear();
      editor.authoredEditor.buffer.dirty = true;
      forceScratchPath(editor.authoredEditor.buffer.filePath);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Reload")) {
    if (reloadDocument(editor.authoredEditor)) {
      forceScratchPath(editor.authoredEditor.buffer.filePath);
    }
  }

  ImGui::PopStyleVar();
}

void drawEditorDisplayView(AppContext& app) {
  static bool showDiagnostics = true;
  static float diagnosticsHeight = 200.0f;

  AuthoredDocEditorState& editor = app.editor.authoredEditor;

  collectFinishedLuaLSDiagnostics(editor);
  maybeStartLuaLSDiagnostics(editor);

  ImGui::Dummy(ImVec2(0.0f, 20.0f));

  // =========== Text Input ================
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 15.0f));

  const std::size_t lineCount = countBufferLines(editor.buffer.text.data());
  const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
  const float topHeight =
      showDiagnostics
          ? (ImGui::GetContentRegionAvail().y - diagnosticsHeight - ImGui::GetStyle().ItemSpacing.y)
          : (ImGui::GetContentRegionAvail().y - 100.0f);
  const float editorHeight = std::max(lineHeight * 22.0f,
                                      lineHeight * static_cast<float>(lineCount + 1) +
                                          ImGui::GetStyle().FramePadding.y * 2.0f);
  const float gutterWidth = lineNumberGutterWidth(lineCount);

  ImGui::BeginChild("AuthoredDocumentText",
                    ImVec2(0.0f, topHeight),
                    // ImVec2(0.0f, ImGui::GetTextLineHeight() * 22.0f),
                    ImGuiChildFlags_Borders,
                    ImGuiWindowFlags_HorizontalScrollbar);
  consumeJumpRequest(editor);
  drawLineNumberGutter(lineCount, gutterWidth, editorHeight);
  ImGui::SameLine();

  std::string& text = editor.buffer.text;

  const float editorWidth =
      std::max(360.0f, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x);

  if (ImGui::InputTextMultiline("##authored-document-buffer",
                                text.data(),
                                text.capacity() + 1, // +1 for null terminator
                                ImVec2(editorWidth, editorHeight),
                                ImGuiInputTextFlags_AllowTabInput |
                                    ImGuiInputTextFlags_CallbackResize,
                                inputTextResizeCallback,
                                &text)) {
    markBufferTextChanged(editor);
  }

  ImGui::EndChild();

  if (ImGui::Button("Apply")) {
    submitEditorBuffer(editor, app);
  }
  ImGui::PopStyleVar();

  drawStatusLine(editor);

  uint8_t numErrors = 0;
  uint8_t numWarnings = 0;

  for (size_t i = 0; i < editor.luals.diagnostics.size(); i++) {
    auto severity = editor.luals.diagnostics[i].severity;
    if (severity == "Warning")
      numWarnings++;

    if (severity == "Error")
      numErrors++;
  }

  char headerBuffer[75];

  if (true) {
    // if (numWarnings || numErrors) {
    snprintf(headerBuffer,
             sizeof(headerBuffer),
             "Diagnostics (%d Errors, %d Warnings)###DiagnosticHeader",
             numErrors,
             numWarnings);
  } else {
    snprintf(headerBuffer, sizeof(headerBuffer), "Diagnostics");
  }

  if (ImGui::CollapsingHeader(headerBuffer, ImGuiTreeNodeFlags_DefaultOpen)) {
    showDiagnostics = true;
    ImGui::BeginChild("EditorDiagnostics", ImVec2(0, 300.0f), true);
    drawDiagnosticList(editor);
    drawLuaLSDiagnostics(editor);
    ImGui::EndChild();
  } else {
    showDiagnostics = false;
  }
}

} // namespace app::display

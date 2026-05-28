#include "app/editor/AuthoredDocEditor.h"

#include "app/AppContext.h"
#include "app/doc/DocAuthoringService.h"

#include <fstream>
#include <sstream>
#include <utility>

namespace app::editor {
namespace {

void setFileMessage(AuthoredDocEditorState& editor,
                    EditorCommandStatus status,
                    std::string message) {
  editor.fileMessage.status = status;
  editor.fileMessage.text = std::move(message);
}

void setApplyMessage(AuthoredDocEditorState& editor,
                     EditorCommandStatus status,
                     std::string message) {
  editor.applyMessage.status = status;
  editor.applyMessage.text = std::move(message);
}

bool readTextFile(const char* path, std::string& out) {
  std::ifstream input(path ? path : "", std::ios::binary);
  if (!input)
    return false;

  std::ostringstream buffer;
  buffer << input.rdbuf();
  out = buffer.str();
  return true;
}

bool writeTextFile(const char* path, const std::string& text) {
  std::ofstream output(path ? path : "", std::ios::binary);
  if (!output)
    return false;
  output << text;
  return static_cast<bool>(output);
}

bool validPath(const char* path) {
  return path && path[0] != '\0';
}

} // namespace

void markBufferEdited(AuthoredDocEditorState& editor, std::string text) {
  editor.buffer.text = std::move(text);
  editor.buffer.dirty = true;
  editor.editSerial++;
  editor.luals.status = LanguageServiceStatus::Pending;
}

void newBlankDocument(AuthoredDocEditorState& editor) {
  const auto previousRevision = editor.buffer.applyRevision;
  editor = AuthoredDocEditorState{};
  editor.buffer.applyRevision = previousRevision;
  editor.buffer.dirty = false;
}

void newTemplateDocument(AuthoredDocEditorState& editor) {
  const auto previousRevision = editor.buffer.applyRevision;
  editor = AuthoredDocEditorState{};
  editor.buffer.applyRevision = previousRevision;
  editor.buffer.text = authoredDocumentTemplate();
  editor.buffer.dirty = true;
}

bool loadDocument(AuthoredDocEditorState& editor, const char* path) {
  if (!validPath(path)) {
    setFileMessage(editor, EditorCommandStatus::Failed, "load failed: empty path");
    return false;
  }

  std::string text{};
  if (!readTextFile(path, text)) {
    setFileMessage(editor, EditorCommandStatus::Failed, "load failed: could not read file");
    return false;
  }

  editor.buffer.text = std::move(text);
  editor.buffer.filePath = path;
  editor.buffer.hasFilePath = true;
  editor.buffer.dirty = false;
  editor.backendDiagnostics.clear();
  setFileMessage(editor, EditorCommandStatus::Succeeded, "loaded file");
  return true;
}

bool saveDocument(AuthoredDocEditorState& editor) {
  if (!editor.buffer.hasFilePath) {
    setFileMessage(editor, EditorCommandStatus::Failed, "save failed: no file path");
    return false;
  }
  return saveDocumentAs(editor, editor.buffer.filePath.c_str());
}

bool saveDocumentAs(AuthoredDocEditorState& editor, const char* path) {
  if (!validPath(path)) {
    setFileMessage(editor, EditorCommandStatus::Failed, "save failed: empty path");
    return false;
  }

  if (!writeTextFile(path, editor.buffer.text)) {
    setFileMessage(editor, EditorCommandStatus::Failed, "save failed: could not write file");
    return false;
  }

  editor.buffer.filePath = path;
  editor.buffer.hasFilePath = true;
  editor.buffer.dirty = false;
  setFileMessage(editor, EditorCommandStatus::Succeeded, "saved file");
  return true;
}

bool reloadDocument(AuthoredDocEditorState& editor) {
  if (!editor.buffer.hasFilePath) {
    setFileMessage(editor, EditorCommandStatus::Failed, "reload failed: no file path");
    return false;
  }
  return loadDocument(editor, editor.buffer.filePath.c_str());
}

bool applyEditorBuffer(AuthoredDocEditorState& editor, app::AppContext& app) {
  const app::doc::DocRevision revision = ++editor.buffer.applyRevision;

  app::doc::ApplyRevisionResult result =
      app::doc::applyAuthoredDocRevision(app.documents.authoring,
                                         app,
                                         revision,
                                         editor.buffer.text.c_str());

  editor.backendDiagnostics = result.diagnostics;
  if (result.ok) {
    editor.applyStatus = EditorApplyStatus::Succeeded;
    editor.buffer.lastAppliedRevision = revision;
    setApplyMessage(editor, EditorCommandStatus::Succeeded, "apply completed");
    return true;
  }

  editor.applyStatus = EditorApplyStatus::Failed;
  setApplyMessage(editor, EditorCommandStatus::Failed, "apply failed");
  return false;
}

void requestDiagnosticJump(AuthoredDocEditorState& editor,
                           const app::doc::DocDiagnostic& diagnostic) {
  editor.jumpRequest.pending = true;
  editor.jumpRequest.line = diagnostic.span.line;
  editor.jumpRequest.column = diagnostic.span.column;
}

void clearDiagnosticJump(AuthoredDocEditorState& editor) {
  editor.jumpRequest = DiagnosticJumpRequest{};
}

} // namespace app::editor

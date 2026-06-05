#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/doc/metadata/DocMetadata.h"
#include "app/editor/AuthoredDocEditor.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace {

constexpr const char* kTempEditorDocPath = "/tmp/meh_editor_doc_test.lua";
constexpr const char* kMissingEditorDocPath = "/tmp/meh_editor_doc_missing.lua";

void writeTempFile(const char* path, const char* text) {
  std::ofstream out(path, std::ios::binary);
  out << text;
}

std::string readTempFile(const char* path) {
  std::ifstream in(path, std::ios::binary);
  std::string result{};
  char ch = 0;
  while (in.get(ch))
    result.push_back(ch);
  return result;
}

using test::hasDiagnostic;

} // namespace

static void test_new_blank_resets_text_without_resetting_apply_revision() {
  TEST("new_blank_resets_text_without_resetting_apply_revision");

  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.text = "track(1, TrackSettings {})";
  editor.buffer.filePath = "/tmp/example.lua";
  editor.buffer.dirty = true;
  editor.buffer.applyRevision = 7;

  app::editor::newBlankDocument(editor);

  CHECK("text empty", editor.buffer.text.empty());
  CHECK("path cleared", editor.buffer.filePath.empty());
  CHECK("dirty false", !editor.buffer.dirty);
  CHECK("revision preserved", editor.buffer.applyRevision == 7);
}

static void test_new_template_marks_dirty_and_contains_track() {
  TEST("new_template_marks_dirty_and_contains_track");

  app::editor::AuthoredDocEditorState editor{};
  app::editor::newTemplateDocument(editor);

  CHECK("dirty", editor.buffer.dirty);
  CHECK("contains track", editor.buffer.text.find("track(1") != std::string::npos);
  CHECK("contains TrackSettings", editor.buffer.text.find("TrackSettings") != std::string::npos);
}

static void test_mark_buffer_edited_sets_text_and_dirty() {
  TEST("mark_buffer_edited_sets_text_and_dirty");

  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.text = "track(1, TrackSettings {})";
  app::editor::markBufferTextChanged(editor);

  CHECK("text updated", editor.buffer.text == "track(1, TrackSettings {})");
  CHECK("dirty", editor.buffer.dirty);
}

static void test_load_document_sets_path_and_clears_dirty() {
  TEST("load_document_sets_path_and_clears_dirty");

  std::remove(kTempEditorDocPath);
  writeTempFile(kTempEditorDocPath, "track(1, TrackSettings {})\n");

  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.dirty = true;

  const bool ok = app::editor::loadDocument(editor, kTempEditorDocPath);

  CHECK("ok", ok);
  CHECK("text loaded", editor.buffer.text == "track(1, TrackSettings {})\n");
  CHECK("path set", editor.buffer.filePath == kTempEditorDocPath);
  CHECK("dirty false", !editor.buffer.dirty);
  CHECK("message succeeded",
        editor.fileMessage.status == app::editor::EditorCommandStatus::Succeeded);
}

static void test_load_missing_file_fails_without_doc_diagnostic() {
  TEST("load_missing_file_fails_without_doc_diagnostic");

  std::remove(kMissingEditorDocPath);

  app::editor::AuthoredDocEditorState editor{};
  const bool ok = app::editor::loadDocument(editor, kMissingEditorDocPath);

  CHECK("not ok", !ok);
  CHECK("file message failed",
        editor.fileMessage.status == app::editor::EditorCommandStatus::Failed);
  CHECK("backend diagnostics empty", editor.backendDiagnostics.empty());
}

static void test_save_as_writes_text_and_sets_path() {
  TEST("save_as_writes_text_and_sets_path");

  std::remove(kTempEditorDocPath);

  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.text = "track(1, TrackSettings {})\n";
  editor.buffer.dirty = true;

  const bool ok = app::editor::saveDocumentAs(editor, kTempEditorDocPath);

  CHECK("ok", ok);
  CHECK("file contents", readTempFile(kTempEditorDocPath) == editor.buffer.text);
  CHECK("path set", editor.buffer.filePath == kTempEditorDocPath);
  CHECK("dirty false", !editor.buffer.dirty);
}

static void test_reload_requires_path() {
  TEST("reload_requires_path");

  app::editor::AuthoredDocEditorState editor{};
  const bool ok = app::editor::reloadDocument(editor);

  CHECK("not ok", !ok);
  CHECK("failed message", editor.fileMessage.status == app::editor::EditorCommandStatus::Failed);
}

static void test_apply_unsaved_valid_buffer_uses_revision_and_succeeds() {
  TEST("apply_unsaved_valid_buffer_uses_revision_and_succeeds");

  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);
  app::doc::initDocAuthoringService(app->documents.authoring);
  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.text = "track(1, TrackSettings {})";
  editor.buffer.dirty = true;

  const bool ok = app::editor::submitEditorBuffer(editor, *app);

  CHECK("ok", ok);
  CHECK("apply revision 1", editor.buffer.applyRevision == 1);
  CHECK("last applied revision 1", editor.buffer.lastAppliedRevision == 1);
  CHECK("apply succeeded", editor.applyStatus == app::editor::EditorApplyStatus::Succeeded);
  CHECK("backend diagnostics empty", editor.backendDiagnostics.empty());
  CHECK("service revision 1", app->documents.authoring.buffer.currentRevision == 1);
  app::doc::destroyDocAuthoringService(app->documents.authoring);
  test::destroyAppContext(app);
}

static void test_apply_invalid_buffer_caches_backend_diagnostics() {
  TEST("apply_invalid_buffer_caches_backend_diagnostics");

  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);
  app::doc::initDocAuthoringService(app->documents.authoring);
  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.text = "track('one', TrackSettings {})";
  editor.buffer.dirty = true;

  const bool ok = app::editor::submitEditorBuffer(editor, *app);

  CHECK("not ok", !ok);
  CHECK("apply revision 1", editor.buffer.applyRevision == 1);
  CHECK("last applied stays 0", editor.buffer.lastAppliedRevision == 0);
  CHECK("apply failed", editor.applyStatus == app::editor::EditorApplyStatus::Failed);
  CHECK("diagnostics cached", !editor.backendDiagnostics.empty());
  CHECK("invalid index diagnostic",
        hasDiagnostic(editor.backendDiagnostics, app::doc::docdiag::SequencerTrackInvalidIndex));
  CHECK("dirty preserved", editor.buffer.dirty);
  app::doc::destroyDocAuthoringService(app->documents.authoring);
  test::destroyAppContext(app);
}

static void test_apply_attempt_revision_increments_on_failure_and_success() {
  TEST("apply_attempt_revision_increments_on_failure_and_success");

  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);
  app::doc::initDocAuthoringService(app->documents.authoring);
  app::editor::AuthoredDocEditorState editor{};

  editor.buffer.text = "track('one', TrackSettings {})";
  CHECK("first apply fails", !app::editor::submitEditorBuffer(editor, *app));

  editor.buffer.text = "track(1, TrackSettings {})";
  CHECK("second apply succeeds", app::editor::submitEditorBuffer(editor, *app));

  CHECK("apply revision 2", editor.buffer.applyRevision == 2);
  CHECK("last applied revision 2", editor.buffer.lastAppliedRevision == 2);
  app::doc::destroyDocAuthoringService(app->documents.authoring);
  test::destroyAppContext(app);
}

static void test_request_diagnostic_jump_uses_source_span() {
  TEST("request_diagnostic_jump_uses_source_span");

  app::editor::AuthoredDocEditorState editor{};
  app::doc::DocDiagnostic diagnostic{};
  diagnostic.span.line = 12;
  diagnostic.span.column = 3;

  app::editor::requestDiagnosticJump(editor, diagnostic);

  CHECK("pending", editor.jumpRequest.pending);
  CHECK("line", editor.jumpRequest.line == 12);
  CHECK("column", editor.jumpRequest.column == 3);

  app::editor::clearDiagnosticJump(editor);
  CHECK("cleared", !editor.jumpRequest.pending);
}

static void test_save_requires_existing_path() {
  TEST("save_requires_existing_path");
  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.text = "track(1, TrackSettings {})\n";
  editor.buffer.dirty = true;
  const bool ok = app::editor::saveDocument(editor);
  CHECK("not ok", !ok);
  CHECK("failed message", editor.fileMessage.status == app::editor::EditorCommandStatus::Failed);
  CHECK("dirty preserved", editor.buffer.dirty);
  CHECK("no path", editor.buffer.filePath.empty());
}
static void test_save_writes_existing_path() {
  TEST("save_writes_existing_path");
  std::remove(kTempEditorDocPath);
  app::editor::AuthoredDocEditorState editor{};
  editor.buffer.text = "track(1, TrackSettings {})\n";
  editor.buffer.filePath = kTempEditorDocPath;
  editor.buffer.dirty = true;
  const bool ok = app::editor::saveDocument(editor);
  CHECK("ok", ok);
  CHECK("file contents", readTempFile(kTempEditorDocPath) == editor.buffer.text);
  CHECK("dirty false", !editor.buffer.dirty);
}

void runAuthoredDocEditorTests() {
  SUITE("AuthoredDocEditor");
  test_new_blank_resets_text_without_resetting_apply_revision();
  test_new_template_marks_dirty_and_contains_track();
  test_mark_buffer_edited_sets_text_and_dirty();
  test_load_document_sets_path_and_clears_dirty();
  test_load_missing_file_fails_without_doc_diagnostic();
  test_save_as_writes_text_and_sets_path();
  test_reload_requires_path();
  test_apply_unsaved_valid_buffer_uses_revision_and_succeeds();
  test_apply_invalid_buffer_caches_backend_diagnostics();
  test_apply_attempt_revision_increments_on_failure_and_success();
  test_request_diagnostic_jump_uses_source_span();
  test_save_requires_existing_path();
  test_save_writes_existing_path();
}

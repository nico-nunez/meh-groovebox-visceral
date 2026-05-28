#pragma once

#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocTypes.h"

#include <cstdint>
#include <string>

namespace app {
struct AppContext;
}

namespace app::editor {

enum class LanguageServiceStatus : uint8_t {
  Unavailable,
  Idle,
  Pending,
  Running,
  Succeeded,
  Failed,
};

struct LuaLSDiagnostic {
  std::string severity{};
  std::string code{};
  std::string message{};
  uint32_t line = 0;
  uint32_t column = 0;
};

struct LuaLSDiagnosticState {
  LanguageServiceStatus status = LanguageServiceStatus::Idle;
  std::string message{};
  std::vector<LuaLSDiagnostic> diagnostics{};
  uint64_t requestedSerial = 0;
  uint64_t completedSerial = 0;
  bool running = false;
};

enum class EditorCommandStatus : uint8_t {
  Idle,
  Succeeded,
  Failed,
};

enum class EditorApplyStatus : uint8_t {
  Idle,
  Succeeded,
  Failed,
};

struct EditorMessage {
  EditorCommandStatus status = EditorCommandStatus::Idle;
  std::string text{};
};

// UI (imgui)
struct AuthoredDocBuffer {
  std::string text{};
  std::string filePath{};
  app::doc::DocRevision applyRevision = 0;
  app::doc::DocRevision lastAppliedRevision = 0;
  bool hasFilePath = false;
  bool dirty = false;
};

struct DiagnosticJumpRequest {
  bool pending = false;
  uint32_t line = 0;
  uint32_t column = 0;
};

struct AuthoredDocEditorState {
  AuthoredDocBuffer buffer{};
  app::doc::DocDiagnostics backendDiagnostics{};
  EditorMessage fileMessage{};
  EditorMessage applyMessage{};
  EditorApplyStatus applyStatus = EditorApplyStatus::Idle;
  DiagnosticJumpRequest jumpRequest{};
  LuaLSDiagnosticState luals{};
  uint64_t editSerial = 0;
};

inline constexpr const char* authoredDocumentTemplate() {
  return "track(1, TrackSettings {\n"
         "  patterns = {\n"
         "    [1] = {\n"
         "      numSteps = 1,\n"
         "      stepsPerBeat = 4,\n"
         "      steps = {\n"
         "        { active = true, note = 60, velocity = 100, gate = 0.8 }\n"
         "      }\n"
         "    }\n"
         "  },\n"
         "  activeSlot = 1\n"
         "})\n";
}

void markBufferTextChanged(AuthoredDocEditorState& editor);

void newBlankDocument(AuthoredDocEditorState& editor);
void newTemplateDocument(AuthoredDocEditorState& editor);

bool loadDocument(AuthoredDocEditorState& editor, const char* path);
bool saveDocument(AuthoredDocEditorState& editor);
bool saveDocumentAs(AuthoredDocEditorState& editor, const char* path);
bool reloadDocument(AuthoredDocEditorState& editor);

bool applyEditorBuffer(AuthoredDocEditorState& editor, app::AppContext& app);

void requestDiagnosticJump(AuthoredDocEditorState& editor,
                           const app::doc::DocDiagnostic& diagnostic);
void clearDiagnosticJump(AuthoredDocEditorState& editor);

} // namespace app::editor

#include "app/FileWatchApply.h"

#include "app/AppContext.h"
#include "app/doc/DocAuthoringService.h"

#include <cstdint>
#include <string>

namespace app {

void onSessionFileChanged(void* context) {
  AppContext* app = static_cast<AppContext*>(context);
  if (!app)
    return;

  if (app->editor.internalEditor)
    return;

  app->documents.externalFileApply.changeSerial.fetch_add(1, std::memory_order_release);
}

void pollExternalSessionFileApply(AppContext* app) {
  if (!app)
    return;

  if (app->editor.internalEditor)
    return;

  ExternalFileApplyState& state = app->documents.externalFileApply;
  const uint64_t serial = state.changeSerial.load(std::memory_order_acquire);
  if (serial == state.appliedSerial)
    return;

  state.appliedSerial = serial;

  const std::string pathString = app->grooveboxPaths.sessionFile.string();
  doc::submitAuthoredDocFile(app->documents.authoring, *app, pathString.c_str());
}

} // namespace app

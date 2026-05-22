#include "app/FileWatchApply.h"

#include "app/AppContext.h"
#include "app/doc/DocAuthoringService.h"

namespace app {

void onSessionFileChanged(void* context) {
  AppContext* app = static_cast<AppContext*>(context);
  if (app->editor.internalEditor)
    return;

  const std::string pathString = app->grooveboxPaths.sessionFile.string();

  const doc::ApplyRevisionResult result =
      doc::applySequencerFile(app->documents.authoring, *app, pathString.c_str());
}

} // namespace app

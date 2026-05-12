#include "file_watch/FileWatch.h"
#include "adapters/fsevents/FSEventsAdapter.h"
#include "shared/FileWatcher.h"

// TODO: platform selection (inotify for Linux)

namespace file_watch {

hFileWatcher* createWatcher(std::string_view path, FileChangeCallback callback, void* context) {
  if (path.empty())
    return nullptr;

  auto* session = new hFileWatcher();
  session->watchedPath = std::string(path);
  session->callback = callback;
  session->userContext = context;

  const int err = FSEventsAdapter::fsEventsSetup(session);
  if (err) {
    printf("file_watch: FSEvents setup failed: %d\n", err);
    delete session;
    return nullptr;
  }

  return session;
}

void destroyWatcher(hFileWatcher* watcher) {
  if (!watcher)
    return;

  FSEventsAdapter::fsEventsCleanup(watcher);
  delete watcher;
}

} // namespace file_watch

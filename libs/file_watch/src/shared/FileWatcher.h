#pragma once

#include "file_watch/FileWatch.h"

#include <string>

namespace file_watch {

struct hFileWatcher {
  std::string watchedPath{};
  FileChangeCallback callback = nullptr;
  void* userContext = nullptr;
  void* platformContext = nullptr; // FSEventsAdapter::FSEventsContext*
};

} // namespace file_watch

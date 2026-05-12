#pragma once

#include <string_view>

namespace file_watch {

using FileChangeCallback = void (*)(void* context);

struct hFileWatcher;

// Creates a watcher on `path`. `callback` fires on a background thread
// (the FSEvents runloop thread) each time the file is modified.
// Returns nullptr if the path is empty or the stream cannot be created.
hFileWatcher* createWatcher(std::string_view path, FileChangeCallback callback, void* context);

// Stops the watcher, joins the background thread, and frees all resources.
// Safe to call with nullptr.
void destroyWatcher(hFileWatcher* watcher);

} // namespace file_watch

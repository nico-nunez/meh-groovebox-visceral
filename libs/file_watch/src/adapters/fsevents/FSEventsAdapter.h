#pragma once

#include "shared/FileWatcher.h"

namespace FSEventsAdapter {

// Allocates and starts the FSEvents stream + runloop thread.
// Stores FSEventsContext* in session->platformContext.
// Returns 0 on success, non-zero on failure.
int fsEventsSetup(file_watch::hFileWatcher* session);

// Stops the runloop, invalidates the stream, joins the thread,
// and frees FSEventsContext.
int fsEventsCleanup(file_watch::hFileWatcher* session);

} // namespace FSEventsAdapter

#include "FSEventsAdapter.h"
#include "shared/FileWatcher.h"

#include <CoreServices/CoreServices.h>

#include <cstdio>
#include <dispatch/dispatch.h>
#include <string>

namespace FSEventsAdapter {
namespace {

struct FSEventsContext {
  dispatch_queue_t queue = nullptr;
  FSEventStreamRef stream = nullptr;
};

void streamCallback(ConstFSEventStreamRef /*stream*/,
                    void* clientCallBackInfo,
                    size_t numEvents,
                    void* /*eventPaths*/,
                    const FSEventStreamEventFlags* /*eventFlags*/,
                    const FSEventStreamEventId* /*eventIds*/) {
  if (numEvents == 0)
    return;

  auto* session = static_cast<file_watch::hFileWatcher*>(clientCallBackInfo);
  if (session->callback)
    session->callback(session->userContext);
}

} // namespace

int fsEventsSetup(file_watch::hFileWatcher* session) {
  printf("file_watch: entering FSEvents setup\n");
  auto* ctx = new FSEventsContext();

  // Watch the parent directory; filter to the specific file in the callback.
  // FSEvents operates on directories, not individual files.
  CFStringRef pathStr = CFStringCreateWithCString(kCFAllocatorDefault,
                                                  session->watchedPath.c_str(),
                                                  kCFStringEncodingUTF8);

  if (!pathStr) {
    delete ctx;
    return -1;
  }

  CFArrayRef pathsToWatch = CFArrayCreate(kCFAllocatorDefault,
                                          reinterpret_cast<const void**>(&pathStr),
                                          1,
                                          &kCFTypeArrayCallBacks);
  CFRelease(pathStr);

  FSEventStreamContext streamCtx{};
  streamCtx.info = session;

  // 50ms latency — coalesces rapid saves before firing callback.
  constexpr CFAbsoluteTime kLatencySeconds = 0.05;

  FSEventStreamRef stream =
      FSEventStreamCreate(kCFAllocatorDefault,
                          &streamCallback,
                          &streamCtx,
                          pathsToWatch,
                          kFSEventStreamEventIdSinceNow,
                          kLatencySeconds,
                          kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer);

  CFRelease(pathsToWatch);

  if (!stream) {
    delete ctx;
    return -1;
  }

  ctx->stream = stream;

  ctx->queue = dispatch_queue_create("com.groovebox.filewatcher", DISPATCH_QUEUE_SERIAL);
  FSEventStreamSetDispatchQueue(ctx->stream, ctx->queue);
  FSEventStreamStart(ctx->stream);

  session->platformContext = ctx;

  return 0;
}

int fsEventsCleanup(file_watch::hFileWatcher* session) {
  if (!session->platformContext)
    return 0;

  auto* ctx = static_cast<FSEventsContext*>(session->platformContext);

  FSEventStreamStop(ctx->stream);
  FSEventStreamInvalidate(ctx->stream);
  FSEventStreamRelease(ctx->stream);
  dispatch_release(ctx->queue);

  delete ctx;
  session->platformContext = nullptr;
  return 0;
}

} // namespace FSEventsAdapter

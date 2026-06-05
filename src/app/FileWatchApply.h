#pragma once

#include <atomic>

namespace app {

struct AppContext;

struct ExternalFileApplyState {
  std::atomic<uint64_t> changeSerial{0};
  uint64_t appliedSerial = 0;
};

// file_watch::FileChangeCallback-compatible callback.
// Context must be AppContext*. This function must remain tiny: it runs on the
// FSEvents dispatch worker and only publishes a cross-thread notification.
void onSessionFileChanged(void* context);

// Main/app-thread drain point for external editor file changes.
void pollExternalSessionFileApply(AppContext* app);

} // namespace app

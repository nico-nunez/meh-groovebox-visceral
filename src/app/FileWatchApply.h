#pragma once

namespace app {

// file_watch::FileChangeCallback-compatible callback.
// context must be AppContext*.
void onSessionFileChanged(void* context);

} // namespace app

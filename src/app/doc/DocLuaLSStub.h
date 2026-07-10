#pragma once

#include <string>

namespace app::doc {

// Renders the LuaLS type-annotation stub for the authored-document Lua
// surface (track()/TrackSettings/synth()/mixer()/etc.) directly from the
// compiled-in C++ metadata descriptors. Used both by the offline
// tools/luals/generate_luals_stubs tool (git-committed reference copy under
// generated/luals/, checked via `make check-luals-stubs`) and by the running
// app itself, which materializes the same content into the user's config
// directory so external-editor autocomplete and in-app LuaLS diagnostics
// work without depending on the repo being present on disk.
std::string renderAuthoredDocumentLuaLSStub();

} // namespace app::doc

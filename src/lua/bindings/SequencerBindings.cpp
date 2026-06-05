#include "lua/bindings/LuaBindings.h"
#include "lua/metadata/LuaRuntimeMetadata.h"

#include "app/Sequencer.h"
#include "app/Types.h"

#include <cassert>
#include <cstdint>

namespace lua::bindings {

namespace {
namespace evt = app::events;
namespace seq = app::sequencer;

// =====================
// Sequencer methods
// =====================

int l_seqListTracks(lua_State* L) {
  auto* ctx = getLuaContext(L);
  uint8_t cur = ctx->currentTrack; // use Lua shadow for * marker

  printf("trk  gain   pan    mute\n");
  for (int i = 0; i < (int)seq::MAX_LANES; ++i) {
    const auto& t = ctx->app->mixer.current.tracks[i];
    printf("  %d%c %.2f  %+.2f   %s\n",
           i + 1,
           (i == (int)cur) ? '*' : ' ',
           t.gain,
           t.pan,
           t.enabled ? "off" : "MUTE");
  }
  return CMD_SUCCESS;
}

int l_seqSelectTrack(lua_State* L) {
  int track = (int)luaL_checkinteger(L, 1);
  auto* ctx = getLuaContext(L);

  if (track < 1 || track > (int)app::MAX_TRACKS)
    return luaL_error(L, "track %d out of range (1–%d)", track, (int)app::MAX_TRACKS);

  uint8_t idx = (uint8_t)(track - 1);

  // Update Lua-side shadow immediately so subsequent Lua commands target
  // the new track without waiting for the audio callback to drain the queue.
  ctx->currentTrack = idx;

  auto evt = evt::createCurrentTrackEvent(idx);
  if (!pushControlEvent(ctx->app, evt).ok)
    return luaL_error(L, "control queue full");

  // Display reads mixer state directly — one block behind is fine for a print.
  const auto& t = ctx->app->mixer.current.tracks[idx];
  printf("[track %d]  gain: %.2f  pan: %+.2f  mute: %s\n",
         track,
         t.gain,
         t.pan,
         t.enabled ? "off" : "MUTE");

  return CMD_SUCCESS;
}

} // namespace

// =========================
// Registration
// =========================
void registerSeqCommands(lua_State* L) {

  lua_newtable(L);

  registerTableFunction(L, l_seqListTracks, rtmethod::ListTracks);
  registerTableFunction(L, l_seqSelectTrack, rtmethod::SelectTrack);

  lua_setglobal(L, rtglobal::Seq);
  addVisibleGlobal(rtglobal::Seq);
}

} // namespace lua::bindings

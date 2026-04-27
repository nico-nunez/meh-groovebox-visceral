#include "MIDIBindings.h"

#include "lua/bindings/LuaBindings.h"

namespace lua::bindings {

namespace {
namespace evt = app::events;

int l_midiSticky(lua_State* L) {
  int track = static_cast<int>(luaL_checkinteger(L, 1));

  if (track < 1 || track > static_cast<int>(app::MAX_TRACKS))
    return luaL_error(L, "track must be 1-%d", app::MAX_TRACKS);

  auto* ctx = getLuaContext(L);
  auto evt = evt::createMidiStickyTrackEvent(static_cast<uint8_t>(track - 1));

  CMD_CHECK(app::pushControlEvent(ctx->app, evt));
}

int l_midiUnsticky(lua_State* L) {
  auto* ctx = getLuaContext(L);
  auto evt = app::events::createMidiUnstickyEvent();

  CMD_CHECK(app::pushControlEvent(ctx->app, evt));
}

int l_midiChannel(lua_State* L) {
  int channel = static_cast<int>(luaL_checkinteger(L, 1));
  int track = static_cast<int>(luaL_checkinteger(L, 2));

  if (channel < 1 || channel > static_cast<int>(app::MAX_MIDI_CHANNELS))
    return luaL_error(L, "channel must be 1-%d", app::MAX_MIDI_CHANNELS);
  if (track < 1 || track > static_cast<int>(app::MAX_TRACKS))
    return luaL_error(L, "track must be 1-%d", app::MAX_TRACKS);

  auto* ctx = getLuaContext(L);
  auto evt = evt::createMidiChannelTrackEvent(static_cast<uint8_t>(channel - 1),
                                              static_cast<uint8_t>(track - 1));
  CMD_CHECK(app::pushControlEvent(ctx->app, evt));
}

int l_midiUnchannel(lua_State* L) {
  int channel = static_cast<int>(luaL_checkinteger(L, 1));

  if (channel < 1 || channel > static_cast<int>(app::MAX_MIDI_CHANNELS))
    return luaL_error(L, "channel must be 1-%d", app::MAX_MIDI_CHANNELS);

  auto* ctx = getLuaContext(L);
  auto evt = evt::createMidiUnchannelEvent(static_cast<uint8_t>(channel - 1));

  CMD_CHECK(app::pushControlEvent(ctx->app, evt));
}

int l_midiRoutes(lua_State* L) {
  auto* ctx = getLuaContext(L);

  if (ctx->app->midiStickyTrack == app::MIDI_CHANNEL_UNASSIGNED)
    printf("sticky: off (follows selected track)\n");
  else
    printf("sticky: track %d\n", ctx->app->midiStickyTrack + 1);

  printf("channel routes:\n");
  for (uint8_t ch = 0; ch < app::MAX_MIDI_CHANNELS; ++ch) {
    uint8_t track = ctx->app->midiChannelMap[ch];
    if (track == app::MIDI_CHANNEL_UNASSIGNED)
      printf("  %2d -> selected track\n", ch + 1);
    else
      printf("  %2d -> track %d\n", ch + 1, track + 1);
  }

  return CMD_SUCCESS;
}

} // namespace

void registerMIDICommands(lua_State* L) {
  lua_newtable(L);

  registerFunction(L, l_midiSticky, "sticky");
  registerFunction(L, l_midiUnsticky, "unsticky");
  registerFunction(L, l_midiChannel, "channel");
  registerFunction(L, l_midiUnchannel, "unchannel");
  registerFunction(L, l_midiRoutes, "routes");

  lua_setglobal(L, "midi");
  addVisibleGlobal("midi");
}

} // namespace lua::bindings

#include "ControlEvents.h"

#include "app/AppContext.h"
#include "app/Constants.h"
#include "app/Transport.h"

namespace app::events {

// ====================
// Event Factories
// ====================

ControlEvent createBPMEvent(float bpm) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::SetBPM;
  evt.data.setBPM.bpm = bpm;

  return evt;
}

ControlEvent createCurrentTrackEvent(uint8_t trackIndex) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::SetCurrentTrack;
  evt.data.setCurrentTrack.track = trackIndex;
  return evt;
}

ControlEvent createAppParamEvent(AppParamID id, float value, uint8_t track) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::SetAppParam;
  evt.data.setAppParam.id = id;
  evt.data.setAppParam.track = track;
  evt.data.setAppParam.value = value;
  return evt;
}

ControlEvent createMidiStickyTrackEvent(uint8_t track) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::SetMidiStickyTrack;
  evt.data.setMidiStickyTrack.track = track;
  return evt;
}

ControlEvent createMidiUnstickyEvent() {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::ClearMidiStickyTrack;
  return evt;
}

ControlEvent createMidiChannelTrackEvent(uint8_t channel, uint8_t track) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::SetMidiChannelTrack;
  evt.data.setMidiChannelTrack.channel = channel;
  evt.data.setMidiChannelTrack.track = track;
  return evt;
}

ControlEvent createMidiUnchannelEvent(uint8_t channel) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::ClearMidiChannelTrack;
  evt.data.clearMidiChannelTrack.channel = channel;
  return evt;
}

void applyControlEvent(AppContext* ctx, const ControlEvent& evt) {
  using EvtType = ControlEvent::Type;

  switch (evt.type) {
  case EvtType::SetBPM: { // TODO - should sub-events exist?
    transport::TransportEvent t{};
    t.type = transport::TransportEvent::Type::SetBPM;
    t.data.setBPM.bpm = evt.data.setBPM.bpm;
    transport::applyTransportEvent(ctx->transport, t);
    break;
  }
  case EvtType::Play: {
    transport::TransportEvent t{};
    t.type = transport::TransportEvent::Type::Play;
    transport::applyTransportEvent(ctx->transport, t);
    break;
  }
  case EvtType::Stop: {
    transport::TransportEvent t{};
    t.type = transport::TransportEvent::Type::Stop;
    transport::applyTransportEvent(ctx->transport, t);
    break;
  }
  case EvtType::SetCurrentTrack: {
    ctx->currentTrack = evt.data.setCurrentTrack.track;
    break;
  }

  case EvtType::SetAppParam: {
    params::applyAppParam(ctx,
                          evt.data.setAppParam.id,
                          evt.data.setAppParam.value,
                          evt.data.setAppParam.track);
    break;
  }
  case EvtType::SetMidiStickyTrack: {
    ctx->midiStickyTrack = evt.data.setMidiStickyTrack.track;
    break;
  }
  case EvtType::ClearMidiStickyTrack: {
    ctx->midiStickyTrack = MIDI_CHANNEL_UNASSIGNED;
    break;
  }
  case EvtType::SetMidiChannelTrack: {
    if (evt.data.setMidiChannelTrack.channel < MAX_MIDI_CHANNELS)
      ctx->midiChannelMap[evt.data.setMidiChannelTrack.channel] =
          evt.data.setMidiChannelTrack.track;
    break;
  }
  case EvtType::ClearMidiChannelTrack: {
    if (evt.data.clearMidiChannelTrack.channel < MAX_MIDI_CHANNELS)
      ctx->midiChannelMap[evt.data.clearMidiChannelTrack.channel] = MIDI_CHANNEL_UNASSIGNED;
    break;
  }
  }
}

} // namespace app::events

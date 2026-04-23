#include "ControlEvents.h"

#include "app/AppContext.h"
#include "app/Transport.h"

namespace app::events {

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
  case EvtType::SetCurrentTrack:
    ctx->currentTrack = evt.data.setCurrentTrack.track;
    break;

  case EvtType::SetAppParam:
    params::applyAppParam(ctx,
                          evt.data.setAppParam.id,
                          evt.data.setAppParam.value,
                          evt.data.setAppParam.track);
    break;
  }
}

} // namespace app::events

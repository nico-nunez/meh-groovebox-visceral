#include "app/Transport.h"

#include <algorithm>

namespace app::transport {

namespace {
constexpr float MIN_BPM = 20.0f;
constexpr float MAX_BPM = 300.0f;
} // namespace

float clampBPM(float bpm) {
  return std::clamp(bpm, MIN_BPM, MAX_BPM);
}

void initTransportContext(TransportContext& ctx, float bpm) {
  ctx.clock.bpm = clampBPM(bpm);
  ctx.transport.mode = TransportMode::Stopped;
}

void applyTransportAction(TransportContext& ctx, const TransportAction& action) {
  switch (action.type) {
  case TransportAction::Type::SetBPM:
    ctx.clock.bpm = clampBPM(action.data.setBPM.bpm);
    return;
  case TransportAction::Type::Play:
    ctx.transport.mode = TransportMode::Playing;
    return;
  case TransportAction::Type::Stop:
    ctx.transport.mode = TransportMode::Stopped;
    return;
  }
}

} // namespace app::transport

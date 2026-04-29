#include "app/Transport.h"

#include <algorithm>

namespace app::transport {
namespace {
constexpr double BEATS_PER_BAR = 4.0;

double calcBeatsPerSample(float bpm, uint32_t sampleRate) {
  return static_cast<double>(bpm) / (60.0 * static_cast<double>(sampleRate));
}

double deriveBeatPosition(const TransportState& rt, uint64_t samplePos) {
  double beatsPerSample = calcBeatsPerSample(rt.bpm, rt.sampleRate);

  return rt.segmentStartBeat +
         static_cast<double>(samplePos - rt.segmentStartSample) * beatsPerSample;
}

void resetTransportPosition(TransportState& rt) {
  rt.samplePosition = 0;
  rt.segmentStartSample = 0;
  rt.beatPosition = 0.0;
  rt.segmentStartBeat = 0.0;
}

} // namespace

float clampBPM(float bpm) {
  return std::clamp(bpm, MIN_BPM, MAX_BPM);
}

void initTransport(TransportState& rt, uint32_t sampleRate, float bpm) {
  rt.sampleRate = sampleRate;
  rt.bpm = bpm;
  rt.mode = TransportMode::Stopped;

  rt.samplePosition = 0;
  rt.segmentStartSample = 0;

  rt.beatPosition = 0.0;
  rt.segmentStartBeat = 0.0;
}

bool isTransportPlaying(TransportMode mode) {
  return mode == TransportMode::Playing;
}

void applyTransportEvent(TransportState& rt, const TransportEvent& event) {
  switch (event.type) {
  case TransportEvent::Type::SetBPM: {
    // Move beat forward at prior bpm (becomes new tempo's start)
    rt.segmentStartBeat = deriveBeatPosition(rt, rt.samplePosition);
    rt.segmentStartSample = rt.samplePosition;

    rt.bpm = clampBPM(event.data.setBPM.bpm);

    rt.beatPosition = rt.segmentStartBeat;
    return;
  }

  case TransportEvent::Type::Play:
    rt.mode = TransportMode::Playing;
    return;

  case TransportEvent::Type::Pause:
    if (rt.mode == TransportMode::Playing)
      rt.mode = TransportMode::Paused;
    return;

  case TransportEvent::Type::Stop:
    rt.mode = TransportMode::Stopped;
    resetTransportPosition(rt);
    return;
  }
}

TransportBlockInfo advanceTransportBlock(TransportState& state,
                                         TransportMode previousMode,
                                         uint32_t numFrames) {
  TransportBlockInfo block{};
  block.sampleRate = state.sampleRate;
  block.numFrames = numFrames;
  block.bpm = state.bpm;
  block.mode = state.mode;

  const bool wasPlaying = isTransportPlaying(previousMode);
  const bool isPlaying = isTransportPlaying(state.mode);

  block.startedThisBlock = (isPlaying && !wasPlaying);
  block.stoppedThisBlock = (wasPlaying && !isPlaying);

  block.startSample = state.samplePosition;
  block.startBeat = deriveBeatPosition(state, block.startSample);

  if (isPlaying)
    state.samplePosition += numFrames;

  block.endSample = state.samplePosition;
  block.endBeat = deriveBeatPosition(state, block.endSample);
  block.advancedThisBlock = (isPlaying && numFrames > 0);
  state.beatPosition = block.endBeat;

  bool crossedBeat = static_cast<int64_t>(std::floor(block.startBeat)) !=
                     static_cast<int64_t>(std::floor(block.endBeat));
  bool crossedBar = static_cast<int64_t>(std::floor(block.startBeat / BEATS_PER_BAR)) !=
                    static_cast<int64_t>(std::floor(block.endBeat / BEATS_PER_BAR));

  if (crossedBeat)
    block.boundaryFlags |= static_cast<uint32_t>(BoundaryFlags::CrossedBeat);
  if (crossedBar)
    block.boundaryFlags |= static_cast<uint32_t>(BoundaryFlags::CrossedBar);

  return block;
}

} // namespace app::transport

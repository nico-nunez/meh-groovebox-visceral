#include "app/Transport.h"

#include <algorithm>

namespace app::transport {
namespace {
constexpr double BEATS_PER_BAR = 4.0;

double calcBeatsPerSample(float bpm, uint32_t sampleRate) {
  return static_cast<double>(bpm) / (60.0 * static_cast<double>(sampleRate));
}

double deriveBeatPosition(const TransportRuntime& rt, uint64_t samplePos) {
  double beatsPerSample = calcBeatsPerSample(rt.bpm, rt.sampleRate);

  return rt.segmentStartBeat +
         static_cast<double>(samplePos - rt.segmentStartSample) * beatsPerSample;
}

} // namespace

float clampBPM(float bpm) {
  return std::clamp(bpm, MIN_BPM, MAX_BPM);
}

void initTransportRuntime(TransportRuntime& rt, uint32_t sampleRate, float bpm) {
  rt.sampleRate = sampleRate;

  rt.bpm = bpm;
  rt.mode = TransportMode::Stopped;

  rt.samplePosition = 0;
  rt.segmentStartSample = 0;

  rt.beatPosition = 0.0;
  rt.segmentStartBeat = 0.0;
}

void applyTransportEvent(TransportRuntime& rt, const TransportEvent& event) {
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
  case TransportEvent::Type::Stop:
    rt.mode = TransportMode::Stopped;
    return;
  }
}

BlockTimeResult advanceTransportBlock(TransportRuntime& rt,
                                      TransportMode previousMode,
                                      uint32_t numFrames) {
  BlockTimeResult result{};
  result.sampleRate = rt.sampleRate;
  result.bpm = rt.bpm;
  result.mode = rt.mode;

  result.startedThisBlock =
      (previousMode == TransportMode::Stopped && rt.mode == TransportMode::Playing);
  result.stoppedThisBlock =
      (previousMode == TransportMode::Playing && rt.mode == TransportMode::Stopped);

  result.startSample = rt.samplePosition;
  result.startBeat = deriveBeatPosition(rt, result.startSample);

  if (rt.mode == TransportMode::Playing)
    rt.samplePosition += numFrames;

  result.endSample = rt.samplePosition;
  result.endBeat = deriveBeatPosition(rt, result.endSample);
  result.advancedThisBlock = (rt.mode == TransportMode::Playing && numFrames > 0);
  rt.beatPosition = result.endBeat;

  bool crossedBeat = static_cast<int64_t>(std::floor(result.startBeat)) !=
                     static_cast<int64_t>(std::floor(result.endBeat));
  bool crossedBar = static_cast<int64_t>(std::floor(result.startBeat / BEATS_PER_BAR)) !=
                    static_cast<int64_t>(std::floor(result.endBeat / BEATS_PER_BAR));

  if (crossedBeat)
    result.boundaryFlags |= static_cast<uint32_t>(BoundaryFlags::CrossedBeat);
  if (crossedBar)
    result.boundaryFlags |= static_cast<uint32_t>(BoundaryFlags::CrossedBar);

  return result;
}

} // namespace app::transport

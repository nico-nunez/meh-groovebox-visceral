#pragma once

#include "app/AppParams.h"
#include <atomic>
#include <cstdint>

namespace app {
struct AppContext;
}

namespace app::events {
using params::AppParamID;

struct ControlEvent {
  enum class Type : uint8_t {
    SetBPM,
    Play,
    Stop,
    SetCurrentTrack,
    SetAppParam,
  } type{};

  union {

    struct {
      float bpm;
    } setBPM;

    struct {
      uint8_t track;
    } setCurrentTrack;

    struct {
      AppParamID id;
      uint8_t track;
      float value;
    } setAppParam;

  } data{};
};

struct ControlEventQueue {
  static constexpr uint8_t SIZE = 128;
  static constexpr uint8_t MASK = SIZE - 1;

  ControlEvent queue[SIZE]{};
  std::atomic<uint8_t> writeIndex{0};
  std::atomic<uint8_t> readIndex{0};

  bool push(const ControlEvent& evt) {
    uint8_t cur = writeIndex.load();
    uint8_t next = (cur + 1) & MASK;
    if (next == readIndex.load())
      return false;
    queue[cur] = evt;
    writeIndex.store(next);
    return true;
  }

  bool pop(ControlEvent& evt) {
    uint8_t cur = readIndex.load();
    if (cur == writeIndex.load())
      return false;
    evt = queue[cur];
    readIndex.store((cur + 1) & MASK);
    return true;
  }
};

// ====================
// Event Factories
// ====================

inline ControlEvent createBPMEvent(float bpm) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::SetBPM;
  evt.data.setBPM.bpm = bpm;

  return evt;
}

inline ControlEvent createCurrentTrackEvent(uint8_t trackIndex) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::SetCurrentTrack;
  evt.data.setCurrentTrack.track = trackIndex;
  return evt;
}

inline ControlEvent createAppParamEvent(AppParamID id, float value, uint8_t track = 0) {
  ControlEvent evt{};
  evt.type = ControlEvent::Type::SetAppParam;
  evt.data.setAppParam.id = id;
  evt.data.setAppParam.track = track;
  evt.data.setAppParam.value = value;
  return evt;
}

void applyControlEvent(AppContext* ctx, const ControlEvent& evt);

} // namespace app::events

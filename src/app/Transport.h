#pragma once

#include <atomic>
#include <cstdint>

namespace app::transport {

enum class TransportMode : uint8_t {
  Stopped = 0,
  Playing = 1,
};

struct ClockState {
  float bpm = 120.0f;
};

struct TransportState {
  TransportMode mode = TransportMode::Stopped;
};

struct TransportContext {
  ClockState clock{};
  TransportState transport{};
};

struct TransportAction {
  enum class Type : uint8_t {
    SetBPM = 0,
    Play = 1,
    Stop = 2,
  };

  Type type = Type::SetBPM;

  union {
    struct {
      float bpm;
    } setBPM;
  } data{};
};

struct TransportActionQueue {
  static constexpr uint32_t SIZE = 128;
  static constexpr uint32_t MASK = SIZE - 1;

  // SPSC queue: one non-RT producer, one audio-thread consumer.
  TransportAction queue[SIZE]{};
  std::atomic<uint32_t> writeIndex{0};
  std::atomic<uint32_t> readIndex{0};

  bool push(const TransportAction& action) {
    uint32_t currentIndex = writeIndex.load();
    uint32_t nextIndex = (currentIndex + 1) & MASK;

    if (nextIndex == readIndex.load())
      return false;

    queue[currentIndex] = action;
    writeIndex.store(nextIndex);

    return true;
  }

  bool pop(TransportAction& action) {
    uint32_t currentIndex = readIndex.load();

    if (currentIndex == writeIndex.load())
      return false;

    action = queue[currentIndex];
    readIndex.store((currentIndex + 1) & MASK);

    return true;
  }
};

float clampBPM(float bpm);
void initTransportContext(TransportContext& ctx, float bpm);
void applyTransportAction(TransportContext& ctx, const TransportAction& action);

} // namespace app::transport

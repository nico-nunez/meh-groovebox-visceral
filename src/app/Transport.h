#pragma once

#include <atomic>
#include <cstdint>

namespace app::transport {
inline constexpr float MIN_BPM = 20.0f;
inline constexpr float MAX_BPM = 300.0f;

// =====================
// Transport Runtime
// =====================
enum class TransportMode : uint8_t {
  Stopped = 0,
  Playing = 1,
};

struct TransportRuntime {
  uint32_t sampleRate = 48000;
  float bpm = 120.0f;
  TransportMode mode = TransportMode::Stopped;

  uint64_t samplePosition = 0;
  uint64_t segmentStartSample = 0;

  double beatPosition = 0.0;
  double segmentStartBeat = 0.0;
};

// =====================
// Block Time
// =====================
enum class BoundaryFlags : uint32_t {
  None = 0,
  CrossedBeat = 1 << 0,
  CrossedBar = 1 << 1,
};

struct BlockTimeResult {
  uint32_t sampleRate = 48000;
  float bpm = 120.0f;
  TransportMode mode = TransportMode::Stopped;

  uint64_t startSample = 0;
  uint64_t endSample = 0;

  double startBeat = 0.0;
  double endBeat = 0.0;

  bool advancedThisBlock = false;
  bool startedThisBlock = false;
  bool stoppedThisBlock = false;
  bool loopedThisBlock = false;

  uint32_t boundaryFlags = 0;
};

// =======================
// Transport Event/Queue
// =======================

struct TransportEvent {
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

struct TransportEventQueue {
  static constexpr uint32_t SIZE = 128;
  static constexpr uint32_t MASK = SIZE - 1;

  // SPSC queue: one non-RT producer, one audio-thread consumer.
  TransportEvent queue[SIZE]{};
  std::atomic<uint32_t> writeIndex{0};
  std::atomic<uint32_t> readIndex{0};

  bool push(const TransportEvent& action) {
    uint32_t currentIndex = writeIndex.load();
    uint32_t nextIndex = (currentIndex + 1) & MASK;

    if (nextIndex == readIndex.load())
      return false;

    queue[currentIndex] = action;
    writeIndex.store(nextIndex);

    return true;
  }

  bool pop(TransportEvent& action) {
    uint32_t currentIndex = readIndex.load();

    if (currentIndex == writeIndex.load())
      return false;

    action = queue[currentIndex];
    readIndex.store((currentIndex + 1) & MASK);

    return true;
  }
};

// ======================
// API Functions
// ======================
float clampBPM(float bpm);

void initTransportRuntime(TransportRuntime& rt, uint32_t sampleRate, float bpm);
void applyTransportEvent(TransportRuntime& rt, const TransportEvent& action);

BlockTimeResult advanceTransportBlock(TransportRuntime& runtime,
                                      TransportMode previousMode,
                                      uint32_t numFrames);

} // namespace app::transport

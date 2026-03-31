#include "Events.h"

#include <atomic>
#include <cstdio>

namespace synth::events {

// ========================
// MIDI Event Queue
// ========================
struct MIDIEventQueue {
  // NOTE(nico): SIZE value need to be power of to use bitmasking for wrapping
  // Alternative is modulo (%) which is more expensive
  static constexpr size_t SIZE{256};
  static constexpr size_t WRAP{SIZE - 1};

  MIDIEvent queue[SIZE];

  std::atomic<size_t> readIndex{0};
  std::atomic<size_t> writeIndex{0};

  bool push(const MIDIEvent& event) {
    size_t currentIndex = writeIndex.load();
    size_t nextIndex = (currentIndex + 1) & WRAP;

    if (nextIndex == readIndex.load())
      return false;

    queue[currentIndex] = event;
    writeIndex.store(nextIndex);

    return true;
  }

  bool pop(MIDIEvent& event) {
    size_t currentIndex = readIndex.load();

    if (currentIndex == writeIndex.load())
      return false;

    event = queue[currentIndex];
    readIndex.store((currentIndex + 1) & WRAP);

    return true;
  }
};

// ========================
// Param Event Queue
// ========================

struct ParamEventQueue {
  // NOTE(nico): SIZE value need to be power of to use bitmasking for wrapping
  // Alternative is modulo (%) which is more expensive
  static constexpr size_t SIZE{256};
  static constexpr size_t WRAP{SIZE - 1};

  ParamEvent queue[SIZE];

  std::atomic<size_t> readIndex{0};
  std::atomic<size_t> writeIndex{0};

  bool push(const ParamEvent& event) {
    size_t currentIndex = writeIndex.load();
    size_t nextIndex = (currentIndex + 1) & WRAP;

    if (nextIndex == readIndex.load())
      return false;

    queue[currentIndex] = event;
    writeIndex.store(nextIndex);

    return true;
  }

  bool pop(ParamEvent& event) {
    size_t currentIndex = readIndex.load();

    if (currentIndex == writeIndex.load())
      return false;

    event = queue[currentIndex];
    readIndex.store((currentIndex + 1) & WRAP);

    return true;
  }

  void printEvent(ParamEvent& event) {
    printf("==== Event ====\n");
    printf("paramID: %d\n", (int)event.id);
    printf("value: %f\n", event.value);
  }

  void printQueue() {
    size_t currentIndex = readIndex.load();
    size_t endIndex = writeIndex.load();

    // Only print events that are able to be read
    printf("======== Event Queue ========\n");
    for (; currentIndex < endIndex; currentIndex++) {
      printEvent(queue[currentIndex]);
    }
  }
};

// ========================
// Engine Event Queue
// ========================

struct EngineEventQueue {
  static constexpr size_t SIZE{256};
  static constexpr size_t WRAP{SIZE - 1};

  EngineEvent queue[SIZE];

  std::atomic<size_t> readIndex{0};
  std::atomic<size_t> writeIndex{0};

  bool push(const EngineEvent& event) {
    size_t currentIndex = writeIndex.load();
    size_t nextIndex = (currentIndex + 1) & WRAP;

    if (nextIndex == readIndex.load())
      return false;

    queue[currentIndex] = event;
    writeIndex.store(nextIndex);

    return true;
  }

  bool pop(EngineEvent& event) {
    size_t currentIndex = readIndex.load();

    if (currentIndex == writeIndex.load())
      return false;

    event = queue[currentIndex];
    readIndex.store((currentIndex + 1) & WRAP);

    return true;
  }
};

} // namespace synth::events

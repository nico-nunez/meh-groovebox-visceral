#pragma once

#include <atomic>
#include <cstddef>
#include <cstdio>

namespace synth_events {

struct ParamEvent {
  uint8_t id = 0;
  float value = 0.0f; // Normalized [0, 1]
};

struct ParamQueue;
using hParamQueue = ParamQueue*;

hParamQueue initParamQueue();
void disposeParamQueue(hParamQueue handle);

bool setParam(hParamQueue handle, uint8_t id, float value); // Lua REPL thread
bool popParamEvent(hParamQueue handle, ParamEvent& event);  // audio thread (drain)

struct ParamEventQueue {
  // NOTE(nico): SIZE value need to be power of to use bitmasking for wrapping
  // Alternative is modulo (%) which is more expensive
  static constexpr size_t SIZE{256};
  static constexpr size_t WRAP{SIZE - 1};

  ParamEvent queue[SIZE];

  std::atomic<size_t> readIndex{0};
  std::atomic<size_t> writeIndex{0};

  bool push(const ParamEvent& event);
  bool pop(ParamEvent& event);

  void printEvent(ParamEvent& event);
  void printQueue();
};

} // namespace synth_events

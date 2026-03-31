#include "synth_io/EventIO.h"

#include <cstdio>

namespace synth_io {

// ========================
// MIDI Event Queue
// ========================

bool MIDIEventQueue::push(const MIDIEvent& event) {
  size_t currentIndex = writeIndex.load();
  size_t nextIndex = (currentIndex + 1) & WRAP;

  if (nextIndex == readIndex.load())
    return false;

  queue[currentIndex] = event;
  writeIndex.store(nextIndex);

  return true;
}

bool MIDIEventQueue::pop(MIDIEvent& event) {
  size_t currentIndex = readIndex.load();

  if (currentIndex == writeIndex.load())
    return false;

  event = queue[currentIndex];
  readIndex.store((currentIndex + 1) & WRAP);

  return true;
}

// ========================
// Param Event Queue
// ========================

bool ParamEventQueue::push(const ParamEvent& event) {
  size_t currentIndex = writeIndex.load();
  size_t nextIndex = (currentIndex + 1) & WRAP;

  if (nextIndex == readIndex.load())
    return false;

  queue[currentIndex] = event;
  writeIndex.store(nextIndex);

  return true;
}

bool ParamEventQueue::pop(ParamEvent& event) {
  size_t currentIndex = readIndex.load();

  if (currentIndex == writeIndex.load())
    return false;

  event = queue[currentIndex];
  readIndex.store((currentIndex + 1) & WRAP);

  return true;
}

void ParamEventQueue::printEvent(ParamEvent& event) {
  printf("==== Event ====\n");
  printf("paramID: %d\n", (int)event.id);
  printf("value: %f\n", event.value);
}

void ParamEventQueue::printQueue() {
  size_t currentIndex = readIndex.load();
  size_t endIndex = writeIndex.load();

  // Only print events that are able to be read
  printf("======== Event Queue ========\n");
  for (; currentIndex < endIndex; currentIndex++) {
    printEvent(queue[currentIndex]);
  }
}

} // namespace synth_io

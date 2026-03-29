#include "synth_events/MIDIEventQueue.h"

#include <cstddef>

namespace synth_events {

struct MIDIHandle {
  MIDIEventQueue queue{};
};

hMIDIHandle initMIDIHandle() {
  return new MIDIHandle{};
}

void disposeMIDIHandle(hMIDIHandle handle) {
  delete handle;
}

bool pushMIDIEvent(hMIDIHandle handle, MIDIEvent event) {
  return handle->queue.push(event);
}

bool popMIDIEvent(hMIDIHandle handle, MIDIEvent& event) {
  return handle->queue.pop(event);
}
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

} // namespace synth_events

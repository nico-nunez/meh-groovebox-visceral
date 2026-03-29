#include "synth_events/ParamEventQueue.h"
#include <cstddef>

namespace synth_events {

struct ParamQueue {
  ParamEventQueue queue{};
};

hParamQueue initParamQueue() {
  return new ParamQueue{};
}

void disposeParamQueue(hParamQueue handle) {
  delete handle;
}

bool setParam(hParamQueue handle, uint8_t id, float value) {
  return handle->queue.push({id, value});
}

bool popParamEvent(hParamQueue handle, ParamEvent& event) {
  return handle->queue.pop(event);
}

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

} // namespace synth_events

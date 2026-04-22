#include "BlockScheduler.h"

#include "app/AppContext.h"
#include "app/Sequencer.h"

#include "synth/events/Events.h"

namespace app {

namespace {
namespace seq = sequencer;
using synth::events::ScheduledEvent;

seq::SequencerBlockWindow makeBlockWindow(const TransportBlockInfo& blockInfo) {
  seq::SequencerBlockWindow block{};
  block.startBeat = blockInfo.startBeat;
  block.endBeat = blockInfo.endBeat;
  block.numFrames = blockInfo.numFrames;
  block.stoppedThisBlock = blockInfo.stoppedThisBlock;
  return block;
}

bool compareEvents(const ScheduledEvent& a, const ScheduledEvent& b) {
  if (a.sampleOffset != b.sampleOffset)
    return a.sampleOffset < b.sampleOffset;

  return static_cast<uint8_t>(a.order) < static_cast<uint8_t>(b.order);
}

void sortScheduledEvents(track::ScheduledEventBuffer& events) {
  std::sort(events.buffer, events.buffer + events.count, compareEvents);
}

} // namespace

void runBlockScheduler(AppContext* app, const TransportBlockInfo& blockInfo) {
  auto block = makeBlockWindow(blockInfo);

  seq::SequencerLaneEvents evts{};
  seq::runSequencer(app->sequencer, block, evts);

  for (uint8_t i = 0; i < app->sequencer.numLanes; ++i) {
    auto& trackEvents = app->tracks[i].events;
    trackEvents.clear();

    const auto& lane = evts.lanes[i];
    for (u_int16_t j = 0; j < lane.count; ++j) {
      trackEvents.push(lane.events[j]);
    }

    sortScheduledEvents(trackEvents);
  }
}

} // namespace app

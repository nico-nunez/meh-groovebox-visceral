#include "BlockScheduler.h"

#include "app/AppContext.h"
#include "app/Sequencer.h"

namespace app {

namespace {
namespace seq = sequencer;

seq::SequencerBlockWindow makeBlockWindow(const TransportBlockInfo& blockInfo) {
  seq::SequencerBlockWindow block{};
  block.startBeat = blockInfo.startBeat;
  block.endBeat = blockInfo.endBeat;
  block.numFrames = blockInfo.numFrames;
  block.stoppedThisBlock = blockInfo.stoppedThisBlock;
  return block;
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
    for (uint8_t j = 0; j < lane.count; ++j) {
      trackEvents.push(lane.events[j]);
    }
  }
}

} // namespace app

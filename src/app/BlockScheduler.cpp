#include "BlockScheduler.h"

#include "app/AppContext.h"
#include "app/Sequencer.h"

namespace app {

namespace {
namespace seq = sequencer;

void applySequencerEvent(Engine& engine, const seq::SequencerEvent& entry) {
  switch (entry.kind) {
  case seq::SequencerEvent::Kind::MIDI:
    engine.processMIDIEvent(entry.data.midi);
    break;
  case seq::SequencerEvent::Kind::Param:
    engine.processParamEvent(entry.data.param);
    break;
  case seq::SequencerEvent::Kind::Engine:
    engine.processEngineEvent(entry.data.engine);
    break;
  }
}

seq::SequencerBlockWindow makeBlockWindow(const TransportBlockInfo& blockInfo) {
  seq::SequencerBlockWindow block{};
  block.startBeat = blockInfo.startBeat;
  block.endBeat = blockInfo.endBeat;
  block.stoppedThisBlock = blockInfo.stoppedThisBlock;
  return block;
}

} // namespace

void runBlockScheduler(AppContext* app, const TransportBlockInfo& blockInfo) {
  auto block = makeBlockWindow(blockInfo);

  seq::SequencerLaneEvents evts{};
  seq::runSequencer(app->sequencer, block, evts);

  for (uint8_t i = 0; i < app->sequencer.numLanes; ++i) {
    for (uint8_t j = 0; j < evts.lanes[i].count; ++j) {
      applySequencerEvent(app->tracks[i].engine, evts.lanes[i].events[j]);
    }
  }
}

} // namespace app

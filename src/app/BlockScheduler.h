#pragma once

#include "app/Sequencer.h"
#include "app/Transport.h"

namespace app {
struct AppContext;

using sequencer::SequencerLaneEvents;
using transport::TransportBlockInfo;

struct BlockSchedulerWorkspace {
  SequencerLaneEvents sequencerEvents{};
};

void runBlockScheduler(AppContext* app, const TransportBlockInfo& blockInfo);

} // namespace app

#pragma once

namespace app {

struct AppContext;
namespace transport {
struct TransportBlockInfo;
}

void runBlockScheduler(AppContext* app, const transport::TransportBlockInfo& blockInfo);

// stuff
} // namespace app

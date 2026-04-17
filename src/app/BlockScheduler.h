#pragma once

#include "app/Transport.h"

namespace app {
struct AppContext;

using transport::TransportBlockInfo;

void runBlockScheduler(AppContext* app, const TransportBlockInfo& blockInfo);

} // namespace app

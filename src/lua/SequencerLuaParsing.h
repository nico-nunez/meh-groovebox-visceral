#pragma once

#include "LuaState.h"

#include "app/Types.h"

namespace app::sequencer {
struct StepEvent;
struct LanePattern;
} // namespace app::sequencer

namespace lua {

app::VoidResult parseLuaStepEvent(lua_State* L, int index, app::sequencer::StepEvent& outEvent);

app::VoidResult parseLuaLanePattern(lua_State* L,
                                    int index,
                                    app::sequencer::LanePattern& outPattern);

} // namespace lua

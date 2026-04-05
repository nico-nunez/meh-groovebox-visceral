#pragma once

namespace synth {
struct Engine;
}

namespace synth::param::sync {

void syncDirtyParams(Engine& eng);

} // namespace synth::param::sync

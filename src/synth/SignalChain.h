#pragma once

#include <cstddef>
#include <cstdint>

namespace synth::signal_chain {

inline constexpr uint8_t MAX_CHAIN_SLOTS = 8;

enum SignalProcessor : uint8_t {
  None = 0,
  SVF,
  Ladder,
  Saturator,
};

struct SignalChain {
  SignalProcessor slots[MAX_CHAIN_SLOTS];
  uint8_t length = 0;
};

void setChain(SignalChain& chain, const SignalProcessor* procs, uint8_t count);
void clearChain(SignalChain& chain);

} // namespace synth::signal_chain

#include "SignalChain.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace synth::signal_chain {

void setChain(SignalChain& chain, const SignalProcessor* procs, uint8_t count) {
  chain.length = std::min(count, MAX_CHAIN_SLOTS);

  for (uint8_t i = 0; i < chain.length; i++) {
    chain.slots[i] = procs[i];
  }
}
void clearChain(SignalChain& chain) {
  for (uint8_t i = 0; i < MAX_CHAIN_SLOTS; i++) {
    chain.slots[i] = SignalProcessor::None;
  }
  chain.length = 0;
};

} // namespace synth::signal_chain

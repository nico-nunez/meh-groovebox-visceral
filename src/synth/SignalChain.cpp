#include "SignalChain.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace synth::signal_chain {

void setSigChain(SignalChain& chain, const SignalProcessor* procs, uint8_t count) {
  chain.length = std::min(count, MAX_CHAIN_SLOTS);

  for (uint8_t i = 0; i < chain.length; i++) {
    chain.slots[i] = procs[i];
  }
}
void clearSigChain(SignalChain& chain) {
  for (uint8_t i = 0; i < MAX_CHAIN_SLOTS; i++) {
    chain.slots[i] = SignalProcessor::None;
  }
  chain.length = 0;
};

void parseSigChainCmd(std::istringstream& iss, SignalChain& chain) {
  std::string subcmd;
  iss >> subcmd;

  if (subcmd == "set") {
    SignalProcessor procs[MAX_CHAIN_SLOTS];
    uint8_t count = 0;
    std::string name;

    while (iss >> name && count < MAX_CHAIN_SLOTS) {
      SignalProcessor p = parseSignalProcessor(name.c_str());
      if (p == SignalProcessor::None) {
        printf("signal chain: unknown processor '%s'\n", name.c_str());
        return;
      }
      procs[count++] = p;
    }

    setSigChain(chain, procs, count);
    printf("OK\n");

  } else if (subcmd == "list") {
    if (chain.length == 0) {
      printf("signal chain: (empty)\n");
      return;
    }
    printf("signal chain: ");
    for (uint8_t i = 0; i < chain.length; i++) {
      if (i > 0)
        printf(" -> ");
      printf("%s", signalProcessorToString(chain.slots[i]));
    }
    printf("\n");

  } else if (subcmd == "clear") {
    clearSigChain(chain);
    printf("OK\n");

  } else {
    printf("signal chain subcommands: set <proc...>, list, clear\n");
    printf("valid processors: svf, ladder, saturator\n");
  }
}
} // namespace synth::signal_chain

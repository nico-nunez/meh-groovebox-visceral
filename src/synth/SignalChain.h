#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>

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

void setSigChain(SignalChain& chain, const SignalProcessor* procs, uint8_t count);
void clearSigChain(SignalChain& chain);

struct SignalProcessorMapping {
  const char* name;
  signal_chain::SignalProcessor proc;
};

// ============================
// Parsing Helpers
// ============================
inline constexpr SignalProcessorMapping signalProcessorMappings[] = {
    {"svf", signal_chain::SignalProcessor::SVF},
    {"ladder", signal_chain::SignalProcessor::Ladder},
    {"saturator", signal_chain::SignalProcessor::Saturator},
};

inline signal_chain::SignalProcessor parseSignalProcessor(const char* name) {
  for (const auto& m : signalProcessorMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.proc;
  return signal_chain::SignalProcessor::None;
}

inline const char* signalProcessorToString(signal_chain::SignalProcessor proc) {
  for (const auto& m : signalProcessorMappings)
    if (m.proc == proc)
      return m.name;
  return "unknown";
}

void parseSigChainCmd(std::istringstream& iss, SignalChain& chain);
} // namespace synth::signal_chain

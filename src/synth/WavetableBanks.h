#pragma once

#include "dsp/Wavetable.h"
#include <cstdint>

namespace synth::wavetable::banks {
using dsp::wavetable::WavetableBank;

enum BankID : uint8_t { Sine = 0, Saw, Square, Triangle, SineToSaw, SampleAndHold, Unknown, COUNT };
inline constexpr const char* UNKNOWN_BANK = "unknown";

inline constexpr int MAX_BANKS = COUNT - 1;

void initFactoryBanks();

void registerBank(BankID id, WavetableBank* bank);

WavetableBank* getBankByID(BankID id);
WavetableBank* getBankByName(const char* name);

// ========================
// Parsing Helpers
// ========================

struct BankMapping {
  const char* name;
  BankID id;
};

const char* bankIDToString(BankID id);
BankID parseBankID(const char* name);

} // namespace synth::wavetable::banks

#include "WavetableBanks.h"

#include "dsp/WavetableGen.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace synth::wavetable::banks {

// ================================
// Bank registry
// ================================
static constexpr uint8_t MAX_REGISTRY_BANKS = 32;
static WavetableBank* s_registry[MAX_REGISTRY_BANKS] = {};
static uint8_t s_registryCount = 0;

void registerBank(BankID id, WavetableBank* bank) {
  if (!bank) {
    printf("registerBank: null bank\n");
    return;
  }
  if (s_registryCount >= MAX_REGISTRY_BANKS) {
    printf("registerBank: registry full\n");
    return;
  }
  s_registry[id] = bank;
  s_registryCount++;
}

// TODO(nico-nunez): deregiter is needed once wav importing is implemented

WavetableBank* getBankByID(BankID id) {
  return s_registry[id];
}

WavetableBank* getBankByName(const char* name) {
  for (int i = 0; i < s_registryCount; i++) {
    if (std::strcmp(s_registry[i]->name, name) == 0)
      return s_registry[i];
  }
  return nullptr;
}

// ====================
// Parsing Helpers
// ====================
const char* bankIDToString(BankID id) {
  auto bank = getBankByID(id);
  if (!bank)
    return UNKNOWN_BANK;

  return bank->name;
}

BankID parseBankID(const char* name) {
  for (int i = 0; i < s_registryCount; i++)
    if (std::strcmp(s_registry[i]->name, name) == 0)
      return static_cast<BankID>(i);

  return BankID::Unknown;
}

// ================================
// Bank Initialization
// ================================
void initFactoryBanks() {
  registerBank(BankID::Sine, dsp::wavetable::generateSine());
  registerBank(BankID::Saw, dsp::wavetable::generateSaw());
  registerBank(BankID::Square, dsp::wavetable::generateSquare());
  registerBank(BankID::Triangle, dsp::wavetable::generateTriangle());
  registerBank(BankID::SineToSaw, dsp::wavetable::generateSineToSaw());
}
} // namespace synth::wavetable::banks

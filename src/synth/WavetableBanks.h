#pragma once

#include "dsp/Wavetable.h"

namespace synth::wavetable::banks {
using dsp::wavetable::WavetableBank;

enum BankID { Sine = 0, Saw, Square, Triangle, SineToSaw, COUNT };

void initFactoryBanks();

void registerBank(BankID id, WavetableBank* bank);

WavetableBank* getBankByID(BankID id);
WavetableBank* getBankByName(const char* name);

} // namespace synth::wavetable::banks

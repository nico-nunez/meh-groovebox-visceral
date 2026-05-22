#pragma once

#include "app/Constants.h"
#include "app/Mixer.h"
#include "app/Sequencer.h"

#include "synth/program/SynthProgram.h"

namespace app {

namespace {
using mixer::MixerSnapshot;
using sequencer::PatternSnapshot;
using synth::program::SynthProgram;
} // namespace

struct GrooveboxTargetState {
  SynthProgram synthPrograms[MAX_TRACKS]{};
  bool hasSynthProgram[MAX_TRACKS]{};

  MixerSnapshot mixer{};
  bool hasMixer = false;

  PatternSnapshot sequencer{};
  bool hasSequencer = false;
};

inline void resetGrooveboxTargetStateFlags(GrooveboxTargetState* target) {
  for (uint8_t t = 0; t < MAX_TRACKS; ++t)
    target->hasSynthProgram[t] = false;

  target->hasMixer = false;
  target->hasSequencer = false;
}

} // namespace app

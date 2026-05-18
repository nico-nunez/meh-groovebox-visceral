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

} // namespace app

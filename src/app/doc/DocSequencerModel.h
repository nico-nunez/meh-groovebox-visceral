#pragma once

#include "app/Sequencer.h"
#include "app/doc/DocTypes.h"

namespace app::doc {

struct PatternArena {
  sequencer::LanePattern pool[app::MAX_TRACKS][sequencer::PATTERNS_PER_LANE]{};

  sequencer::LanePattern* get(uint8_t track, uint8_t slot) { return &pool[track][slot]; }
  const sequencer::LanePattern* get(uint8_t track, uint8_t slot) const {
    return &pool[track][slot];
  }
};

struct AuthoredPatternSlot {
  bool occupied = false;
  sequencer::LanePattern* pattern = nullptr;
  SourceSpan slotSpan{};
};

struct AuthoredTrackSeqModel {
  AuthoredPatternSlot patterns[sequencer::PATTERNS_PER_LANE]{};
  uint8_t activeSlot = sequencer::INVALID_PATTERN_SLOT;
  SourceSpan patternsSpan{};
  SourceSpan activeSlotSpan{};
  SourceSpan trackSpan{};
  uint8_t trackIndex = 0;
  ActivePatternSlotSource activeSlotSource = ActivePatternSlotSource::Unset;
  bool explicitlyAuthoredEmpty = false;
};

struct AuthoredSeqDocModel {
  DocID documentID = 0;
  DocRevision revision = 0;
  bool hasTrackState[app::MAX_TRACKS]{};
  AuthoredTrackSeqModel tracks[app::MAX_TRACKS]{};
};

} // namespace app::doc

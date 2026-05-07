#pragma once

#include "app/Constants.h"
#include "app/doc/DocSequencerModel.h"
#include "app/doc/DocSynthSettingsMetadata.h"
#include "app/doc/DocTypes.h"

#include <vector>

namespace app::doc {

struct AuthoredSynthParamWrite {
  synth::param::ParamID paramID = synth::param::PARAM_UNKNOWN;
  float value = 0.0f;
  const AuthoredSynthParamField* field = nullptr;
  SourceSpan span{};
};

struct AuthoredTrackSynthPatch {
  bool hasPatch = false;
  uint8_t trackIndex = 0;
  SourceSpan trackSpan{};
  std::vector<AuthoredSynthParamWrite> writes{};
};

struct AuthoredDocModel {
  DocID documentID = 0;
  DocRevision revision = 0;

  AuthoredSeqDocModel sequencer{};

  bool hasSynthState[app::MAX_TRACKS]{};
  AuthoredTrackSynthPatch synthTracks[app::MAX_TRACKS]{};
};

} // namespace app::doc

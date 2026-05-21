#pragma once

#include "app/Constants.h"
#include "app/doc/DocMixerSettingsMetadata.h"
#include "app/doc/DocSequencerModel.h"
#include "app/doc/DocSynthSettingsMetadata.h"
#include "app/doc/DocTypes.h"

#include <vector>

namespace app::doc {

// ==== Mixer ====
struct AuthoredMixerParamWrite {
  app::params::AppParamID paramID = app::params::AppParamID::Count;
  float value = 0.0f;
  const AuthoredMixerParamField* field = nullptr;
  SourceSpan span{};
};

struct AuthoredTrackMixerPatch {
  bool hasPatch = false;
  uint8_t trackIndex = 0;
  SourceSpan trackSpan{};
  std::vector<AuthoredMixerParamWrite> writes{};
};

// ==== Synth ====
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

// ==== Doc ====
struct AuthoredDocModel {
  DocID documentID = 0;
  DocRevision revision = 0;

  AuthoredSeqDocModel sequencer{};

  bool hasSynthState[app::MAX_TRACKS]{};
  AuthoredTrackSynthPatch synthTracks[app::MAX_TRACKS]{};

  bool hasMixerState[app::MAX_TRACKS]{};
  AuthoredTrackMixerPatch mixerTracks[app::MAX_TRACKS]{};
};

inline void resetAuthoredDocModel(AuthoredDocModel* model, DocID documentID, DocRevision revision) {
  *model = AuthoredDocModel{};
  model->documentID = documentID;
  model->revision = revision;
  model->sequencer.documentID = documentID;
  model->sequencer.revision = revision;
}

} // namespace app::doc

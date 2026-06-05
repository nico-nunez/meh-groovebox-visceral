#pragma once

#include "app/Constants.h"
#include "app/doc/DocSequencerModel.h"
#include "app/doc/DocTypes.h"
#include "app/doc/metadata/DocMixerSettingsMetadata.h"
#include "app/doc/metadata/DocSynthSettingsMetadata.h"

#include "synth/ModMatrix.h"
#include "synth/SignalChain.h"
#include "synth/WavetableOsc.h"
#include "synth/params/ParamDefs.h"

#include <vector>

namespace app::doc {

namespace {
using synth::mod_matrix::ModDest;
using synth::mod_matrix::ModSrc;
using synth::param::ParamID;
using synth::signal_chain::SignalProcessor;
using synth::wavetable::osc::FMCarrier;
using synth::wavetable::osc::FMSource;
} // namespace

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
  ParamID paramID = ParamID::PARAM_UNKNOWN;
  float value = 0.0f;
  const AuthoredSynthParamField* field = nullptr;
  SourceSpan span{};
};

struct AuthoredModRoute {
  ModSrc src = ModSrc::NoSrc;
  ModDest dest = ModDest::NoDest;
  float amount = 0.0f;
  SourceSpan span{};
};

struct AuthoredFMRoute {
  FMCarrier carrier = FMCarrier::None;
  FMSource modulator = FMSource::None;
  float depth = 0.0f;
  SourceSpan span{};
};

struct AuthoredTrackSynthPatch {
  bool hasPatch = false;
  uint8_t trackIndex = 0;
  SourceSpan trackSpan{};
  std::vector<AuthoredSynthParamWrite> writes{};

  bool hasModRoutes = false;
  std::vector<AuthoredModRoute> modRoutes{};

  bool hasFMRoutes = false;
  std::vector<AuthoredFMRoute> fmRoutes{};

  bool hasSignalChain = false;
  std::vector<SignalProcessor> signalChain{};
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
  model->~AuthoredDocModel();
  new (model) AuthoredDocModel;

  *model = AuthoredDocModel{};
  model->documentID = documentID;
  model->revision = revision;
  model->sequencer.documentID = documentID;
  model->sequencer.revision = revision;
}

} // namespace app::doc

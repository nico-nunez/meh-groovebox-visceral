#include "app/doc/DocMixerPlanner.h"

#include "app/AppParams.h"
#include "app/Mixer.h"
#include "app/doc/DocMetadata.h"
#include "dsp/Math.h"

namespace app::doc {
namespace {

void upsertMixerWrite(AuthoredTrackMixerPatch& patch, const AuthoredMixerParamWrite& write) {
  for (auto& existing : patch.writes) {
    if (existing.paramID == write.paramID) {
      existing = write;
      return;
    }
  }
  patch.writes.push_back(write);
}

DocDiagnostic makeMixerTargetDiagnostic(DocID documentID,
                                        DocRevision revision,
                                        const AuthoredMixerParamWrite* write,
                                        const char* message) {
  DocDiagnostic d{};
  d.severity = DiagnosticSeverity::Error;
  d.source = DiagnosticSource::Planner;
  d.documentID = documentID;
  d.revision = revision;
  d.code = docdiag::MixerPlanningFailed;
  d.message = message ? message : "mixer target build failed";
  if (write) {
    d.span = write->span;
    if (write->field && write->field->authoredField)
      d.relatedTarget = write->field->authoredField;
  }
  return d;
}

bool applyMixerWrite(app::mixer::MixerSnapshot* mixer,
                     uint8_t trackIndex,
                     const AuthoredMixerParamWrite& write,
                     DocID documentID,
                     DocRevision revision,
                     DocDiagnostics* diagnostics) {
  if (!mixer || !diagnostics)
    return false;

  if (!app::params::isValidAppParamID(write.paramID)) {
    diagnostics->push_back(
        makeMixerTargetDiagnostic(documentID, revision, &write, "invalid mixer param id"));
    return false;
  }

  const auto& def = app::params::getAppParamDef(write.paramID);
  if (write.value < def.min || write.value > def.max) {
    diagnostics->push_back(
        makeMixerTargetDiagnostic(documentID, revision, &write, "mixer param out of range"));
    return false;
  }

  const float value = app::params::clampAppParam(write.paramID, write.value);

  switch (write.paramID) {
  case app::params::AppParamID::TrackGain:
    mixer->tracks[trackIndex].gain = value;
    return true;
  case app::params::AppParamID::TrackPan:
    mixer->tracks[trackIndex].pan = value;
    return true;
  case app::params::AppParamID::TrackMute:
    mixer->tracks[trackIndex].enabled = value < 0.5f;
    return true;
  case app::params::AppParamID::MasterGain:
    mixer->masterGain = value;
    return true;
  case app::params::AppParamID::LimiterThresholdDB:
    mixer->limiterThreshold = dsp::math::dBToLinear(value);
    return true;
  case app::params::AppParamID::Count:
    break;
  }

  diagnostics->push_back(
      makeMixerTargetDiagnostic(documentID, revision, &write, "unhandled mixer param id"));
  return false;
}

} // namespace

MixerTargetResult buildMixerTargetSnapshot(const AuthoredDocModel* model,
                                           DocID documentID,
                                           DocRevision revision,
                                           mixer::MixerSnapshot* out) {
  MixerTargetResult result{};
  if (!model || !out) {
    const char* errMsg = !model ? "null authored model" : "null mixer target out";
    result.diagnostics.push_back(makeMixerTargetDiagnostic(documentID, revision, nullptr, errMsg));
    return result;
  }

  mixer::initMixerSnapshot(out);

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!model->hasMixerState[trackIndex])
      continue;

    const AuthoredTrackMixerPatch& patch = model->mixerTracks[trackIndex];
    for (const auto& write : patch.writes) {
      if (!applyMixerWrite(out, trackIndex, write, documentID, revision, &result.diagnostics))
        return result;
    }
  }

  result.ok = true;
  return result;
}

void buildAdmittedMixerTargetModel(const AuthoredDocModel* nextModel, AuthoredDocModel* admitted) {
  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!nextModel->hasMixerState[trackIndex])
      continue;

    admitted->hasMixerState[trackIndex] = true;
    AuthoredTrackMixerPatch& admittedPatch = admitted->mixerTracks[trackIndex];
    if (!admittedPatch.hasPatch) {
      admittedPatch.hasPatch = true;
      admittedPatch.trackIndex = trackIndex;
    }
    admittedPatch.trackSpan = nextModel->mixerTracks[trackIndex].trackSpan;

    for (const auto& write : nextModel->mixerTracks[trackIndex].writes)
      upsertMixerWrite(admittedPatch, write);
  }
}

} // namespace app::doc

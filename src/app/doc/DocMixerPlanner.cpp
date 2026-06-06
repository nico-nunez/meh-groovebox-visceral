#include "app/doc/DocMixerPlanner.h"

#include "app/AppParams.h"
#include "app/doc/DocMetadata.h"

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

bool appendMixerPatchWrite(app::MixerPatch* patch,
                           uint8_t trackIndex,
                           const AuthoredMixerParamWrite& write,
                           DocID documentID,
                           DocRevision revision,
                           DocDiagnostics* diagnostics) {
  if (patch->writeCount >= app::MAX_MIXER_PARAM_PATCH_WRITES) {
    diagnostics->push_back(
        makeMixerTargetDiagnostic(documentID, revision, &write, "mixer patch capacity exceeded"));
    return false;
  }

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

  app::MixerParamPatchWrite& dst = patch->writes[patch->writeCount++];
  dst.paramID = write.paramID;
  dst.trackIndex = trackIndex;
  dst.value = app::params::clampAppParam(write.paramID, write.value);
  return true;
}

} // namespace

MixerTargetResult buildMixerPatch(const AuthoredDocModel* model,
                                  DocID documentID,
                                  DocRevision revision,
                                  app::MixerPatch* out) {
  MixerTargetResult result{};
  if (!model || !out) {
    const char* errMsg = !model ? "null authored model" : "null mixer patch out";
    result.diagnostics.push_back(makeMixerTargetDiagnostic(documentID, revision, nullptr, errMsg));
    return result;
  }

  *out = app::MixerPatch{};

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!model->hasMixerState[trackIndex])
      continue;

    const AuthoredTrackMixerPatch& authored = model->mixerTracks[trackIndex];
    for (const auto& write : authored.writes) {
      if (!appendMixerPatchWrite(out, trackIndex, write, documentID, revision, &result.diagnostics))
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

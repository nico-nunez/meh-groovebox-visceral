#include "app/doc/DocMixerPlanner.h"

namespace app::doc {
namespace {

const AuthoredMixerParamWrite* findMixerWrite(const AuthoredTrackMixerPatch& patch,
                                              app::params::AppParamID paramID) {
  for (const auto& write : patch.writes) {
    if (write.paramID == paramID)
      return &write;
  }
  return nullptr;
}

bool previousMixerValueMatches(const AuthoredDocModel* previous,
                               uint8_t trackIndex,
                               const AuthoredMixerParamWrite& nextWrite) {
  if (!previous || !previous->hasMixerState[trackIndex])
    return false;
  const auto* prev = findMixerWrite(previous->mixerTracks[trackIndex], nextWrite.paramID);
  return prev && prev->value == nextWrite.value;
}

void upsertMixerWrite(AuthoredTrackMixerPatch& patch, const AuthoredMixerParamWrite& write) {
  for (auto& existing : patch.writes) {
    if (existing.paramID == write.paramID) {
      existing = write;
      return;
    }
  }
  patch.writes.push_back(write);
}

} // namespace

PlannedMixerApply planMixerApply(const AuthoredDocModel& nextModel,
                                 const AuthoredDocModel* previousAdmittedModel) {
  PlannedMixerApply result{};
  result.ok = true;

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!nextModel.hasMixerState[trackIndex])
      continue;

    const AuthoredTrackMixerPatch& patch = nextModel.mixerTracks[trackIndex];
    for (const auto& write : patch.writes) {
      if (previousMixerValueMatches(previousAdmittedModel, trackIndex, write))
        continue;

      PlannedMixerParamOp op{};
      op.trackIndex = trackIndex;
      op.paramID = write.paramID;
      op.value = write.value;
      op.field = write.field;
      op.span = write.span;
      result.paramOps.push_back(op);
    }
  }

  return result;
}

AuthoredDocModel buildAdmittedMixerTargetModel(const AuthoredDocModel& nextModel,
                                               const AuthoredDocModel* previousAdmittedModel) {
  AuthoredDocModel admitted = previousAdmittedModel ? *previousAdmittedModel : AuthoredDocModel{};

  admitted.documentID = nextModel.documentID;
  admitted.revision = nextModel.revision;

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!nextModel.hasMixerState[trackIndex])
      continue;

    admitted.hasMixerState[trackIndex] = true;
    AuthoredTrackMixerPatch& admittedPatch = admitted.mixerTracks[trackIndex];
    if (!admittedPatch.hasPatch) {
      admittedPatch.hasPatch = true;
      admittedPatch.trackIndex = trackIndex;
    }
    admittedPatch.trackSpan = nextModel.mixerTracks[trackIndex].trackSpan;

    for (const auto& write : nextModel.mixerTracks[trackIndex].writes)
      upsertMixerWrite(admittedPatch, write);
  }

  return admitted;
}

} // namespace app::doc

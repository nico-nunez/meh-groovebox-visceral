#include "app/doc/DocSynthPlanner.h"

namespace app::doc {
namespace {

bool sameParam(const AuthoredSynthParamWrite& write, synth::param::ParamID paramID) {
  return write.paramID == paramID;
}

const AuthoredSynthParamWrite* findWrite(const AuthoredTrackSynthPatch& patch,
                                         synth::param::ParamID paramID) {
  for (const auto& write : patch.writes) {
    if (sameParam(write, paramID))
      return &write;
  }
  return nullptr;
}

bool previousValueMatches(const AuthoredDocModel* previous,
                          uint8_t trackIndex,
                          const AuthoredSynthParamWrite& nextWrite) {
  if (!previous || !previous->hasSynthState[trackIndex])
    return false;

  const auto* previousWrite = findWrite(previous->synthTracks[trackIndex], nextWrite.paramID);
  return previousWrite && previousWrite->value == nextWrite.value;
}

void upsertWrite(AuthoredTrackSynthPatch& patch, const AuthoredSynthParamWrite& write) {
  for (auto& existing : patch.writes) {
    if (existing.paramID == write.paramID) {
      existing = write;
      return;
    }
  }
  patch.writes.push_back(write);
}

} // namespace

PlannedSynthApply planSynthApply(const AuthoredDocModel& nextModel,
                                 const AuthoredDocModel* previousAdmittedModel) {
  PlannedSynthApply result{};
  result.ok = true;

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!nextModel.hasSynthState[trackIndex])
      continue;

    const AuthoredTrackSynthPatch& patch = nextModel.synthTracks[trackIndex];
    for (const auto& write : patch.writes) {
      if (previousValueMatches(previousAdmittedModel, trackIndex, write))
        continue;

      PlannedSynthParamOp op{};
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

AuthoredDocModel buildAdmittedSynthTargetModel(const AuthoredDocModel& nextModel,
                                               const AuthoredDocModel* previousAdmittedModel) {
  AuthoredDocModel admitted = previousAdmittedModel ? *previousAdmittedModel : AuthoredDocModel{};

  admitted.documentID = nextModel.documentID;
  admitted.revision = nextModel.revision;
  admitted.sequencer.documentID = nextModel.sequencer.documentID;
  admitted.sequencer.revision = nextModel.sequencer.revision;

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!nextModel.hasSynthState[trackIndex])
      continue;

    admitted.hasSynthState[trackIndex] = true;
    AuthoredTrackSynthPatch& admittedPatch = admitted.synthTracks[trackIndex];
    if (!admittedPatch.hasPatch) {
      admittedPatch.hasPatch = true;
      admittedPatch.trackIndex = trackIndex;
    }
    admittedPatch.trackSpan = nextModel.synthTracks[trackIndex].trackSpan;

    for (const auto& write : nextModel.synthTracks[trackIndex].writes)
      upsertWrite(admittedPatch, write);
  }

  return admitted;
}

} // namespace app::doc

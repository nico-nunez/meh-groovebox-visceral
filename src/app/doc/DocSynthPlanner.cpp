#include "app/doc/DocSynthPlanner.h"

#include "app/doc/DocMetadata.h"
#include "synth/program/SynthProgram.h"

namespace app::doc {
namespace {

void upsertWrite(AuthoredTrackSynthPatch& patch, const AuthoredSynthParamWrite& write) {
  for (auto& existing : patch.writes) {
    if (existing.paramID == write.paramID) {
      existing = write;
      return;
    }
  }
  patch.writes.push_back(write);
}

DocDiagnostic makeSynthTargetDiagnostic(DocID documentID,
                                        DocRevision revision,
                                        const AuthoredSynthParamWrite* write,
                                        const char* message) {
  DocDiagnostic d{};
  d.severity = DiagnosticSeverity::Error;
  d.source = DiagnosticSource::Planner;
  d.documentID = documentID;
  d.revision = revision;
  d.code = docdiag::SynthPlanningFailed;
  d.message = message ? message : "synth target build failed";
  if (write) {
    d.span = write->span;
    if (write->field && write->field->authoredPath)
      d.relatedTarget = write->field->authoredPath;
  }
  return d;
}

} // namespace

SynthTargetProgramsResult buildSynthTargetPrograms(const AuthoredDocModel* model,
                                                   DocID documentID,
                                                   DocRevision revision,
                                                   synth::program::SynthProgram* out) {
  SynthTargetProgramsResult result{};
  if (!model || !out) {
    const char* errMsg = !model ? "null authored model" : "null synth target out";
    result.diagnostics.push_back(makeSynthTargetDiagnostic(documentID, revision, nullptr, errMsg));
    return result;
  }

  for (uint8_t t = 0; t < app::MAX_TRACKS; ++t)
    synth::program::initSynthProgram(out[t]);

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!model->hasSynthState[trackIndex])
      continue;

    const AuthoredTrackSynthPatch& patch = model->synthTracks[trackIndex];
    auto& program = out[trackIndex];

    for (const auto& write : patch.writes) {
      if (write.paramID == synth::param::PARAM_UNKNOWN ||
          static_cast<int>(write.paramID) >= synth::param::PARAM_COUNT) {
        result.diagnostics.push_back(
            makeSynthTargetDiagnostic(documentID, revision, &write, "invalid synth param id"));
        return result;
      }

      const auto& def = synth::param::PARAM_DEFS[static_cast<int>(write.paramID)];
      if (write.value < def.min || write.value > def.max) {
        result.diagnostics.push_back(
            makeSynthTargetDiagnostic(documentID, revision, &write, "synth param out of range"));
        return result;
      }

      program.paramValues[static_cast<int>(write.paramID)] = write.value;
    }
  }

  result.ok = true;
  return result;
}

void buildAdmittedSynthTargetModel(const AuthoredDocModel* nextModel, AuthoredDocModel* admitted) {
  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!nextModel->hasSynthState[trackIndex])
      continue;

    admitted->hasSynthState[trackIndex] = true;
    AuthoredTrackSynthPatch& admittedPatch = admitted->synthTracks[trackIndex];
    if (!admittedPatch.hasPatch) {
      admittedPatch.hasPatch = true;
      admittedPatch.trackIndex = trackIndex;
    }
    admittedPatch.trackSpan = nextModel->synthTracks[trackIndex].trackSpan;

    for (const auto& write : nextModel->synthTracks[trackIndex].writes)
      upsertWrite(admittedPatch, write);
  }
}

} // namespace app::doc

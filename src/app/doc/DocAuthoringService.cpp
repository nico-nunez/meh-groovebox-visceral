#include "app/doc/DocAuthoringService.h"

#include "app/AppContext.h"
#include "app/doc/DocSequencerParser.h"
#include "app/doc/DocSequencerPlanner.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

namespace app::doc {
namespace {

std::string trackTarget(uint8_t trackIndex, const char* suffix) {
  std::string target = "track:";
  target += std::to_string(static_cast<int>(trackIndex) + 1);
  if (suffix && suffix[0] != '\0') {
    target += ".";
    target += suffix;
  }
  return target;
}

bool isActiveSlotAdmissionFailure(const char* message) {
  return message && std::string(message).find("activeSlot") != std::string::npos;
}

DocDiagnostic makeSequencerAdmissionDiagnostic(DocID documentID,
                                               DocRevision revision,
                                               const AuthoredTrackSeqModel* track,
                                               const char* message) {
  DocDiagnostic diagnostic{};
  diagnostic.severity = DiagnosticSeverity::Error;
  diagnostic.source = DiagnosticSource::GrooveboxAdmission;
  diagnostic.documentID = documentID;
  diagnostic.revision = revision;
  diagnostic.code = "sequencer.admission_failed";
  diagnostic.message = message ? message : "sequencer admission failed";
  if (track) {
    const bool activeSlotFailure = isActiveSlotAdmissionFailure(message);
    if (activeSlotFailure) {
      diagnostic.span = track->activeSlotSpan;
      diagnostic.relatedTarget = trackTarget(track->trackIndex, "activeSlot");
    } else {
      diagnostic.span = track->patternsSpan;
      diagnostic.relatedTarget = trackTarget(track->trackIndex, "patterns");
    }
  }
  return diagnostic;
}

DocDiagnostic makeFileReadDiagnostic(DocAuthoringService& service,
                                     DocRevision revision,
                                     const char* path) {
  DocDiagnostic diagnostic{};
  diagnostic.severity = DiagnosticSeverity::Error;
  diagnostic.source = DiagnosticSource::Parser;
  diagnostic.documentID = service.buffer.documentID;
  diagnostic.revision = revision;
  diagnostic.code = "document.file.read_failed";
  diagnostic.message = "failed to read document file";
  diagnostic.relatedTarget = path ? path : "";
  return diagnostic;
}

ApplyOperationID beginApplyOperation(DocAuthoringService& service) {
  if (service.apply.activeApplyOperationID != 0) {
    service.apply.lastSupersededApplyOperationID = service.apply.activeApplyOperationID;
    service.apply.status = ApplyStatus::Superseded;
    service.apply.activeApplyOperationID = 0;
  }

  const ApplyOperationID id = service.apply.nextApplyOperationID++;
  service.apply.activeApplyOperationID = id;
  service.apply.status = ApplyStatus::Started;
  service.apply.diagnostics.clear();
  return id;
}

void markApplyFailed(DocAuthoringService& service, ApplyOperationID id) {
  if (service.apply.activeApplyOperationID == id)
    service.apply.activeApplyOperationID = 0;
  service.apply.status = ApplyStatus::Failed;
}

void markApplyCompleted(DocAuthoringService& service, ApplyOperationID id) {
  if (service.apply.activeApplyOperationID == id)
    service.apply.activeApplyOperationID = 0;
  service.apply.status = ApplyStatus::Completed;
}

void failApply(DocAuthoringService& service,
               ApplyOperationID operationID,
               DocDiagnostics diagnostics) {
  service.apply.diagnostics = diagnostics;
  markApplyFailed(service, operationID);
}

bool submitSequencerPlan(DocAuthoringService& service,
                         app::AppContext& app,
                         DocRevision revision,
                         const AuthoredSeqDocModel& model,
                         const PlannedSequencerApply& plan,
                         DocDiagnostics& diagnostics) {
  if (plan.trackOps.empty())
    return true;

  auto begin = sequencer::beginPatternEdit(app.sequencer, true);
  if (!begin.ok) {
    diagnostics.push_back(
        makeSequencerAdmissionDiagnostic(service.buffer.documentID, revision, nullptr, begin.err));
    return false;
  }

  for (const PlannedSequencerTrackOp& op : plan.trackOps) {
    auto replace = sequencer::replacePatternBank(app.sequencer, op.trackIndex, op.bank);
    if (!replace.ok) {
      auto abort = sequencer::abortPatternEdit(app.sequencer);
      const AuthoredTrackSeqModel* track =
          model.hasTrackState[op.trackIndex] ? &model.tracks[op.trackIndex] : nullptr;
      diagnostics.push_back(makeSequencerAdmissionDiagnostic(service.buffer.documentID,
                                                             revision,
                                                             track,
                                                             replace.err));
      if (!abort.ok)
        diagnostics.push_back(makeSequencerAdmissionDiagnostic(service.buffer.documentID,
                                                               revision,
                                                               nullptr,
                                                               abort.err));
      return false;
    }
  }

  auto commit = sequencer::commitPattern(app.sequencer);
  if (!commit.ok) {
    auto abort = sequencer::abortPatternEdit(app.sequencer);
    diagnostics.push_back(
        makeSequencerAdmissionDiagnostic(service.buffer.documentID, revision, nullptr, commit.err));
    if (!abort.ok)
      diagnostics.push_back(makeSequencerAdmissionDiagnostic(service.buffer.documentID,
                                                             revision,
                                                             nullptr,
                                                             abort.err));
    return false;
  }

  return true;
}

AuthoredSeqDocModel buildAdmittedTargetModel(const AuthoredSeqDocModel& nextModel,
                                             const AuthoredSeqDocModel* previousAdmittedModel) {
  AuthoredSeqDocModel admitted =
      previousAdmittedModel ? *previousAdmittedModel : AuthoredSeqDocModel{};

  admitted.documentID = nextModel.documentID;
  admitted.revision = nextModel.revision;

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!nextModel.hasTrackState[trackIndex])
      continue;

    admitted.hasTrackState[trackIndex] = true;
    admitted.tracks[trackIndex] = nextModel.tracks[trackIndex];
  }

  return admitted;
}

} // namespace

ApplyRevisionResult applySequencerRevision(DocAuthoringService& service,
                                           app::AppContext& app,
                                           DocRevision revision,
                                           const char* bufferText) {
  ApplyRevisionResult result{};
  const ApplyOperationID operationID = beginApplyOperation(service);
  result.applyOperationID = operationID;

  service.buffer.currentRevision = revision;
  service.buffer.bufferText = bufferText ? bufferText : "";

  SequencerNormalizeResult normalize =
      parseAndNormalizeSequencerDocument(service.buffer.documentID,
                                         revision,
                                         service.buffer.bufferText.c_str());
  if (!normalize.ok) {
    failApply(service, operationID, normalize.diagnostics);
    result.diagnostics = service.apply.diagnostics;
    return result;
  }

  service.apply.status = ApplyStatus::Validated;

  const AuthoredSeqDocModel* previous =
      service.apply.hasLastAdmittedSequencerModel ? &service.apply.lastAdmittedSeqModel : nullptr;
  PlannedSequencerApply plan = planSequencerApply(normalize.model, previous);
  if (!plan.ok) {
    failApply(service, operationID, plan.diagnostics);
    result.diagnostics = service.apply.diagnostics;
    return result;
  }

  service.apply.status = ApplyStatus::Planned;

  DocDiagnostics admissionDiagnostics{};
  if (!submitSequencerPlan(service, app, revision, normalize.model, plan, admissionDiagnostics)) {
    failApply(service, operationID, admissionDiagnostics);
    result.diagnostics = service.apply.diagnostics;
    return result;
  }

  service.apply.lastAdmittedSeqModel = buildAdmittedTargetModel(normalize.model, previous);
  service.apply.hasLastAdmittedSequencerModel = true;
  service.buffer.lastAdmittedRevision = revision;
  service.apply.status = ApplyStatus::Admitted;
  markApplyCompleted(service, operationID);

  result.ok = true;
  result.diagnostics = service.apply.diagnostics;
  return result;
}

ApplyRevisionResult applySequencerFile(DocAuthoringService& service,
                                       app::AppContext& app,
                                       const char* path) {
  const DocRevision revision = service.buffer.currentRevision + 1;
  service.buffer.path = path ? path : "";

  std::ifstream input(service.buffer.path);
  if (!input) {
    ApplyRevisionResult result{};
    const ApplyOperationID operationID = beginApplyOperation(service);
    result.applyOperationID = operationID;
    service.buffer.currentRevision = revision;
    service.apply.diagnostics.push_back(makeFileReadDiagnostic(service, revision, path));
    markApplyFailed(service, operationID);
    result.diagnostics = service.apply.diagnostics;
    return result;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string fileText = buffer.str();
  return applySequencerRevision(service, app, revision, fileText.c_str());
}

void initDocAuthoringService(DocAuthoringService& service) {
  service = DocAuthoringService{};
}

} // namespace app::doc

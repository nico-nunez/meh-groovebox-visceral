#include "app/doc/DocAuthoringService.h"

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/doc/DocGrooveboxTargetBuilder.h"
#include "app/doc/DocMetadata.h"
#include "app/doc/DocMixerPlanner.h"
#include "app/doc/DocSequencerParser.h"
#include "app/doc/DocSequencerPlanner.h"
#include "app/doc/DocSynthPlanner.h"

#include <fstream>
#include <sstream>
#include <string>

namespace app::doc {
namespace {

DocDiagnostic makeFileReadDiagnostic(DocAuthoringService& service,
                                     DocRevision revision,
                                     const char* path) {
  DocDiagnostic diagnostic{};
  diagnostic.severity = DiagnosticSeverity::Error;
  diagnostic.source = DiagnosticSource::Parser;
  diagnostic.documentID = service.buffer.documentID;
  diagnostic.revision = revision;
  diagnostic.code = docdiag::DocumentFileReadFailed;
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

void buildAdmittedDocumentModel(const AuthoredDocModel* nextModel, DocApplyState* apply) {
  AuthoredDocModel* admitted = &apply->lastAdmittedDocModel;
  PatternArena* admittedArena = apply->admittedArena;

  admitted->documentID = nextModel->documentID;
  admitted->revision = nextModel->revision;
  admitted->sequencer.documentID = nextModel->sequencer.documentID;
  admitted->sequencer.revision = nextModel->sequencer.revision;

  buildAdmittedSynthTargetModel(nextModel, admitted);
  buildAdmittedMixerTargetModel(nextModel, admitted);
  buildAdmittedSeqTargetModel(nextModel, admitted, admittedArena);

  apply->hasLastAdmittedDocModel = true;
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

  AuthoredDocumentNormalizeResult normalize =
      parseAndNormalizeAuthoredDocument(service.buffer.documentID,
                                        revision,
                                        service.buffer.bufferText.c_str(),
                                        service.apply.scratchArena);

  if (!normalize.ok) {
    failApply(service, operationID, normalize.diagnostics);
    result.diagnostics = service.apply.diagnostics;
    return result;
  }

  service.apply.status = ApplyStatus::Validated;

  GrooveboxTargetState* target = &service.applyWorkspace->target;
  GrooveboxTargetBuildResult build =
      buildGrooveboxTargetState(&normalize.model, service.buffer.documentID, revision, target);
  if (!build.ok) {
    failApply(service, operationID, build.diagnostics);
    result.diagnostics = service.apply.diagnostics;
    return result;
  }

  service.apply.status = ApplyStatus::Planned;

  GrooveboxEditSession session{};
  beginGrooveboxEdit(&session, revision);
  stageGrooveboxTarget(&session, target);

  DocDiagnostics admissionDiagnostics{};
  GrooveboxEditResult edit = commitGrooveboxEditImmediate(&session, &app, &admissionDiagnostics);
  if (!edit.ok) {
    failApply(service, operationID, admissionDiagnostics);
    result.diagnostics = service.apply.diagnostics;
    return result;
  }

  buildAdmittedDocumentModel(&normalize.model, &service.apply);
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
  destroyDocAuthoringService(service);

  service = DocAuthoringService{};
  service.apply.admittedArena = new PatternArena{};
  service.apply.scratchArena = new PatternArena{};
  service.applyWorkspace = new DocApplyWorkspace{};
}

void destroyDocAuthoringService(DocAuthoringService& service) {
  delete service.apply.admittedArena;
  delete service.apply.scratchArena;
  delete service.applyWorkspace;
  service = DocAuthoringService{};
}

} // namespace app::doc

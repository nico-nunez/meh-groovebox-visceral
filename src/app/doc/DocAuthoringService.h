#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"

#include <string>

namespace app {
struct AppContext;
}

namespace app::doc {

struct ApplyRevisionResult {
  bool ok = false;
  ApplyOperationID applyOperationID = 0;
  DocDiagnostics diagnostics{};
};

struct DocBufferState {
  DocID documentID = 1;
  DocRevision currentRevision = 0;
  DocRevision lastAdmittedRevision = 0;
  std::string path{};
  std::string bufferText{};
};

struct DocApplyState {
  ApplyOperationID nextApplyOperationID = 1;
  ApplyOperationID activeApplyOperationID = 0;
  ApplyOperationID lastSupersededApplyOperationID = 0;
  ApplyStatus status = ApplyStatus::Idle;
  DocDiagnostics diagnostics{};

  AuthoredDocModel lastAdmittedDocModel{};
  bool hasLastAdmittedDocModel = false;
};

struct DocApplyWorkspace {
  AuthoredDocModel parseModel{};
  app::GrooveboxPatch patch{};
};

struct DocAuthoringService {
  DocBufferState buffer{};
  DocApplyState apply{};
  DocApplyWorkspace* applyWorkspace{};
};

void initDocAuthoringService(DocAuthoringService& service);
void destroyDocAuthoringService(DocAuthoringService& service);

ApplyRevisionResult applyAuthoredDocRevision(DocAuthoringService& service,
                                             app::AppContext& app,
                                             DocRevision revision,
                                             const char* bufferText);

ApplyRevisionResult applySequencerFile(DocAuthoringService& service,
                                       app::AppContext& app,
                                       const char* path);

inline const DocDiagnostics& getDocDiagnostics(const DocAuthoringService& service) {
  return service.apply.diagnostics;
}

inline ApplyStatus getApplyStatus(const DocAuthoringService& service) {
  return service.apply.status;
}

inline DocRevision getLastAdmittedRevision(const DocAuthoringService& service) {
  return service.buffer.lastAdmittedRevision;
}
} // namespace app::doc

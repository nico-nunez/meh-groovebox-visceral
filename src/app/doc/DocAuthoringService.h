#pragma once

#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocSequencerModel.h"

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
  ApplyStatus status = ApplyStatus::Idle;
  DocDiagnostics diagnostics{};
  AuthoredSeqDocModel lastAdmittedSeqModel{};
  bool hasLastAdmittedSequencerModel = false;
};

struct DocAuthoringService {
  DocBufferState buffer{};
  DocApplyState apply{};
};

void initDocAuthoringService(DocAuthoringService& service);

ApplyRevisionResult applySequencerRevision(DocAuthoringService& service,
                                           app::AppContext& app,
                                           DocRevision revision,
                                           const char* bufferText);
} // namespace app::doc

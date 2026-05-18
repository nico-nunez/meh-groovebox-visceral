#pragma once

#include "app/GrooveboxTargetState.h"
#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocTypes.h"

namespace app::doc {

struct GrooveboxTargetBuildResult {
  bool ok = false;
  DocDiagnostics diagnostics{};
};

GrooveboxTargetBuildResult buildGrooveboxTargetState(const AuthoredDocModel* model,
                                                     DocID documentID,
                                                     DocRevision revision,
                                                     app::GrooveboxTargetState* out);

} // namespace app::doc

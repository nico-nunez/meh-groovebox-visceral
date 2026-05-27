#pragma once

#include "app/GrooveboxPatch.h"
#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocTypes.h"

namespace app::doc {

struct GrooveboxTargetBuildResult {
  bool ok = false;
  DocDiagnostics diagnostics{};
};

GrooveboxTargetBuildResult buildGrooveboxPatch(const AuthoredDocModel* model,
                                               DocID documentID,
                                               DocRevision revision,
                                               app::GrooveboxPatch* out);

} // namespace app::doc

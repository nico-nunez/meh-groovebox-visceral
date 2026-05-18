#pragma once

#include "app/Constants.h"
#include "app/GrooveboxTargetState.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocTypes.h"

#include <atomic>

namespace app {

struct AppContext;

struct PendingGrooveboxApply {
  bool synthPrepared[MAX_TRACKS]{};

  doc::DocRevision revision = 0;

  bool mixerPrepared = false;
  bool sequencerPrepared = false;

  std::atomic<bool> ready{false};
  std::atomic<bool> writeInFlight{false};
};

struct GrooveboxEditSession {
  doc::DocRevision revision = 0;
  const GrooveboxTargetState* target = nullptr;
};

struct GrooveboxEditResult {
  bool ok = false;
};

void beginGrooveboxEdit(GrooveboxEditSession* session, doc::DocRevision revision);
void stageGrooveboxTarget(GrooveboxEditSession* session, const GrooveboxTargetState* target);
void abortGrooveboxEdit(GrooveboxEditSession* session, AppContext* app);

GrooveboxEditResult commitGrooveboxEditImmediate(GrooveboxEditSession* session,
                                                 AppContext* app,
                                                 doc::DocDiagnostics* diagnostics);

void publishPendingGrooveboxEditIfReady(AppContext* app);

} // namespace app

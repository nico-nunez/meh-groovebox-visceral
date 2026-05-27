#pragma once

#include "app/Constants.h"
#include "app/GrooveboxPatch.h"
#include "app/Transport.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocTypes.h"

#include <atomic>

namespace app {

struct AppContext;

enum class GrooveboxApplyTiming : uint8_t {
  Immediate = 0,
  NextBeat,
  NextBar,
};

struct PendingGrooveboxApply {
  bool synthPrepared[MAX_TRACKS]{};

  doc::DocRevision revision = 0;
  GrooveboxApplyTiming timing = GrooveboxApplyTiming::Immediate;

  bool mixerPrepared = false;
  bool sequencerPrepared = false;

  std::atomic<bool> ready{false};
  std::atomic<bool> writeInFlight{false};
};

struct GrooveboxEditSession {
  doc::DocRevision revision = 0;
  const GrooveboxPatch* patch = nullptr;
};

struct GrooveboxEditResult {
  bool ok = false;
};

void beginGrooveboxEdit(GrooveboxEditSession* session, doc::DocRevision revision);
void abortGrooveboxEdit(GrooveboxEditSession* session, AppContext* app);

void stageGrooveboxPatch(GrooveboxEditSession* session, const GrooveboxPatch* patch);

GrooveboxEditResult commitGrooveboxEdit(GrooveboxEditSession* session,
                                        AppContext* app,
                                        GrooveboxApplyTiming timing,
                                        doc::DocDiagnostics* diagnostics);

void publishPendingGrooveboxEditIfReady(AppContext* app,
                                        const transport::TransportBlockInfo& blockInfo);

inline GrooveboxEditResult commitGrooveboxEditImmediate(GrooveboxEditSession* session,
                                                        AppContext* app,
                                                        doc::DocDiagnostics* diagnostics) {
  return commitGrooveboxEdit(session, app, GrooveboxApplyTiming::Immediate, diagnostics);
}

} // namespace app

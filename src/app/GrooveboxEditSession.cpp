#include "app/GrooveboxEditSession.h"

#include "app/AppContext.h"
#include "app/Sequencer.h"
#include "app/doc/DocMetadata.h"

#include "synth/program/SynthProgram.h"

#include <cassert>
#include <cstdint>

namespace app {

namespace {

bool shouldPublishPendingGrooveboxApply(GrooveboxApplyTiming timing,
                                        const transport::TransportBlockInfo& blockInfo) {
  using transport::BoundaryFlags;
  using transport::TransportMode;

  if (timing == GrooveboxApplyTiming::Immediate)
    return true;

  if (blockInfo.mode == TransportMode::Stopped)
    return true;

  if (blockInfo.mode == TransportMode::Paused)
    return false;

  if (timing == GrooveboxApplyTiming::NextBeat) {
    return (blockInfo.boundaryFlags & static_cast<uint32_t>(BoundaryFlags::CrossedBeat)) != 0;
  }

  if (timing == GrooveboxApplyTiming::NextBar) {
    return (blockInfo.boundaryFlags & static_cast<uint32_t>(BoundaryFlags::CrossedBar)) != 0;
  }

  return false;
}

doc::DocDiagnostic makeEditDiagnostic(doc::DocRevision revision,
                                      const char* code,
                                      const char* message) {
  doc::DocDiagnostic d{};
  d.severity = doc::DiagnosticSeverity::Error;
  d.source = doc::DiagnosticSource::GrooveboxAdmission;
  d.revision = revision;
  d.code = code ? code : doc::docdiag::InternalPlannerError;
  d.message = message ? message : "groovebox edit failed";
  return d;
}

void clearPendingPreparedFlags(PendingGrooveboxApply* pending) {
  pending->revision = 0;
  pending->timing = GrooveboxApplyTiming::Immediate;
  for (bool& prepared : pending->synthPrepared)
    prepared = false;
  pending->mixerPrepared = false;
  pending->sequencerPrepared = false;
}

void abortPreparedSynth(AppContext* app, const PendingGrooveboxApply& pending) {
  for (uint8_t t = 0; t < MAX_TRACKS; ++t) {
    if (pending.synthPrepared[t])
      synth::program::abortProgramSwap(app->tracks[t].engine);
  }
}

void abortPreparedSequencer(AppContext* app, const PendingGrooveboxApply& pending) {
  if (pending.sequencerPrepared)
    sequencer::abortSequencerSnapshotSwap(app->sequencer);
}

void abortPreparedMixer(AppContext* app, const PendingGrooveboxApply& pending) {
  if (pending.mixerPrepared)
    mixer::abortMixerSnapshotSwap(app->mixer);
}

bool acquirePendingWriter(PendingGrooveboxApply* pending) {
  bool expected = false;
  return pending->writeInFlight.compare_exchange_strong(expected,
                                                        true,
                                                        std::memory_order_acq_rel,
                                                        std::memory_order_acquire);
}

void releasePendingWriter(PendingGrooveboxApply* pending) {
  pending->writeInFlight.store(false, std::memory_order_release);
}

} // namespace

void beginGrooveboxEdit(GrooveboxEditSession* session, doc::DocRevision revision) {
  *session = GrooveboxEditSession{};
  session->revision = revision;
}

void stageGrooveboxTarget(GrooveboxEditSession* session, const GrooveboxTargetState* target) {
  session->target = target;
}

void abortGrooveboxEdit(GrooveboxEditSession* session, AppContext* app) {
  if (app) {
    auto& pending = app->pendingGrooveboxApply;
    abortPreparedSynth(app, pending);
    abortPreparedMixer(app, pending);
    abortPreparedSequencer(app, pending);
    clearPendingPreparedFlags(&pending);
    pending.ready.store(false, std::memory_order_release);
    releasePendingWriter(&pending);
  }
  if (session)
    *session = GrooveboxEditSession{};
}

GrooveboxEditResult commitGrooveboxEdit(GrooveboxEditSession* session,
                                        AppContext* app,
                                        GrooveboxApplyTiming timing,
                                        doc::DocDiagnostics* diagnostics) {
  GrooveboxEditResult result{};
  if (!session || !app || !diagnostics || !session->target) {
    if (diagnostics)
      diagnostics->push_back(
          makeEditDiagnostic(0, doc::docdiag::InternalPlannerError, "invalid edit session"));
    return result;
  }

  auto& pending = app->pendingGrooveboxApply;
  if (!acquirePendingWriter(&pending)) {
    diagnostics->push_back(makeEditDiagnostic(session->revision,
                                              doc::docdiag::InternalPlannerError,
                                              "groovebox edit already in flight"));
    return result;
  }

  if (pending.ready.load(std::memory_order_acquire)) {
    releasePendingWriter(&pending);
    diagnostics->push_back(makeEditDiagnostic(session->revision,
                                              doc::docdiag::InternalPlannerError,
                                              "pending groovebox edit has not published"));
    return result;
  }

  clearPendingPreparedFlags(&pending);
  pending.revision = session->revision;
  pending.timing = timing;

  const GrooveboxTargetState& target = *session->target;

  // Prepare synth engine write buffers. This is non-observable until the audio
  // callback commits and publishes them.
  for (uint8_t t = 0; t < MAX_TRACKS; ++t) {
    if (!target.hasSynthProgram[t])
      continue;

    auto prepare =
        synth::program::prepareProgramSwap(app->tracks[t].engine, target.synthPrograms[t]);
    if (!prepare.ok) {
      diagnostics->push_back(
          makeEditDiagnostic(session->revision, doc::docdiag::SynthAdmissionFailed, prepare.err));
      abortGrooveboxEdit(session, app);
      return result;
    }
    pending.synthPrepared[t] = true;
  }

  // Prepare mixer pending buffer. This is non-observable until audio publish.
  if (target.hasMixer) {
    auto prepare = mixer::prepareMixerSnapshotSwap(app->mixer, target.mixer);
    if (!prepare.ok) {
      diagnostics->push_back(
          makeEditDiagnostic(session->revision, doc::docdiag::MixerAdmissionFailed, prepare.err));
      abortGrooveboxEdit(session, app);
      return result;
    }
    pending.mixerPrepared = true;
  }

  // Validate and prepare sequencer write buffer. Validation is separate from
  // inactive-buffer preparation so commit cannot discover structural errors.
  if (target.hasSequencer) {
    auto validate = sequencer::validatePatternSnapshot(target.sequencer);
    if (!validate.ok) {
      diagnostics->push_back(makeEditDiagnostic(session->revision,
                                                doc::docdiag::SequencerAdmissionFailed,
                                                validate.err));
      abortGrooveboxEdit(session, app);
      return result;
    }

    auto prepare = sequencer::prepareSequencerSnapshotSwap(app->sequencer, target.sequencer);
    if (!prepare.ok) {
      diagnostics->push_back(makeEditDiagnostic(session->revision,
                                                doc::docdiag::SequencerAdmissionFailed,
                                                prepare.err));
      abortGrooveboxEdit(session, app);
      return result;
    }
    pending.sequencerPrepared = true;
  }

  pending.ready.store(true, std::memory_order_release);
  releasePendingWriter(&pending);
  result.ok = true;
  return result;
}

void publishPendingGrooveboxEditIfReady(AppContext* app,
                                        const transport::TransportBlockInfo& blockInfo) {
  if (!app)
    return;

  auto& pending = app->pendingGrooveboxApply;
  if (!pending.ready.load(std::memory_order_acquire))
    return;

  if (!shouldPublishPendingGrooveboxApply(pending.timing, blockInfo))
    return;

  // Synth: commit and publish prepared per-engine program swaps inside this one
  // audio callback, so synth cannot advance ahead of mixer/sequencer.
  for (uint8_t t = 0; t < MAX_TRACKS; ++t) {
    if (!pending.synthPrepared[t])
      continue;

    auto commit = synth::program::commitProgramSwap(app->tracks[t].engine);
    assert(commit.ok);
    synth::program::publishPendingProgramIfReady(app->tracks[t].engine);
  }

  if (pending.mixerPrepared) {
    auto commit = mixer::commitMixerSnapshotSwap(app->mixer);
    assert(commit.ok);
    mixer::publishPendingMixerSnapshotIfReady(app->mixer);
  }

  if (pending.sequencerPrepared) {
    auto commit = sequencer::commitSequencerSnapshotSwap(app->sequencer);
    assert(commit.ok);
    sequencer::publishPendingSequencerSnapshotIfReady(app->sequencer);
  }

  clearPendingPreparedFlags(&pending);
  pending.ready.store(false, std::memory_order_release);
}

} // namespace app

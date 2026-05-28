#include "app/GrooveboxEditSession.h"

#include "app/AppContext.h"
#include "app/Sequencer.h"
#include "app/doc/metadata/DocMetadata.h"

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

void captureTrackControlProgram(const track::TrackState& track, synth::program::SynthProgram* out) {
  *out = track.controlProgram;
}

void applySynthParamPatch(synth::program::SynthProgram* program,
                          synth::param::ParamID paramID,
                          float value) {
  program->paramValues[static_cast<int>(paramID)] = value;
}

void commitTrackControlProgram(track::TrackState* track,
                               const synth::program::SynthProgram& program) {
  track->controlProgram = program;
  track->controlProgramValid = true;
}
bool applyMixerPatchWrite(mixer::MixerSnapshot* snapshot,
                          const MixerParamPatchWrite& write,
                          doc::DocRevision revision,
                          doc::DocDiagnostics* diagnostics) {
  const float value = app::params::clampAppParam(write.paramID, write.value);

  switch (write.paramID) {
  case app::params::AppParamID::TrackGain:
    snapshot->tracks[write.trackIndex].gain = value;
    return true;
  case app::params::AppParamID::TrackPan:
    snapshot->tracks[write.trackIndex].pan = value;
    return true;
  case app::params::AppParamID::TrackMute:
    snapshot->tracks[write.trackIndex].enabled = value < 0.5f;
    return true;
  case app::params::AppParamID::MasterGain:
    snapshot->masterGain = value;
    return true;
  case app::params::AppParamID::LimiterThresholdDB:
    snapshot->limiterThreshold = dsp::math::dBToLinear(value);
    return true;
  case app::params::AppParamID::Count:
    break;
  }

  diagnostics->push_back(makeEditDiagnostic(revision,
                                            doc::docdiag::MixerAdmissionFailed,
                                            "unhandled mixer patch param"));
  return false;
}

bool composeMixerPatch(const MixerPatch& patch,
                       const mixer::MixerSnapshot& current,
                       mixer::MixerSnapshot* out,
                       doc::DocRevision revision,
                       doc::DocDiagnostics* diagnostics) {
  *out = current;
  for (uint16_t i = 0; i < patch.writeCount; ++i) {
    if (!applyMixerPatchWrite(out, patch.writes[i], revision, diagnostics))
      return false;
  }
  return true;
}

void applyStepPatch(sequencer::StepEvent* step, const StepEventPatch& patch) {
  if (patch.op == PatchObjectOp::Clear) {
    sequencer::resetStepEvent(step);
    return;
  }

  if (patch.hasActive) {
    step->active = patch.active;
    step->noteOn = patch.active;
  }
  if (patch.hasNoteOn)
    step->noteOn = patch.noteOn;
  if (patch.hasLegato)
    step->legato = patch.legato;
  if (patch.hasGate)
    step->gate = patch.gate;
  if (patch.hasNote)
    step->note = patch.note;
  if (patch.hasVelocity)
    step->velocity = patch.velocity;

  if (patch.locks.op == PatchObjectOp::Clear) {
    step->numLocks = 0;
  } else if (patch.locks.op == PatchObjectOp::Replace) {
    step->numLocks = patch.locks.lockCount;
    for (uint8_t i = 0; i < patch.locks.lockCount; ++i)
      step->locks[i] = patch.locks.locks[i];
  }
}

void applyPatternPatch(sequencer::LanePattern* pattern, const PatternPatch& patch) {
  if (patch.op == PatchObjectOp::Clear) {
    sequencer::resetLanePattern(pattern);
    return;
  }

  if (patch.hasNumSteps) {
    if (patch.numSteps < pattern->numSteps) {
      for (uint8_t step = patch.numSteps; step < sequencer::MAX_PATTERN_STEPS; ++step)
        sequencer::resetStepEvent(&pattern->steps[step]);
    }
    pattern->numSteps = patch.numSteps;
  }

  if (patch.hasStepsPerBeat)
    pattern->stepsPerBeat = patch.stepsPerBeat;

  for (uint8_t step = 0; step < sequencer::MAX_PATTERN_STEPS; ++step) {
    if (patch.hasStep[step])
      applyStepPatch(&pattern->steps[step], patch.steps[step]);
  }
}

bool composeSequencerPatch(const SequencerPatch& patch,
                           const sequencer::PatternSnapshot& current,
                           sequencer::PatternSnapshot* out,
                           doc::DocRevision revision,
                           doc::DocDiagnostics* diagnostics) {
  *out = current;

  for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
    if (!patch.hasTrack[track])
      continue;

    const TrackSequencerPatch& trackPatch = patch.tracks[track];
    sequencer::PatternBank& bank = out->lanes[track];

    if (trackPatch.bankOp == PatchObjectOp::Clear)
      sequencer::resetPatternBank(&bank);

    for (uint8_t slot = 0; slot < sequencer::PATTERNS_PER_LANE; ++slot) {
      if (!trackPatch.hasSlot[slot])
        continue;

      const PatternSlotPatch& slotPatch = trackPatch.slots[slot];
      if (slotPatch.op == PatchObjectOp::Clear) {
        bank.slots[slot].occupied = false;
        sequencer::resetLanePattern(&bank.slots[slot].pattern);
        if (bank.activeSlot == slot)
          bank.activeSlot = sequencer::INVALID_PATTERN_SLOT;
        continue;
      }

      bank.slots[slot].occupied = true;
      applyPatternPatch(&bank.slots[slot].pattern, slotPatch.pattern);
      if (bank.activeSlot == sequencer::INVALID_PATTERN_SLOT)
        bank.activeSlot = slot;
    }

    if (trackPatch.hasActiveSlot)
      bank.activeSlot = trackPatch.activeSlot;
  }

  auto validate = sequencer::validatePatternSnapshot(*out);
  if (!validate.ok) {
    diagnostics->push_back(
        makeEditDiagnostic(revision, doc::docdiag::SequencerAdmissionFailed, validate.err));
    return false;
  }

  return true;
}

bool composeSynthPatch(track::TrackState& track,
                       const TrackSynthPatch& patch,
                       synth::program::SynthProgram* out,
                       doc::DocRevision revision,
                       doc::DocDiagnostics* diagnostics) {
  if (!track.controlProgramValid) {
    diagnostics->push_back(makeEditDiagnostic(revision,
                                              doc::docdiag::SynthAdmissionFailed,
                                              "missing track control program"));
    return false;
  }

  captureTrackControlProgram(track, out);

  for (uint16_t i = 0; i < patch.writeCount; ++i) {
    const SynthParamPatchWrite& write = patch.writes[i];
    if (write.paramID == synth::param::PARAM_UNKNOWN ||
        static_cast<int>(write.paramID) >= synth::param::PARAM_COUNT) {
      diagnostics->push_back(makeEditDiagnostic(revision,
                                                doc::docdiag::SynthAdmissionFailed,
                                                "invalid synth patch param"));
      return false;
    }
    applySynthParamPatch(out, write.paramID, write.value);
  }

  return true;
}

GrooveboxEditResult commitGrooveboxPatchEdit(GrooveboxEditSession* session,
                                             AppContext* app,
                                             GrooveboxApplyTiming timing,
                                             doc::DocDiagnostics* diagnostics) {
  GrooveboxEditResult result{};
  auto& pending = app->documents.pendingApply;
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

  const GrooveboxPatch& patch = *session->patch;
  if (!hasGrooveboxPatchEdits(patch)) {
    clearPendingPreparedFlags(&pending);
    releasePendingWriter(&pending);
    result.ok = true;
    return result;
  }

  synth::program::SynthProgram synthPrograms[MAX_TRACKS]{};
  for (uint8_t t = 0; t < MAX_TRACKS; ++t) {
    if (!patch.hasSynth[t])
      continue;
    if (!composeSynthPatch(app->tracks[t],
                           patch.synth[t],
                           &synthPrograms[t],
                           session->revision,
                           diagnostics)) {
      abortGrooveboxEdit(session, app);
      return result;
    }
    auto prepare = synth::program::prepareProgramSwap(app->tracks[t].engine, synthPrograms[t]);
    if (!prepare.ok) {
      diagnostics->push_back(
          makeEditDiagnostic(session->revision, doc::docdiag::SynthAdmissionFailed, prepare.err));
      abortGrooveboxEdit(session, app);
      return result;
    }
    pending.synthPrepared[t] = true;
  }

  if (patch.hasMixer) {
    mixer::MixerSnapshot mixerSnapshot{};
    if (!composeMixerPatch(patch.mixer,
                           app->mixer.current,
                           &mixerSnapshot,
                           session->revision,
                           diagnostics)) {
      abortGrooveboxEdit(session, app);
      return result;
    }
    auto prepare = mixer::prepareMixerSnapshotSwap(app->mixer, mixerSnapshot);
    if (!prepare.ok) {
      diagnostics->push_back(
          makeEditDiagnostic(session->revision, doc::docdiag::MixerAdmissionFailed, prepare.err));
      abortGrooveboxEdit(session, app);
      return result;
    }
    pending.mixerPrepared = true;
  }

  if (patch.hasSequencer) {
    sequencer::PatternSnapshot seqSnapshot{};
    const sequencer::PatternSnapshot& current = app::sequencer::getPatternSnapshot(app->sequencer);
    if (!composeSequencerPatch(patch.sequencer,
                               current,
                               &seqSnapshot,
                               session->revision,
                               diagnostics)) {
      abortGrooveboxEdit(session, app);
      return result;
    }
    auto prepare = sequencer::prepareSequencerSnapshotSwap(app->sequencer, seqSnapshot);
    if (!prepare.ok) {
      diagnostics->push_back(makeEditDiagnostic(session->revision,
                                                doc::docdiag::SequencerAdmissionFailed,
                                                prepare.err));
      abortGrooveboxEdit(session, app);
      return result;
    }
    pending.sequencerPrepared = true;
  }

  for (uint8_t t = 0; t < MAX_TRACKS; ++t) {
    if (pending.synthPrepared[t])
      commitTrackControlProgram(&app->tracks[t], synthPrograms[t]);
  }

  pending.ready.store(true, std::memory_order_release);
  releasePendingWriter(&pending);
  result.ok = true;
  return result;
}

} // namespace

void beginGrooveboxEdit(GrooveboxEditSession* session, doc::DocRevision revision) {
  *session = GrooveboxEditSession{};
  session->revision = revision;
}

void stageGrooveboxPatch(GrooveboxEditSession* session, const GrooveboxPatch* patch) {
  session->patch = patch;
}
void abortGrooveboxEdit(GrooveboxEditSession* session, AppContext* app) {
  if (app) {
    auto& pending = app->documents.pendingApply;
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
  if (!session || !app || !diagnostics || !session->patch) {
    if (diagnostics)
      diagnostics->push_back(
          makeEditDiagnostic(0, doc::docdiag::InternalPlannerError, "invalid edit session"));
    return result;
  }

  return commitGrooveboxPatchEdit(session, app, timing, diagnostics);
}

void publishPendingGrooveboxEditIfReady(AppContext* app,
                                        const transport::TransportBlockInfo& blockInfo) {
  if (!app)
    return;

  auto& pending = app->documents.pendingApply;
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

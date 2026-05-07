#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/Sequencer.h"
#include "app/doc/DocAuthoringService.h"

#include <cstdio>

namespace {

bool hasDiagnostic(const app::doc::DocDiagnostics& diagnostics, const char* code) {
  for (const auto& d : diagnostics) {
    if (d.code == code)
      return true;
  }
  return false;
}

const char* kNonEmptyTrack1 =
    "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
    "steps = { { active = true, note = 60, velocity = 100 } } } }, activeSlot = 1 })";

const char* kChangedTrack1 =
    "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
    "steps = { { active = true, note = 61, velocity = 100 } } } }, activeSlot = 1 })";

bool popParam(app::AppContext& app, uint8_t trackIndex, synth::ParamEvent& out) {
  return app.tracks[trackIndex].queues.param.pop(out);
}

const app::doc::AuthoredSynthParamWrite*
findAdmittedWrite(const app::doc::AuthoredTrackSynthPatch& patch, synth::param::ParamID id) {
  for (const auto& write : patch.writes) {
    if (write.paramID == id)
      return &write;
  }
  return nullptr;
}

} // namespace

static void test_parse_failure_sets_failed_without_admission() {
  TEST("parse_failure_sets_failed_without_admission");
  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(service, app, 1, "track('one', TrackSettings {})");
  CHECK("not ok", !result.ok);
  CHECK("status Failed", service.apply.status == app::doc::ApplyStatus::Failed);
  CHECK("diagnostic sequencer.track.invalid_index",
        hasDiagnostic(result.diagnostics, "sequencer.track.invalid_index"));
  CHECK("no admitted model", !service.apply.hasLastAdmittedDocModel);
  CHECK("not editing", !app.sequencer.isEditing);
}

static void test_successful_non_empty_apply_admits_and_commits() {
  TEST("successful_non_empty_apply_admits_and_commits");
  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(service, app, 1, kNonEmptyTrack1);
  CHECK("ok", result.ok);
  CHECK("status Completed", service.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("lastAdmittedRevision == 1", service.buffer.lastAdmittedRevision == 1);
  CHECK("has admitted model", service.apply.hasLastAdmittedDocModel);
  CHECK("admitted track 0 present", service.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
  CHECK("admitted slot 0 occupied",
        service.apply.lastAdmittedDocModel.sequencer.tracks[0].patterns[0].occupied);
  CHECK("not editing", !app.sequencer.isEditing);
}

static void test_successful_no_op_apply_does_not_require_staging() {
  TEST("successful_no_op_apply_does_not_require_staging");
  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  app::doc::applySequencerRevision(service, app, 1, kNonEmptyTrack1);

  // Simulate an unrelated runtime edit session
  app.sequencer.isEditing = true;

  // Empty text produces a valid no-op revision (no track calls)
  auto result = app::doc::applySequencerRevision(service, app, 2, "");
  CHECK("ok", result.ok);
  CHECK("status Completed", service.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("unrelated isEditing still true", app.sequencer.isEditing);
  CHECK("admitted revision == 2", service.apply.lastAdmittedDocModel.revision == 2);
  CHECK("track 0 preserved", service.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
  CHECK("track 0 slot 0 still occupied",
        service.apply.lastAdmittedDocModel.sequencer.tracks[0].patterns[0].occupied);
}

static void test_omitted_track_preserves_previous_admitted_target() {
  TEST("omitted_track_preserves_previous_admitted_target");
  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  // Admit track 1 (index 0)
  app::doc::applySequencerRevision(service, app, 1, kNonEmptyTrack1);

  // Admit only track 2 (index 1) as explicit empty
  auto result = app::doc::applySequencerRevision(service, app, 2, "track(2, TrackSettings {})");
  CHECK("ok", result.ok);
  CHECK("status Completed", service.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("track 0 still present", service.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
  CHECK("track 1 present", service.apply.lastAdmittedDocModel.sequencer.hasTrackState[1]);
  CHECK("track 0 slot 0 preserved",
        service.apply.lastAdmittedDocModel.sequencer.tracks[0].patterns[0].occupied);
  CHECK("track 1 explicitlyAuthoredEmpty",
        service.apply.lastAdmittedDocModel.sequencer.tracks[1].explicitlyAuthoredEmpty);
}

static void test_begin_failure_preserves_previous_admitted_target() {
  TEST("begin_failure_preserves_previous_admitted_target");
  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  app::doc::applySequencerRevision(service, app, 1, kNonEmptyTrack1);

  // Start an unrelated edit session so the next beginPatternEdit call fails
  app::sequencer::beginPatternEdit(app.sequencer, true);

  // Submit a changed revision — plan is non-empty, so submitSequencerPlan
  // will try beginPatternEdit and fail
  auto result = app::doc::applySequencerRevision(service, app, 2, kChangedTrack1);
  CHECK("not ok", !result.ok);
  CHECK("diagnostic sequencer.admission_failed",
        hasDiagnostic(result.diagnostics, "sequencer.admission_failed"));
  CHECK("admitted revision still 1", service.apply.lastAdmittedDocModel.revision == 1);
  CHECK("unrelated isEditing still true", app.sequencer.isEditing);
}

static void test_abort_pattern_edit_clears_staging_without_publishing() {
  TEST("abort_pattern_edit_clears_staging_without_publishing");
  app::AppContext app{};

  // Confirm the default read buffer is empty for lane 0
  auto preBankResult = app::sequencer::getPatternBank(app.sequencer, 0);
  CHECK("pre-edit bank readable", preBankResult.ok);
  CHECK("pre-edit slot 0 not occupied", !preBankResult.value->slots[0].occupied);

  // Begin, mutate write buffer, then abort without committing
  app::sequencer::beginPatternEdit(app.sequencer, true);

  app::sequencer::PatternBank mutatedBank{};
  mutatedBank.slots[0].occupied = true;
  mutatedBank.slots[0].pattern.numSteps = 1;
  mutatedBank.slots[0].pattern.stepsPerBeat = 4;
  mutatedBank.activeSlot = 0;
  app::sequencer::replacePatternBank(app.sequencer, 0, mutatedBank);

  auto abortResult = app::sequencer::abortPatternEdit(app.sequencer);
  CHECK("abort ok", abortResult.ok);
  CHECK("not editing", !app.sequencer.isEditing);

  // Read buffer must still reflect the pre-edit state
  auto postBankResult = app::sequencer::getPatternBank(app.sequencer, 0);
  CHECK("post-abort bank readable", postBankResult.ok);
  CHECK("slot 0 not occupied in read buffer", !postBankResult.value->slots[0].occupied);
  CHECK("activeSlot is INVALID",
        postBankResult.value->activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
}

static void test_synth_apply_queues_param_events() {
  TEST("synth_apply_queues_param_events");

  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(
      service,
      app,
      1,
      "synth(1, SynthSettings { osc1 = { mix = 0.8 }, svf = { cutoff = 1200 } })");

  CHECK("ok", result.ok);
  CHECK("status Completed", service.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("has admitted doc model", service.apply.hasLastAdmittedDocModel);
  CHECK("track 0 synth admitted", service.apply.lastAdmittedDocModel.hasSynthState[0]);

  synth::ParamEvent first{};
  synth::ParamEvent second{};
  CHECK("pop first", popParam(app, 0, first));
  CHECK("pop second", popParam(app, 0, second));
  CHECK("no third", !popParam(app, 0, second));

  const bool hasMix = (first.id == synth::param::OSC1_MIX_LEVEL && first.value == 0.8f) ||
                      (second.id == synth::param::OSC1_MIX_LEVEL && second.value == 0.8f);
  const bool hasCutoff = (first.id == synth::param::SVF_CUTOFF && first.value == 1200.0f) ||
                         (second.id == synth::param::SVF_CUTOFF && second.value == 1200.0f);

  CHECK("mix queued", hasMix);
  CHECK("cutoff queued", hasCutoff);
}

static void test_synth_noop_apply_queues_no_param_events() {
  TEST("synth_noop_apply_queues_no_param_events");

  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  app::doc::applySequencerRevision(service,
                                   app,
                                   1,
                                   "synth(1, SynthSettings { osc1 = { mix = 0.8 } })");

  synth::ParamEvent event{};
  CHECK("drain first", popParam(app, 0, event));
  CHECK("queue drained", !popParam(app, 0, event));

  auto result =
      app::doc::applySequencerRevision(service,
                                       app,
                                       2,
                                       "synth(1, SynthSettings { osc1 = { mix = 0.8 } })");

  CHECK("ok", result.ok);
  CHECK("no new event", !popParam(app, 0, event));
}

static void test_synth_patch_preserves_omitted_params_in_admitted_target() {
  TEST("synth_patch_preserves_omitted_params_in_admitted_target");

  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  app::doc::applySequencerRevision(
      service,
      app,
      1,
      "synth(1, SynthSettings { osc1 = { mix = 0.8 }, svf = { cutoff = 1200 } })");

  auto result =
      app::doc::applySequencerRevision(service,
                                       app,
                                       2,
                                       "synth(1, SynthSettings { osc1 = { mix = 0.25 } })");

  CHECK("ok", result.ok);

  const auto& patch = service.apply.lastAdmittedDocModel.synthTracks[0];
  const auto* mix = findAdmittedWrite(patch, synth::param::OSC1_MIX_LEVEL);
  const auto* cutoff = findAdmittedWrite(patch, synth::param::SVF_CUTOFF);

  CHECK("mix updated", mix && mix->value == 0.25f);
  CHECK("cutoff preserved", cutoff && cutoff->value == 1200.0f);
}

static void test_mixed_synth_and_sequencer_apply_succeeds() {
  TEST("mixed_synth_and_sequencer_apply_succeeds");

  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(
      service,
      app,
      1,
      "track(1, TrackSettings { synth = SynthSettings { osc1 = { mix = 0.5 } }, "
      "patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
      "steps = { { active = true, note = 60 } } } }, activeSlot = 1 })");

  CHECK("ok", result.ok);
  CHECK("seq admitted", service.apply.hasLastAdmittedDocModel);
  CHECK("doc admitted", service.apply.hasLastAdmittedDocModel);
  CHECK("pattern occupied",
        service.apply.lastAdmittedDocModel.sequencer.tracks[0].patterns[0].occupied);

  synth::ParamEvent event{};
  CHECK("synth event queued", popParam(app, 0, event));
  CHECK("mix event", event.id == synth::param::OSC1_MIX_LEVEL && event.value == 0.5f);
}

static void test_synth_queue_full_fails_apply() {
  TEST("synth_queue_full_fails_apply");

  app::doc::DocAuthoringService service{};
  app::AppContext app{};

  synth::ParamEvent filler{};
  for (std::size_t i = 0; i < synth::events::ParamEventQueue::SIZE - 1; ++i)
    CHECK("fill queue", app.tracks[0].queues.param.push(filler));

  auto result =
      app::doc::applySequencerRevision(service,
                                       app,
                                       1,
                                       "synth(1, SynthSettings { osc1 = { mix = 0.8 } })");

  CHECK("not ok", !result.ok);
  CHECK("diagnostic synth.admission_failed",
        hasDiagnostic(result.diagnostics, app::doc::docdiag::SynthAdmissionFailed));
  CHECK("no admitted doc model", !service.apply.hasLastAdmittedDocModel);
}

void runDocAuthoringServiceSeqApplyTests() {
  SUITE("DocAuthoringService / SequencerApply");
  test_parse_failure_sets_failed_without_admission();
  test_successful_non_empty_apply_admits_and_commits();
  test_successful_no_op_apply_does_not_require_staging();
  test_omitted_track_preserves_previous_admitted_target();
  test_begin_failure_preserves_previous_admitted_target();
  test_abort_pattern_edit_clears_staging_without_publishing();
  test_parse_failure_sets_failed_without_admission();
  test_synth_apply_queues_param_events();
  test_synth_noop_apply_queues_no_param_events();
  test_synth_patch_preserves_omitted_params_in_admitted_target();
  test_mixed_synth_and_sequencer_apply_succeeds();
  test_synth_queue_full_fails_apply(); // if optional test included
  test_successful_non_empty_apply_admits_and_commits();
}

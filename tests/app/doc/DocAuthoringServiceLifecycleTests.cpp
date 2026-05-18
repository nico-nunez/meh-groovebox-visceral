#include "TestRunner.h"
#include "TestHelpers.h"

#include "app/AppContext.h"
#include "app/doc/DocAuthoringService.h"

#include <cstdio>
#include <cstring>

namespace {

using test::hasDiagnostic;

static constexpr const char* kOneTrackDocument =
    "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
    "steps = { { active = true, note = 60, velocity = 100 } } } }, activeSlot = 1 })";

static constexpr const char* kTempFilePath = "/tmp/doc_lifecycle_test.lua";

} // namespace

static void test_successful_apply_completes_lifecycle() {
  TEST("successful_apply_completes_lifecycle");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(service, app, 1, kOneTrackDocument);
  CHECK("ok", result.ok);
  CHECK("applyOperationID == 1", result.applyOperationID == 1);
  CHECK("status Completed", app::doc::getApplyStatus(service) == app::doc::ApplyStatus::Completed);
  CHECK("activeApplyOperationID == 0", service.apply.activeApplyOperationID == 0);
  CHECK("lastAdmittedRevision == 1", app::doc::getLastAdmittedRevision(service) == 1);
  CHECK("admitted model has track 0",
        service.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
  app::doc::destroyDocAuthoringService(service);
}

static void test_parse_failure_fails_lifecycle() {
  TEST("parse_failure_fails_lifecycle");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(service, app, 1, "track('bad', TrackSettings {})");
  CHECK("not ok", !result.ok);
  CHECK("status Failed", app::doc::getApplyStatus(service) == app::doc::ApplyStatus::Failed);
  CHECK("activeApplyOperationID == 0", service.apply.activeApplyOperationID == 0);
  CHECK("diagnostic sequencer.track.invalid_index",
        hasDiagnostic(result.diagnostics, "sequencer.track.invalid_index"));
  CHECK("no admitted model", !service.apply.hasLastAdmittedDocModel);
  app::doc::destroyDocAuthoringService(service);
}

static void test_supersedes_existing_active_operation() {
  TEST("supersedes_existing_active_operation");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  service.apply.activeApplyOperationID = 41;
  service.apply.status = app::doc::ApplyStatus::Started;

  auto result = app::doc::applySequencerRevision(service, app, 1, kOneTrackDocument);
  CHECK("ok", result.ok);
  CHECK("applyOperationID == 1", result.applyOperationID == 1);
  CHECK("status Completed", app::doc::getApplyStatus(service) == app::doc::ApplyStatus::Completed);
  CHECK("active operation cleared", service.apply.activeApplyOperationID == 0);
  CHECK("lastSupersededApplyOperationID == 41", service.apply.lastSupersededApplyOperationID == 41);
  app::doc::destroyDocAuthoringService(service);
}

static void test_file_apply_success_uses_buffer_pipeline() {
  TEST("file_apply_success_uses_buffer_pipeline");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  FILE* f = fopen(kTempFilePath, "w");
  fputs(kOneTrackDocument, f);
  fclose(f);

  auto result = app::doc::applySequencerFile(service, app, kTempFilePath);
  CHECK("ok", result.ok);
  CHECK("status Completed", app::doc::getApplyStatus(service) == app::doc::ApplyStatus::Completed);
  CHECK("buffer.path == tmpPath", service.buffer.path == kTempFilePath);
  CHECK("buffer.currentRevision == 1", service.buffer.currentRevision == 1);
  CHECK("lastAdmittedRevision == 1", app::doc::getLastAdmittedRevision(service) == 1);
  CHECK("admitted model has track 0",
        service.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
  app::doc::destroyDocAuthoringService(service);
}

static void test_file_read_failure_is_failed_apply_operation() {
  TEST("file_read_failure_is_failed_apply_operation");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result = app::doc::applySequencerFile(service, app, "/path/that/does/not/exist.lua");
  CHECK("not ok", !result.ok);
  CHECK("applyOperationID non-zero", result.applyOperationID != 0);
  CHECK("status Failed", app::doc::getApplyStatus(service) == app::doc::ApplyStatus::Failed);
  CHECK("active operation cleared", service.apply.activeApplyOperationID == 0);
  CHECK("diagnostic document.file.read_failed",
        hasDiagnostic(result.diagnostics, "document.file.read_failed"));
  CHECK("no admitted model", !service.apply.hasLastAdmittedDocModel);
  CHECK("lastAdmittedRevision == 0", app::doc::getLastAdmittedRevision(service) == 0);
  app::doc::destroyDocAuthoringService(service);
}

static void test_query_helpers_return_service_state() {
  TEST("query_helpers_return_service_state");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  app::doc::applySequencerRevision(service, app, 1, "track('bad', TrackSettings {})");

  CHECK("getDocDiagnostics matches",
        &app::doc::getDocDiagnostics(service) == &service.apply.diagnostics);
  CHECK("getApplyStatus matches", app::doc::getApplyStatus(service) == service.apply.status);
  CHECK("getLastAdmittedRevision matches",
        app::doc::getLastAdmittedRevision(service) == service.buffer.lastAdmittedRevision);
  CHECK("status is Failed", app::doc::getApplyStatus(service) == app::doc::ApplyStatus::Failed);
  CHECK("lastAdmittedRevision is 0", app::doc::getLastAdmittedRevision(service) == 0);
  app::doc::destroyDocAuthoringService(service);
}

void runDocAuthoringServiceLifecycleTests() {
  SUITE("DocAuthoringService / Lifecycle");
  test_successful_apply_completes_lifecycle();
  test_parse_failure_fails_lifecycle();
  test_supersedes_existing_active_operation();
  test_file_apply_success_uses_buffer_pipeline();
  test_file_read_failure_is_failed_apply_operation();
  test_query_helpers_return_service_state();
}

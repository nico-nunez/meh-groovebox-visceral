#include "app/doc/DocGrooveboxTargetBuilder.h"

#include "app/doc/DocMetadata.h"
#include "app/doc/DocMixerPlanner.h"
#include "app/doc/DocSequencerPlanner.h"
#include "app/doc/DocSynthPlanner.h"

namespace app::doc {

namespace {

void appendDiagnostics(DocDiagnostics* dst, const DocDiagnostics& src) {
  for (const auto& diagnostic : src)
    dst->push_back(diagnostic);
}

} // namespace

GrooveboxTargetBuildResult buildGrooveboxTargetState(const AuthoredDocModel* model,
                                                     DocID documentID,
                                                     DocRevision revision,
                                                     app::GrooveboxTargetState* out) {
  GrooveboxTargetBuildResult result{};

  if (!model || !out) {
    DocDiagnostic d{};
    d.severity = DiagnosticSeverity::Error;
    d.source = DiagnosticSource::Planner;
    d.documentID = documentID;
    d.revision = revision;
    d.code = docdiag::InternalPlannerError;
    d.message = !model ? "null authored model" : "null groovebox target output";
    result.diagnostics.push_back(d);
    return result;
  }

  *out = app::GrooveboxTargetState{};

  auto synth = buildSynthTargetPrograms(model, documentID, revision, out->synthPrograms);
  appendDiagnostics(&result.diagnostics, synth.diagnostics);
  if (!synth.ok)
    return result;

  auto mixer = buildMixerTargetSnapshot(model, documentID, revision, &out->mixer);
  appendDiagnostics(&result.diagnostics, mixer.diagnostics);
  if (!mixer.ok)
    return result;

  auto sequencer =
      buildSequencerTargetSnapshot(&model->sequencer, documentID, revision, &out->sequencer);
  appendDiagnostics(&result.diagnostics, sequencer.diagnostics);
  if (!sequencer.ok)
    return result;

  for (uint8_t t = 0; t < MAX_TRACKS; ++t)
    out->hasSynthProgram[t] = true;

  out->hasMixer = true;

  out->hasSequencer = true;

  result.ok = true;
  return result;
}

} // namespace app::doc

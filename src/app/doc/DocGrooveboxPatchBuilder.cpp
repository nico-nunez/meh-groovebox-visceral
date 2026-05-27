#include "app/doc/DocGrooveboxPatchBuilder.h"

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

GrooveboxTargetBuildResult buildGrooveboxPatch(const AuthoredDocModel* model,
                                               DocID documentID,
                                               DocRevision revision,
                                               app::GrooveboxPatch* out) {
  GrooveboxTargetBuildResult result{};
  if (!model || !out) {
    DocDiagnostic d{};
    d.severity = DiagnosticSeverity::Error;
    d.source = DiagnosticSource::Planner;
    d.documentID = documentID;
    d.revision = revision;
    d.code = docdiag::InternalPlannerError;
    d.message = !model ? "null authored model" : "null groovebox patch output";
    result.diagnostics.push_back(d);
    return result;
  }

  app::resetGrooveboxPatch(out);

  auto synth = buildSynthPatches(model, documentID, revision, out->synth, out->hasSynth);
  appendDiagnostics(&result.diagnostics, synth.diagnostics);
  if (!synth.ok)
    return result;

  auto mixer = buildMixerPatch(model, documentID, revision, &out->mixer);
  appendDiagnostics(&result.diagnostics, mixer.diagnostics);
  if (!mixer.ok)
    return result;
  out->hasMixer = out->mixer.writeCount > 0;

  auto sequencer = buildSequencerPatch(&model->sequencer, documentID, revision, &out->sequencer);
  appendDiagnostics(&result.diagnostics, sequencer.diagnostics);
  if (!sequencer.ok)
    return result;
  for (uint8_t track = 0; track < app::MAX_TRACKS; ++track) {
    out->hasSequencer =
        out->hasSequencer || (out->sequencer.hasTrack[track] &&
                              app::hasTrackSequencerPatchEdits(out->sequencer.tracks[track]));
  }

  result.ok = true;
  return result;
}

} // namespace app::doc

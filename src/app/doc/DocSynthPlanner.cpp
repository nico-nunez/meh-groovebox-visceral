#include "app/doc/DocSynthPlanner.h"

#include "app/doc/DocMetadata.h"

namespace app::doc {
namespace {

void upsertWrite(AuthoredTrackSynthPatch& patch, const AuthoredSynthParamWrite& write) {
  for (auto& existing : patch.writes) {
    if (existing.paramID == write.paramID) {
      existing = write;
      return;
    }
  }
  patch.writes.push_back(write);
}

DocDiagnostic makeSynthTargetDiagnostic(DocID documentID,
                                        DocRevision revision,
                                        const AuthoredSynthParamWrite* write,
                                        const char* message) {
  DocDiagnostic d{};
  d.severity = DiagnosticSeverity::Error;
  d.source = DiagnosticSource::Planner;
  d.documentID = documentID;
  d.revision = revision;
  d.code = docdiag::SynthPlanningFailed;
  d.message = message ? message : "synth target build failed";
  if (write) {
    d.span = write->span;
    if (write->field && write->field->authoredPath)
      d.relatedTarget = write->field->authoredPath;
  }
  return d;
}

bool appendSynthPatchWrite(app::TrackSynthPatch* patch,
                           const AuthoredSynthParamWrite& write,
                           DocID documentID,
                           DocRevision revision,
                           DocDiagnostics* diagnostics) {
  if (patch->writeCount >= app::MAX_SYNTH_PARAM_PATCH_WRITES) {
    diagnostics->push_back(
        makeSynthTargetDiagnostic(documentID, revision, &write, "synth patch capacity exceeded"));
    return false;
  }

  if (write.paramID == synth::param::PARAM_UNKNOWN ||
      static_cast<int>(write.paramID) >= synth::param::PARAM_COUNT) {
    diagnostics->push_back(
        makeSynthTargetDiagnostic(documentID, revision, &write, "invalid synth param id"));
    return false;
  }

  const auto& def = synth::param::PARAM_DEFS[static_cast<int>(write.paramID)];
  if (write.value < def.min || write.value > def.max) {
    diagnostics->push_back(
        makeSynthTargetDiagnostic(documentID, revision, &write, "synth param out of range"));
    return false;
  }

  app::SynthParamPatchWrite& dst = patch->writes[patch->writeCount++];
  dst.paramID = write.paramID;
  dst.value = write.value;
  return true;
}

} // namespace
//
SynthTargetProgramsResult buildSynthPatches(const AuthoredDocModel* model,
                                            DocID documentID,
                                            DocRevision revision,
                                            app::TrackSynthPatch* out,
                                            bool* hasSynth) {
  SynthTargetProgramsResult result{};
  if (!model || !out || !hasSynth) {
    const char* errMsg = !model ? "null authored model" : "null synth patch output";
    result.diagnostics.push_back(makeSynthTargetDiagnostic(documentID, revision, nullptr, errMsg));
    return result;
  }

  for (uint8_t track = 0; track < app::MAX_TRACKS; ++track) {
    out[track] = app::TrackSynthPatch{};
    hasSynth[track] = false;
  }

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!model->hasSynthState[trackIndex])
      continue;

    const AuthoredTrackSynthPatch& authored = model->synthTracks[trackIndex];
    for (const auto& write : authored.writes) {
      if (!appendSynthPatchWrite(&out[trackIndex],
                                 write,
                                 documentID,
                                 revision,
                                 &result.diagnostics))
        return result;
    }

    hasSynth[trackIndex] = out[trackIndex].writeCount > 0 || out[trackIndex].hasModRoutes ||
                           out[trackIndex].hasFMRoutes || out[trackIndex].hasSignalChain;

    if (authored.hasModRoutes) {
      if (authored.modRoutes.size() > synth::mod_matrix::MAX_MOD_ROUTES) {
        result.diagnostics.push_back(makeSynthTargetDiagnostic(documentID,
                                                               revision,
                                                               nullptr,
                                                               "mod route count exceeds capacity"));
        return result;
      }
      out[trackIndex].hasModRoutes = true;
      out[trackIndex].modRouteCount = static_cast<uint8_t>(authored.modRoutes.size());
      for (size_t r = 0; r < authored.modRoutes.size(); ++r) {
        const auto& ar = authored.modRoutes[r];
        out[trackIndex].modRoutes[r] = {ar.src, ar.dest, ar.amount};
      }
    }

    // FM routes — validate and expand flat list into [carrier][modulator] layout
    // fmRouteCounts is already zeroed: out[trackIndex] = app::TrackSynthPatch{} at loop start
    if (authored.hasFMRoutes) {
      out[trackIndex].hasFMRoutes = true;

      for (const auto& ar : authored.fmRoutes) {
        if (ar.depth < 0.0f || ar.depth > 10.0f) {
          result.diagnostics.push_back(
              makeSynthTargetDiagnostic(documentID,
                                        revision,
                                        nullptr,
                                        "fm route depth out of range [0.0, 10.0]"));
          return result;
        }

        uint8_t& count = out[trackIndex].fmRouteCounts[ar.carrier];

        // Check for duplicate (carrier, modulator) pair
        const auto modFMSrc = static_cast<synth::wavetable::osc::FMSource>(ar.modulator + 1);
        for (uint8_t r = 0; r < count; ++r) {
          if (out[trackIndex].fmRoutes[ar.carrier][r].source == modFMSrc) {
            result.diagnostics.push_back(
                makeSynthTargetDiagnostic(documentID,
                                          revision,
                                          nullptr,
                                          "duplicate fm route (carrier, mod) pair"));
            return result;
          }
        }

        if (count >= synth::preset::NUM_OSCS) {
          result.diagnostics.push_back(
              makeSynthTargetDiagnostic(documentID,
                                        revision,
                                        nullptr,
                                        "too many fm routes for one carrier"));
          return result;
        }

        out[trackIndex].fmRoutes[ar.carrier][count++] = {modFMSrc, ar.depth};
      }
    }

    // Signal chain
    if (authored.hasSignalChain) {
      if (authored.signalChain.size() > synth::signal_chain::MAX_CHAIN_SLOTS) {
        result.diagnostics.push_back(
            makeSynthTargetDiagnostic(documentID,
                                      revision,
                                      nullptr,
                                      "signal chain length exceeds capacity"));
        return result;
      }
      out[trackIndex].hasSignalChain = true;
      out[trackIndex].signalChainLength = static_cast<uint8_t>(authored.signalChain.size());
      for (size_t s = 0; s < authored.signalChain.size(); ++s)
        out[trackIndex].signalChain[s] = authored.signalChain[s];
    }
  }

  result.ok = true;
  return result;
}

void buildAdmittedSynthTargetModel(const AuthoredDocModel* nextModel, AuthoredDocModel* admitted) {
  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!nextModel->hasSynthState[trackIndex])
      continue;

    admitted->hasSynthState[trackIndex] = true;
    AuthoredTrackSynthPatch& admittedPatch = admitted->synthTracks[trackIndex];
    if (!admittedPatch.hasPatch) {
      admittedPatch.hasPatch = true;
      admittedPatch.trackIndex = trackIndex;
    }
    admittedPatch.trackSpan = nextModel->synthTracks[trackIndex].trackSpan;

    for (const auto& write : nextModel->synthTracks[trackIndex].writes)
      upsertWrite(admittedPatch, write);

    const AuthoredTrackSynthPatch& next = nextModel->synthTracks[trackIndex];

    if (next.hasModRoutes) {
      admittedPatch.hasModRoutes = true;
      admittedPatch.modRoutes = next.modRoutes;
    }
    if (next.hasFMRoutes) {
      admittedPatch.hasFMRoutes = true;
      admittedPatch.fmRoutes = next.fmRoutes;
    }
    if (next.hasSignalChain) {
      admittedPatch.hasSignalChain = true;
      admittedPatch.signalChain = next.signalChain;
    }
  }
}

} // namespace app::doc

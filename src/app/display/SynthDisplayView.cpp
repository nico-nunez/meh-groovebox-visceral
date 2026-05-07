#include "app/display/SynthDisplayView.h"

#include "imgui.h"

namespace app::display {
namespace {

void drawParamLine(const SynthParamDisplayValue* value) {
  if (!value || !value->def) {
    ImGui::TextUnformatted("unavailable");
    return;
  }

  ImGui::Text("%s: %.3f", value->def->name, static_cast<double>(value->value));
}

void drawParam(const SynthSummarySnapshot& snapshot, synth::param::ParamID id) {
  drawParamLine(findSynthParam(snapshot, id));
}

void drawOsc(const SynthSummarySnapshot& snapshot, uint8_t oscIndex) {
  const auto& ids = synth::param::OSC_PARAM_IDS[oscIndex];
  ImGui::SeparatorText(oscIndex == 0   ? "Osc 1"
                       : oscIndex == 1 ? "Osc 2"
                       : oscIndex == 2 ? "Osc 3"
                                       : "Osc 4");
  drawParam(snapshot, ids.enabled);
  drawParam(snapshot, ids.bankID);
  drawParam(snapshot, ids.mixLevel);
  drawParam(snapshot, ids.detune);
  drawParam(snapshot, ids.octave);
  drawParam(snapshot, ids.scanPos);
  drawParam(snapshot, ids.fmDepth);
  drawParam(snapshot, ids.ratio);
  drawParam(snapshot, ids.fixed);
  drawParam(snapshot, ids.fixedFreq);
}

} // namespace

void drawSynthSummarySection(const SynthSummarySnapshot& snapshot) {
  ImGui::SeparatorText("Selected Track Synth Summary");
  ImGui::Text("notes: %u", snapshot.noteCount);
  drawParam(snapshot, synth::param::MASTER_GAIN);
  drawParam(snapshot, synth::param::OSC1_ENABLED);
  drawParam(snapshot, synth::param::OSC1_BANK_ID);
  drawParam(snapshot, synth::param::OSC1_MIX_LEVEL);
  drawParam(snapshot, synth::param::SVF_ENABLED);
  drawParam(snapshot, synth::param::SVF_CUTOFF);
  drawParam(snapshot, synth::param::AMP_ENV_ATTACK);
  drawParam(snapshot, synth::param::AMP_ENV_RELEASE);
}

void drawSynthDisplay(const SynthSummarySnapshot& snapshot) {
  if (ImGui::BeginTabBar("SynthDisplayTabs")) {
    if (ImGui::BeginTabItem("Overview")) {
      drawSynthOverviewTab(snapshot);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Osc")) {
      drawSynthOscTab(snapshot);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Filter")) {
      drawSynthFilterTab(snapshot);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Amp Env")) {
      drawSynthAmpEnvTab(snapshot);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Voice")) {
      drawSynthVoiceTab(snapshot);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("FX")) {
      drawSynthFXTab(snapshot);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void drawSynthOverviewTab(const SynthSummarySnapshot& snapshot) {
  drawSynthSummarySection(snapshot);
}

void drawSynthOscTab(const SynthSummarySnapshot& snapshot) {
  for (uint8_t i = 0; i < 4; ++i)
    drawOsc(snapshot, i);
}

void drawSynthFilterTab(const SynthSummarySnapshot& snapshot) {
  ImGui::SeparatorText("Noise");
  drawParam(snapshot, synth::param::NOISE_ENABLED);
  drawParam(snapshot, synth::param::NOISE_MIX_LEVEL);
  ImGui::SeparatorText("SVF");
  drawParam(snapshot, synth::param::SVF_ENABLED);
  drawParam(snapshot, synth::param::SVF_CUTOFF);
  drawParam(snapshot, synth::param::SVF_RESONANCE);
  ImGui::SeparatorText("Ladder");
  drawParam(snapshot, synth::param::LADDER_ENABLED);
  drawParam(snapshot, synth::param::LADDER_CUTOFF);
  drawParam(snapshot, synth::param::LADDER_RESONANCE);
  drawParam(snapshot, synth::param::LADDER_DRIVE);
}

void drawSynthAmpEnvTab(const SynthSummarySnapshot& snapshot) {
  drawParam(snapshot, synth::param::AMP_ENV_ATTACK);
  drawParam(snapshot, synth::param::AMP_ENV_DECAY);
  drawParam(snapshot, synth::param::AMP_ENV_SUSTAIN);
  drawParam(snapshot, synth::param::AMP_ENV_RELEASE);
}

void drawSynthVoiceTab(const SynthSummarySnapshot& snapshot) {
  drawParam(snapshot, synth::param::MONO_ENABLED);
  drawParam(snapshot, synth::param::UNISON_ENABLED);
  drawParam(snapshot, synth::param::UNISON_VOICES);
  drawParam(snapshot, synth::param::UNISON_DETUNE);
  drawParam(snapshot, synth::param::UNISON_SPREAD);
  drawParam(snapshot, synth::param::MASTER_GAIN);
}

void drawSynthFXTab(const SynthSummarySnapshot& snapshot) {
  drawParam(snapshot, synth::param::FX_DISTORTION_ENABLED);
  drawParam(snapshot, synth::param::FX_CHORUS_ENABLED);
  drawParam(snapshot, synth::param::FX_PHASER_ENABLED);
  drawParam(snapshot, synth::param::FX_DELAY_ENABLED);
  drawParam(snapshot, synth::param::FX_REVERB_ENABLED);
}

} // namespace app::display

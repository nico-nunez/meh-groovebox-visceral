#include "PresetCmd.h"

#include "synth/ParamDefs.h"
#include "synth/Preset.h"
#include "synth/PresetApply.h"
#include "synth/PresetIO.h"
#include "synth/PresetSerializer.h"
#include "synth/WavetableBanks.h"
#include <cstdint>

namespace synth::preset {

// ============================================================
// Process Preset Input Command (terminal) Helper
// ============================================================
void processPresetCmd(std::istringstream& iss, Engine& engine) {
  std::string subCmd;
  iss >> subCmd;

  if (subCmd.empty()) {
    printf("Usage: preset <save|load|init|list|info|help>\n");
    return;
  }

  // ---- preset save <name> ----
  if (subCmd == "save") {
    std::string name;
    iss >> name;

    if (name.empty()) {
      printf("Usage: preset save <name>\n");
      return;
    }

    // Capture current engine state
    auto captured = capturePreset(engine);
    captured.metadata.name = name;

    // Save to user presets dir
    std::string path = getUserPresetsDir() + "/" + name + ".json";
    std::string err = savePreset(captured, path);

    if (!err.empty()) {
      printf("Error: %s\n", err.c_str());
      return;
    }

    printf("Saved: %s\n", path.c_str());

    // ---- preset load <name|path> ----
  } else if (subCmd == "load") {
    std::string name;
    iss >> name;

    if (name.empty()) {
      printf("Usage: preset load <name|path>\n");
      return;
    }

    auto loadResult = loadPresetByName(name);
    if (!loadResult.ok()) {
      printf("Error: %s\n", loadResult.error.c_str());
      return;
    }

    auto applyResult = applyPreset(loadResult.preset, engine);

    printf("Loaded: %s", loadResult.preset.metadata.name.c_str());
    if (!loadResult.preset.metadata.category.empty())
      printf(" [%s]", loadResult.preset.metadata.category.c_str());
    printf("\n");

    // Print all warnings (load + apply)
    for (const auto& w : loadResult.warnings)
      printf("  warning: %s\n", w.c_str());
    for (const auto& w : applyResult.warnings)
      printf("  warning: %s\n", w.c_str());

    // ---- preset init ----
  } else if (subCmd == "init") {
    auto initPreset = createInitPreset();
    auto applyResult = applyPreset(initPreset, engine);

    printf("Init preset applied\n");
    for (const auto& w : applyResult.warnings)
      printf("  warning: %s\n", w.c_str());

    // ---- preset list ----
  } else if (subCmd == "list") {
    auto entries = listPresets();

    if (entries.empty()) {
      printf("No presets found\n");
      return;
    }

    printf("Presets:\n");
    for (const auto& entry : entries) {
      printf("  %-20s [%s]\n", entry.name.c_str(), entry.isFactory ? "factory" : "user");
    }

    // ---- preset info <name|path> ----
  } else if (subCmd == "info") {
    std::string name;
    iss >> name;

    if (name.empty()) {
      printf("Usage: preset info <name|path>\n");
      return;
    }

    auto loadResult = loadPresetByName(name);
    if (!loadResult.ok()) {
      printf("Error: %s\n", loadResult.error.c_str());
      return;
    }

    const auto& p = loadResult.preset;
    printf("Name:     %s\n", p.metadata.name.c_str());
    if (!p.metadata.author.empty())
      printf("Author:   %s\n", p.metadata.author.c_str());
    if (!p.metadata.category.empty())
      printf("Category: %s\n", p.metadata.category.c_str());
    if (!p.metadata.description.empty())
      printf("Desc:     %s\n", p.metadata.description.c_str());
    printf("Version:  %u\n", p.version);
    printf("Path:     %s\n", loadResult.filePath.c_str());

    // Quick summary of what's enabled
    using param::ParamID;
    using wavetable::banks::bankIDToString;

    bool osc1Enabled = static_cast<bool>(getPresetValue(p, ParamID::OSC1_ENABLED));
    bool osc2Enabled = static_cast<bool>(getPresetValue(p, ParamID::OSC2_ENABLED));
    bool osc3Enabled = static_cast<bool>(getPresetValue(p, ParamID::OSC3_ENABLED));
    bool osc4Enabled = static_cast<bool>(getPresetValue(p, ParamID::OSC4_ENABLED));

    int oscCount = osc1Enabled + osc2Enabled + osc3Enabled + osc4Enabled;
    printf("Oscs:     %d enabled", oscCount);
    if (osc1Enabled)
      printf(" (1:%s", bankIDToString(p.oscBanks[0]));
    if (osc2Enabled)
      printf(" 2:%s", bankIDToString(p.oscBanks[1]));
    if (osc3Enabled)
      printf(" 3:%s", bankIDToString(p.oscBanks[2]));
    if (osc4Enabled)
      printf(" 4:%s", bankIDToString(p.oscBanks[3]));
    if (oscCount > 0)
      printf(")");
    printf("\n");

    bool svfEnabled = static_cast<bool>(getPresetValue(p, ParamID::SVF_ENABLED));
    bool ladderEnabled = static_cast<bool>(getPresetValue(p, ParamID::LADDER_ENABLED));
    bool saturatorEnabled = static_cast<bool>(getPresetValue(p, ParamID::SATURATOR_ENABLED));
    bool monoEnabled = static_cast<bool>(getPresetValue(p, ParamID::MONO_ENABLED));
    bool portaEnabled = static_cast<bool>(getPresetValue(p, ParamID::PORTA_ENABLED));
    bool unisonEnabled = static_cast<bool>(getPresetValue(p, ParamID::UNISON_ENABLED));

    if (svfEnabled)
      printf("SVF:      %s %.0fHz\n",
             filters::svfModeToString(
                 static_cast<filters::SVFMode>(getPresetValue(p, ParamID::SVF_MODE))),
             getPresetValue(p, ParamID::SVF_CUTOFF));
    if (ladderEnabled)
      printf("Ladder:   %.0fHz\n", getPresetValue(p, ParamID::LADDER_CUTOFF));
    if (saturatorEnabled)
      printf("Sat:      drive=%.1f\n", getPresetValue(p, ParamID::SATURATOR_DRIVE));
    if (p.modMatrixCount)
      printf("Mod:      %u routes\n", p.modMatrixCount);
    if (monoEnabled)
      printf("Mono:     on%s\n",
             static_cast<bool>(getPresetValue(p, ParamID::MONO_LEGATO)) ? " (legato)" : "");
    if (portaEnabled)
      printf("Porta:     on%s\n",
             static_cast<bool>(getPresetValue(p, ParamID::PORTA_LEGATO)) ? " (legato)" : "");
    if (unisonEnabled)
      printf("Unison:   %d voices\n",
             static_cast<uint8_t>(getPresetValue(p, ParamID::UNISON_VOICES)));

    for (const auto& w : loadResult.warnings)
      printf("  warning: %s\n", w.c_str());

  } else if (subCmd == "dump") {
    std::string arg1, arg2;
    iss >> arg1 >> arg2;

    bool wantJson = (arg1 == "json");
    std::string name = wantJson ? arg2 : arg1;

    if (name.empty()) {
      // No name: operate on current engine state
      auto p = capturePreset(engine);
      if (wantJson)
        printf("%s\n", serializePreset(p).c_str());
      else
        printPreset(p);
    } else {
      // Name given: load from file
      auto loadResult = loadPresetByName(name);
      if (!loadResult.ok()) {
        printf("Error: %s\n", loadResult.error.c_str());
        return;
      }
      for (const auto& w : loadResult.warnings)
        printf("  warning: %s\n", w.c_str());
      if (wantJson)
        printf("%s\n", serializePreset(loadResult.preset).c_str());
      else
        printPreset(loadResult.preset);
    }

    // ---- preset help ----
  } else if (subCmd == "help") {
    printf("Preset commands:\n");
    printf("  preset save <name>       - Save current state as user preset\n");
    printf("  preset load <name|path>  - Load preset by name or file path\n");
    printf("  preset init              - Reset to init preset\n");
    printf("  preset list              - List available presets\n");
    printf("  preset info <name|path>  - Show preset metadata\n");
    printf("  preset help              - Show this help\n");
    printf("\nSearch order for load/info: user/ → factory/\n");
  } else {
    printf("Unknown preset command: %s\n", subCmd.c_str());
    printf("Type 'preset help' for usage\n");
  }
}

} // namespace synth::preset

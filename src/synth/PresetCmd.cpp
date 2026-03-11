#include "PresetCmd.h"

#include "synth/PresetApply.h"
#include "synth/PresetIO.h"

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
    int oscCount = p.osc1.enabled + p.osc2.enabled + p.osc3.enabled + p.osc4.enabled;
    printf("Oscs:     %d enabled", oscCount);
    if (p.osc1.enabled)
      printf(" (1:%s", p.osc1.bank.c_str());
    if (p.osc2.enabled)
      printf(" 2:%s", p.osc2.bank.c_str());
    if (p.osc3.enabled)
      printf(" 3:%s", p.osc3.bank.c_str());
    if (p.osc4.enabled)
      printf(" 4:%s", p.osc4.bank.c_str());
    if (oscCount > 0)
      printf(")");
    printf("\n");

    if (p.svf.enabled)
      printf("SVF:      %s %.0fHz\n", p.svf.mode.c_str(), p.svf.cutoff);
    if (p.ladder.enabled)
      printf("Ladder:   %.0fHz\n", p.ladder.cutoff);
    if (p.saturator.enabled)
      printf("Sat:      drive=%.1f\n", p.saturator.drive);
    if (!p.modMatrix.empty())
      printf("Mod:      %zu routes\n", p.modMatrix.size());
    if (p.mono.enabled)
      printf("Mono:     on%s\n", p.mono.legato ? " (legato)" : "");
    if (p.unison.enabled)
      printf("Unison:   %d voices\n", p.unison.voices);

    for (const auto& w : loadResult.warnings)
      printf("  warning: %s\n", w.c_str());

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

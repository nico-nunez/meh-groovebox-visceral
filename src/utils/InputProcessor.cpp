#include "InputProcessor.h"

#include "synth/Engine.h"
#include "synth/ModMatrix.h"
#include "synth/Noise.h"
#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/PresetCmd.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

#include "synth_io/SynthIO.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

namespace synth::utils {
namespace s_io = synth_io;

namespace mm = mod_matrix;
namespace pb = param::bindings;

// ==== Internal Helpers ====
namespace {

// Parse input string and update param value
int setInputParam(const std::string& paramName,
                  std::istringstream& iss,
                  s_io::hSynthSession session) {
  float paramValue;

  auto paramID = pb::getParamIDByName(paramName.c_str());
  if (paramID == param::PARAM_COUNT) {
    printf("Error: Unknown parameter '%s'\n", paramName.c_str());
    return 1;
  }

  auto& paramDef = param::getParamDef(paramID);
  switch (paramDef.type) {

  // Enable/Disable Item
  case param::ParamType::Bool: {
    std::string value;
    iss >> value;

    paramValue = strcasecmp(value.c_str(), "true") == 0 ? 1.0f : 0.0f;

  } break;

  // Set SVF Mode
  case param::ParamType::FilterMode: {
    std::string value;
    iss >> value;

    auto filterMode = pb::getSVFModeType(value.c_str());
    paramValue = static_cast<float>(filterMode);

  } break;

  // Treat all other params values as floats (denormalized)
  default:
    iss >> paramValue;
  }

  /*
   * NOTE(nico): User is entering denormalized value and param is stored
   * denormalized.  May consider normalizing in the future, but seems
   * pointless at this time.
   */
  if (!s_io::setParam(session, static_cast<uint8_t>(paramID), paramValue)) {
    printf("Warning: Param queue full, event dropped\n");
    return 2;
  }

  return 0;
}

using FMSource = wavetable::osc::FMSource;
FMSource parseFMSource(const char* s) {
  if (strcasecmp(s, "osc1") == 0)
    return FMSource::Osc1;
  if (strcasecmp(s, "osc2") == 0)
    return FMSource::Osc2;
  if (strcasecmp(s, "osc3") == 0)
    return FMSource::Osc3;
  if (strcasecmp(s, "osc4") == 0)
    return FMSource::Osc4;
  return FMSource::None;
}

using NoiseType = noise::NoiseType;
NoiseType parseNoiseType(const char* s) {
  if (strcasecmp(s, "pink") == 0)
    return noise::NoiseType::Pink;
  return noise::NoiseType::White;
}

} // namespace

void parseCommand(const std::string& line, Engine& engine, s_io::hSynthSession session) {
  using synth::wavetable::banks::getBankByName;

  std::istringstream iss(line);
  std::string cmd;
  iss >> cmd;

  int errStatus = 0;

  // SET: set param value (adds ParamEvent to the queue)
  if (cmd == "set") {
    std::string paramName;
    iss >> paramName;

    // Direct-handled params — bypass binding system and event queue
    if (paramName == "osc1.bank") {
      std::string value;
      iss >> value;

      auto* bank = getBankByName(value.c_str());

      if (bank)
        engine.voicePool.osc1.bank = bank;
      else
        printf("unknown bank: %s\n", value.c_str());

      return;
    }
    if (paramName == "osc2.bank") {
      std::string value;
      iss >> value;

      auto* bank = synth::wavetable::banks::getBankByName(value.c_str());

      if (bank)
        engine.voicePool.osc2.bank = bank;
      else
        printf("unknown bank: %s\n", value.c_str());

      return;
    }
    if (paramName == "osc3.bank") {
      std::string value;
      iss >> value;

      auto* bank = getBankByName(value.c_str());

      if (bank)
        engine.voicePool.osc3.bank = bank;
      else
        printf("unknown bank: %s\n", value.c_str());

      return;
    }
    if (paramName == "osc4.bank") {
      std::string value;
      iss >> value;

      auto* bank = getBankByName(value.c_str());

      if (bank)
        engine.voicePool.osc4.bank = bank;
      else
        printf("unknown bank: %s\n", value.c_str());

      return;
    }

    if (paramName == "osc1.fmSource") {
      std::string value;
      iss >> value;
      engine.voicePool.osc1.fmSource = parseFMSource(value.c_str());
      return;
    }

    if (paramName == "osc2.fmSource") {
      std::string value;
      iss >> value;
      engine.voicePool.osc2.fmSource = parseFMSource(value.c_str());
      return;
    }

    if (paramName == "osc3.fmSource") {
      std::string value;
      iss >> value;
      engine.voicePool.osc3.fmSource = parseFMSource(value.c_str());
      return;
    }

    if (paramName == "osc4.fmSource") {
      std::string value;
      iss >> value;
      engine.voicePool.osc4.fmSource = parseFMSource(value.c_str());
      return;
    }

    if (paramName == "noise.type") {
      std::string value;
      iss >> value;
      engine.voicePool.noise.type = parseNoiseType(value.c_str());
      return;
    }

    if (paramName == "lfo1.bank") {
      std::string value;
      iss >> value;
      if (value == "sah")
        engine.voicePool.lfo1.bank = nullptr; // S&H sentinel
      else {
        auto* bank = getBankByName(value.c_str());
        if (bank)
          engine.voicePool.lfo1.bank = bank;
        else
          printf("unknown bank: %s\n", value.c_str());
      }
      return;
    }

    if (paramName == "lfo2.bank") {
      std::string value;
      iss >> value;
      if (value == "sah")
        engine.voicePool.lfo2.bank = nullptr; // S&H sentinel
      else {
        auto* bank = getBankByName(value.c_str());
        if (bank)
          engine.voicePool.lfo2.bank = bank;
        else
          printf("unknown bank: %s\n", value.c_str());
      }
      return;
    }

    if (paramName == "lfo3.bank") {
      std::string value;
      iss >> value;
      if (value == "sah")
        engine.voicePool.lfo3.bank = nullptr; // S&H sentinel
      else {
        auto* bank = getBankByName(value.c_str());
        if (bank)
          engine.voicePool.lfo3.bank = bank;
        else
          printf("unknown bank: %s\n", value.c_str());
      }
      return;
    }

    errStatus = setInputParam(paramName, iss, session);
    if (!errStatus)
      printf("OK\n");

    // ==== Get Current Param Value =====
  } else if (cmd == "get") {
    using wavetable::banks::getBankByID;
    std::string paramName;
    iss >> paramName;

    // Direct-handled params — bypass binding system and event queue
    if (paramName == "osc1.bank") {
      if (engine.voicePool.osc1.bank)
        printf("%s = %s\n", paramName.c_str(), engine.voicePool.osc1.bank->name);
      else
        printf("%s bank is null\n", paramName.c_str());
      return;
    }
    if (paramName == "osc2.bank") {
      if (engine.voicePool.osc2.bank)
        printf("%s = %s\n", paramName.c_str(), engine.voicePool.osc2.bank->name);
      else
        printf("%s bank is null\n", paramName.c_str());
      return;
    }
    if (paramName == "osc3.bank") {
      if (engine.voicePool.osc3.bank)
        printf("%s = %s\n", paramName.c_str(), engine.voicePool.osc3.bank->name);
      else
        printf("%s bank is null\n", paramName.c_str());
      return;
    }
    if (paramName == "osc4.bank") {
      if (engine.voicePool.osc4.bank)
        printf("%s = %s\n", paramName.c_str(), engine.voicePool.osc4.bank->name);
      else
        printf("%s bank is null\n", paramName.c_str());
      return;
    }

    auto paramID = pb::getParamIDByName(paramName.c_str());
    if (paramID == param::PARAM_COUNT) {
      printf("Error: Unknown parameter '%s'\n", paramName.c_str());
      return;
    }

    float rawValue = pb::getParamValueByID(engine.paramRouter, paramID);

    printf("%s = %.2f\n", paramName.c_str(), rawValue);

    // ==== List Confirgurable Params =====
  } else if (cmd == "list") {

    std::string optionalParam;
    iss >> optionalParam;

    pb::printParamList(optionalParam.empty() ? nullptr : optionalParam.c_str());

    // ==== Mod Matrix =====
  } else if (cmd == "mod") {
    mm::parseModCommand(iss, engine.voicePool.modMatrix);

    // ==== Presets =====
  } else if (cmd == "preset") {
    preset::processPresetCmd(iss, engine);

    // HELP: print available commands
  } else if (cmd == "help") {
    printf("Commands:\n");
    printf("  set <param> <value>  - Set parameter value\n");
    printf("  get <param>          - Query parameter value\n");
    printf("  list                 - List all parameters\n");
    printf("  mod ...              - Modulation routing (mod help)\n");
    printf("  preset ...           - Preset management (preset help)\n");
    printf("  help                 - Show this help\n");
    printf("  quit                 - Exit\n");
    printf("\nNote commands: a-k (play notes)\n");
  } else if (cmd == "clear") {
    // Clear console
    system("clear");

    // Invalid command
  } else if (cmd != "quit") {
    std::cout << "Invalid command: " << cmd << std::endl;
    printf("Enter 'help' for list of valid commands.\n");
  }
}

} // namespace synth::utils

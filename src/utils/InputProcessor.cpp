#include "InputProcessor.h"

#include "synth/Engine.h"
#include "synth/FXChain.h"
#include "synth/Filters.h"
#include "synth/ModMatrix.h"
#include "synth/Noise.h"
#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/PresetCmd.h"
#include "synth/SignalChain.h"
#include "synth/VoicePool.h"
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
namespace osc = wavetable::osc;

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

    auto filterMode = filters::parseSVFMode(value.c_str());
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

void parseFMCmd(std::istringstream& iss, voices::VoicePool& pool) {
  std::string sub;
  iss >> sub;

  if (sub != "route") {
    printf("unknown fm subcommand: %s\n", sub.c_str());
    return;
  }

  std::string action;
  iss >> action;

  if (action == "add") {
    std::string carrierName, sourceName;
    float depth = 1.0f;
    iss >> carrierName >> sourceName >> depth;

    osc::WavetableOsc* osc = voices::getOscByName(pool, carrierName);

    if (!osc) {
      printf("unknown carrier: %s\n", carrierName.c_str());
      return;
    }

    auto src = osc::parseFMSource(sourceName.c_str());
    if (src == osc::FMSource::None) {
      printf("unknown source: %s\n", sourceName.c_str());
      return;
    }

    // Update existing route if source already present
    for (uint8_t r = 0; r < osc->fmRouteCount; r++) {
      if (osc->fmRoutes[r].source == src) {
        osc->fmRoutes[r].depth = depth;
        printf("updated route %s -> %s depth=%.3f\n",
               sourceName.c_str(),
               carrierName.c_str(),
               depth);
        return;
      }
    }

    if (osc->fmRouteCount >= 4) {
      printf("carrier %s already has 4 routes\n", carrierName.c_str());
      return;
    }

    osc->fmRoutes[osc->fmRouteCount++] = {src, depth};
    printf("added route %s -> %s depth=%.3f\n", sourceName.c_str(), carrierName.c_str(), depth);

  } else if (action == "remove") {
    std::string carrierName, sourceName;
    iss >> carrierName >> sourceName;

    osc::WavetableOsc* osc = voices::getOscByName(pool, carrierName);
    if (!osc) {
      printf("unknown carrier: %s\n", carrierName.c_str());
      return;
    }

    auto src = osc::parseFMSource(sourceName.c_str());
    for (uint8_t r = 0; r < osc->fmRouteCount; r++) {
      if (osc->fmRoutes[r].source == src) {
        // Shift remaining routes down
        for (uint8_t i = r; i < osc->fmRouteCount - 1; i++)
          osc->fmRoutes[i] = osc->fmRoutes[i + 1];
        osc->fmRouteCount--;
        printf("removed route %s -> %s\n", sourceName.c_str(), carrierName.c_str());
        return;
      }
    }
    printf("route not found: %s -> %s\n", sourceName.c_str(), carrierName.c_str());

  } else if (action == "clear") {
    std::string carrierName;
    iss >> carrierName;

    osc::WavetableOsc* osc = voices::getOscByName(pool, carrierName);
    if (!osc) {
      printf("unknown carrier: %s\n", carrierName.c_str());
      return;
    }

    osc->fmRouteCount = 0;
    printf("cleared all routes on %s\n", carrierName.c_str());

  } else if (action == "list") {
    std::string carrierName;
    iss >> carrierName;

    osc::WavetableOsc* osc = voices::getOscByName(pool, carrierName);
    if (!osc) {
      printf("unknown carrier: %s\n", carrierName.c_str());
      return;
    }

    if (osc->fmRouteCount == 0) {
      printf("%s: no routes\n", carrierName.c_str());
    } else {
      for (uint8_t r = 0; r < osc->fmRouteCount; r++)
        printf("  %s -> %s  depth=%.3f\n",
               osc::fmSourceToString(osc->fmRoutes[r].source),
               carrierName.c_str(),
               osc->fmRoutes[r].depth);
    }

  } else {
    printf("usage: fm route add|remove|clear|list <carrier> [<source>] [<depth>]\n");
  }
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

    if (paramName == "osc1.fmSource" || paramName == "osc2.fmSource" ||
        paramName == "osc3.fmSource" || paramName == "osc4.fmSource") {
      std::string value;
      iss >> value;
      auto osc = voices::getOscByName(engine.voicePool, paramName.substr(0, 4));
      auto src = osc::parseFMSource(value.c_str());
      if (src == osc::FMSource::None) {
        osc->fmRouteCount = 0;
      } else {
        osc->fmRoutes[0] = {src, 1.0f};
        osc->fmRouteCount = 1;
      }
      return;
    }

    if (paramName == "noise.type") {
      std::string value;
      iss >> value;
      engine.voicePool.noise.type = noise::parseNoiseType(value.c_str());
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

    // ==== FM Routes =====
  } else if (cmd == "fm") {
    parseFMCmd(iss, engine.voicePool);

    // ==== Mod Matrix =====
  } else if (cmd == "mod") {
    mm::parseModCommand(iss, engine.voicePool.modMatrix);

    // ==== FX Chain =====
  } else if (cmd == "fx") {
    fx_chain::parseFXChainCmd(iss, engine.fxChain);

    // ==== Signal Chain =====
  } else if (cmd == "signal") {
    signal_chain::parseSigChainCmd(iss, engine.voicePool.signalChain);

    // ==== Presets =====
  } else if (cmd == "preset") {
    preset::processPresetCmd(iss, engine);
  } else if (cmd == "panic") {
    voices::panicVoicePool(engine.voicePool);

    // HELP: print available commands
  } else if (cmd == "help") {
    printf("Commands:\n");
    printf("  set <param> <value>  - Set parameter value\n");
    printf("  get <param>          - Query parameter value\n");
    printf("  list                 - List all parameters\n");
    printf("  mod ...              - Modulation routing (mod help)\n");
    printf("  preset ...           - Preset management (preset help)\n");
    printf("  fx ...               - Effects chain order (fx set/list/clear)\n");
    printf("  signal ...           - Signal chain order (signal set/list/clear)\n");
    printf("  panic                - Force ALL voices to release\n");
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

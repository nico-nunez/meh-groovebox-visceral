#include "ParamUtils.h"
#include "ParamDefs.h"

#include "synth/Filters.h"
#include "synth/Noise.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

#include "dsp/Tempo.h"
#include "dsp/fx/Distortion.h"

#include <cstddef>
#include <cstdint>

namespace synth::param::utils {

namespace {
ParseResult parseOK(uint8_t value) {
  return {true, value, nullptr};
}

ParseResult parseFail(const char* error, uint8_t defaultVal = 0) {
  return {false, defaultVal, error};
}

} // namespace

const char* enumToString(ParamType pType, uint8_t paramEnum) {
  switch (pType) {
  case ParamType::OscBankID: {
    using namespace wavetable::banks;
    return bankIDToString(static_cast<BankID>(paramEnum));
  }

  case ParamType::PhaseMode: {
    using namespace wavetable::osc;
    return phaseModeToString(static_cast<PhaseMode>(paramEnum));
  }

  case ParamType::NoiseType: {
    return noise::noiseTypeToString(static_cast<noise::NoiseType>(paramEnum));
  }

  case ParamType::FilterMode: {
    return filters::svfModeToString(static_cast<filters::SVFMode>(paramEnum));
  }

  case ParamType::DistortionType: {
    using namespace dsp::fx::distortion;
    return distortionTypeToString(static_cast<DistortionType>(paramEnum));
  }

  case ParamType::Subdivision: {
    using namespace dsp::tempo;
    return subdivisionToString(static_cast<Subdivision>(paramEnum));
  }
  default:
    return "unknown";
  }
}

ParseResult parseEnum(ParamType pType, const char* token) {
  switch (pType) {
  case ParamType::OscBankID: {
    using namespace wavetable::banks;
    const auto id = parseBankID(token);
    if (id == BankID::Unknown)
      return parseFail("unknown bank; default is 'sine'", BankID::Sine);
    return parseOK(id);
  }

  case ParamType::PhaseMode: {
    using namespace wavetable::osc;
    const auto mode = parsePhaseMode(token);
    if (mode == PhaseMode::Unknown)
      return parseFail("unknown phase mode; default is 'reset'", PhaseMode::Reset);
    return parseOK(mode);
  }

  case ParamType::NoiseType: {
    const auto noiseType = noise::parseNoiseType(token);
    if (noiseType == noise::NoiseType::Unknown)
      return parseFail("unknown noise type; default is 'white'", noise::NoiseType::White);
    return parseOK(noiseType);
  }

  case ParamType::FilterMode: {
    const auto mode = filters::parseSVFMode(token);
    if (mode == filters::SVFMode::Unknown)
      return parseFail("unknown filter mode; default is 'lp'", filters::SVFMode::LP);
    return parseOK(mode);
  }

  case ParamType::DistortionType: {
    using namespace dsp::fx::distortion;
    const auto dist = parseDistortionType(token);
    if (dist == DistortionType::Unknown)
      return parseFail("unknown distortion type; default is 'soft'", DistortionType::Soft);
    return parseOK(dist);
  }

  case ParamType::Subdivision: {
    using namespace dsp::tempo;
    const auto sub = parseSubdivision(token);
    if (sub == Subdivision::Unknown)
      return parseFail("unknown subdivision; default is '1/4'", Subdivision::Quarter);
    return parseOK(sub);
  }
  default:
    return parseFail("Unsupported enum type", 0);
  }
}

} // namespace synth::param::utils

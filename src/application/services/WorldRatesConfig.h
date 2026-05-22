#pragma once

#include <shared/Config.h>
#include <shared/game/ExperienceRates.h>
#include <algorithm>

namespace Firelands {

inline float ClampExperienceRate(float value) {
  if (value < 0.1f)
    return 0.1f;
  if (value > 1000.0f)
    return 1000.0f;
  return value;
}

inline ExperienceRates LoadExperienceRatesFromConfig(Config const &config) {
  ExperienceRates rates;
  rates.kill =
      ClampExperienceRate(config.GetNested<float>({"Rates", "Experience", "Kill"}, 1.0f));
  rates.quest =
      ClampExperienceRate(config.GetNested<float>({"Rates", "Experience", "Quest"}, 1.0f));
  rates.explore = ClampExperienceRate(
      config.GetNested<float>({"Rates", "Experience", "Explore"}, 1.0f));
  return rates;
}

} // namespace Firelands

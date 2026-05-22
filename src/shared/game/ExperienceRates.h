#pragma once

namespace Firelands {

/// Server-wide XP multipliers from `worldserver.yaml` → `Rates.Experience.*`.
struct ExperienceRates {
  float kill = 1.0f;
  float quest = 1.0f;
  float explore = 1.0f;
};

} // namespace Firelands

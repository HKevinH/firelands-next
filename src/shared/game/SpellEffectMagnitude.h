#pragma once

#include <shared/Common.h>

#include <cmath>

namespace Firelands {
namespace SpellEffectMagnitude {

/// Approximate `SpellEffect.dbc` magnitude for server MVP (matches `SignedImmediateHealthDelta`
/// and periodic aura ticks). Cataclysm rows often store heals as negative `EffectBasePoints`
/// (e.g. -4 ⇒ ~4–5 per tick); positive rows use `basePoints + 1` plus average die.
inline int32 NeutralMagnitude(int32 basePoints, int32 dieSides) {
  int32 const diceMid = dieSides > 0 ? (dieSides + 1) / 2 : 0;
  if (basePoints < 0)
    return (-basePoints) + diceMid;
  return (basePoints + 1) + diceMid;
}

inline int32 SignedImmediateHealthDelta(uint32 spellEffectKind, int32 basePoints,
                                        int32 dieSides) {
  int32 const magnitude = NeutralMagnitude(basePoints, dieSides);
  if (magnitude == 0)
    return 0;
  constexpr uint32 kSchoolDamage = 2u;
  constexpr uint32 kHeal = 10u;
  if (spellEffectKind == kSchoolDamage)
    return -magnitude;
  if (spellEffectKind == kHeal)
    return magnitude;
  return 0;
}

inline int32 NeutralMagnitudeAtLevel(int32 basePoints, int32 dieSides,
                                    float realPointsPerLevel, uint8 level) {
  int32 mag = 0;
  if (basePoints != 0 || dieSides != 0)
    mag = NeutralMagnitude(basePoints, dieSides);
  else if (realPointsPerLevel == 0.f)
    mag = NeutralMagnitude(basePoints, dieSides);
  if (realPointsPerLevel != 0.f) {
    mag += static_cast<int32>(std::lround(
        realPointsPerLevel * static_cast<float>(level > 0 ? level : 1)));
  }
  return mag;
}

inline int32 PeriodicHealTick(int32 basePoints, int32 dieSides) {
  return NeutralMagnitude(basePoints, dieSides);
}

inline int32 PeriodicHealTickAtLevel(int32 basePoints, int32 dieSides,
                                     float realPointsPerLevel, uint8 level) {
  return NeutralMagnitudeAtLevel(basePoints, dieSides, realPointsPerLevel, level);
}

inline int32 PeriodicDamageTick(int32 basePoints, int32 dieSides) {
  int32 const mag = NeutralMagnitude(basePoints, dieSides);
  return mag > 0 ? -mag : 0;
}

inline int32 PeriodicDamageTickAtLevel(int32 basePoints, int32 dieSides,
                                       float realPointsPerLevel, uint8 level) {
  int32 const mag =
      NeutralMagnitudeAtLevel(basePoints, dieSides, realPointsPerLevel, level);
  return mag > 0 ? -mag : 0;
}

} // namespace SpellEffectMagnitude
} // namespace Firelands

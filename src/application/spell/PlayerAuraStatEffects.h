#pragma once

#include <array>
#include <cstdint>
#include <domain/world/Aura.h>
#include <domain/repositories/ISpellDefinitionStore.h>
#include <vector>

namespace Firelands {

/// Aggregated stat/rating/AP bonuses from active auras (update fields on apply/remove).
struct PlayerAuraStatBonus {
  std::array<int32, 5> posStat{};
  std::array<int32, 5> negStat{};
  /// `PLAYER_FIELD_COMBAT_RATING_1` + index (0..25).
  std::array<int32, 26> combatRating{};
  int32 attackPowerModPos = 0;
  int32 attackPowerModNeg = 0;
  /// Fraction added to melee AP (e.g. 0.07f = 7% from `SPELL_AURA_MOD_ATTACK_POWER_PCT`).
  float attackPowerMultiplier = 0.f;
};

/// Sums bonuses from `auras` using `spellDefinitions` aura rows (`auraEffects` when present).
PlayerAuraStatBonus ComputePlayerAuraStatBonus(
    std::vector<Aura> const &auras, ISpellDefinitionStore const *spellDefinitions,
    uint8 casterLevel);

} // namespace Firelands

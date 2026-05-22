#include <application/spell/PlayerAuraStatEffects.h>
#include <shared/game/SpellAuraTypes.h>
#include <shared/game/SpellEffectMagnitude.h>

namespace Firelands {

namespace {

constexpr int kMaxStatIndex = 4;

void ApplyAuraRowToBonus(SpellDefinition const &def, SpellAuraEffectRow const &row,
                         uint8 casterLevel, PlayerAuraStatBonus &bonus) {
  uint8 const level = casterLevel > 0 ? casterLevel : 1;
  int32 const magnitude = SpellEffectMagnitude::NeutralMagnitudeAtLevel(
      row.basePoints, row.dieSides, row.realPointsPerLevel, level);

  if (row.auraType == kSpellAuraModStat) {
    int const statIdx = row.miscValue;
    if (statIdx >= 0 && statIdx <= kMaxStatIndex) {
      if (magnitude >= 0)
        bonus.posStat[static_cast<size_t>(statIdx)] += magnitude;
      else
        bonus.negStat[static_cast<size_t>(statIdx)] += -magnitude;
    }
    return;
  }

  if (row.auraType == kSpellAuraModPercentStat) {
    int const statIdx = row.miscValue;
    if (statIdx >= 0 && statIdx <= kMaxStatIndex && magnitude != 0) {
      if (magnitude >= 0)
        bonus.posStat[static_cast<size_t>(statIdx)] += magnitude;
      else
        bonus.negStat[static_cast<size_t>(statIdx)] += -magnitude;
    }
    return;
  }

  if (row.auraType == kSpellAuraModRating) {
    int const ratingIdx = row.miscValue;
    if (ratingIdx >= 0 && ratingIdx < static_cast<int>(bonus.combatRating.size()))
      bonus.combatRating[static_cast<size_t>(ratingIdx)] += magnitude;
    return;
  }

  if (row.auraType == kSpellAuraModAttackPower) {
    if (magnitude >= 0)
      bonus.attackPowerModPos += magnitude;
    else
      bonus.attackPowerModNeg += -magnitude;
    return;
  }

  if (row.auraType == kSpellAuraModAttackPowerPct) {
    if (magnitude != 0)
      bonus.attackPowerMultiplier += static_cast<float>(magnitude) / 100.0f;
  }
}

void ApplyDefinitionAurasToBonus(SpellDefinition const &def, uint8 casterLevel,
                                 PlayerAuraStatBonus &bonus) {
  if (!def.auraEffects.empty()) {
    for (SpellAuraEffectRow const &row : def.auraEffects)
      ApplyAuraRowToBonus(def, row, casterLevel, bonus);
    return;
  }
  if (!def.hasAuraEffect)
    return;
  SpellAuraEffectRow row{};
  row.effectIndex = def.auraEffectIndex;
  row.auraType = def.auraEffectType;
  row.basePoints = def.auraBasePoints;
  row.dieSides = def.auraDieSides;
  row.realPointsPerLevel = def.auraRealPointsPerLevel;
  ApplyAuraRowToBonus(def, row, casterLevel, bonus);
}

} // namespace

PlayerAuraStatBonus ComputePlayerAuraStatBonus(
    std::vector<Aura> const &auras, ISpellDefinitionStore const *spellDefinitions,
    uint8 casterLevel) {
  PlayerAuraStatBonus bonus{};
  if (!spellDefinitions)
    return bonus;

  for (Aura const &aura : auras) {
    std::optional<SpellDefinition> def =
        spellDefinitions->GetDefinition(aura.GetSpellId());
    if (!def)
      continue;
    uint8 const level = aura.GetClientWireMeta().casterLevel > 0
                            ? aura.GetClientWireMeta().casterLevel
                            : casterLevel;
    ApplyDefinitionAurasToBonus(*def, level, bonus);
  }
  return bonus;
}

} // namespace Firelands

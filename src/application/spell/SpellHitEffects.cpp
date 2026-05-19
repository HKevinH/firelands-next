#include <application/spell/SpellHitEffects.h>
#include <application/spell/SpellManager.h>
#include <shared/game/SpellAttributes.h>
#include <shared/game/SpellAuraTypes.h>
#include <shared/game/SpellEffectMagnitude.h>

namespace Firelands {
namespace SpellHitEffects {

namespace {

bool AuraIsNegative(SpellDefinition const &def) {
  if (def.auraEffectType == kSpellAuraPeriodicDamage)
    return true;
  if ((def.attributes & SpellAttr0::kNegativeSpell) != 0u)
    return true;
  if ((def.attributes & SpellAttr0::kAuraIsDebuff) != 0u)
    return true;
  return false;
}

} // namespace

uint64 ResolvePrimarySpellHitUnitGuid(uint32 clientTargetFlags, uint64 casterGuid,
                                      uint64 unitTargetGuid) {
  if ((clientTargetFlags & SpellCastWire::ClientTargetPrimaryGuidMask) != 0 &&
      unitTargetGuid != 0u)
    return unitTargetGuid;
  return casterGuid;
}

void ApplyImmediateHealthFromDefinition(SpellDefinition const *def, uint64 hitGuid,
                                        SpellCastOutcome *out) {
  if (!out || !def || def->immediateHealthEffectDelta == 0)
    return;
  out->hasDirectHealthEffect = true;
  out->directHealthTargetGuid = hitGuid;
  out->directHealthDelta = def->immediateHealthEffectDelta;
}

void ApplyAuraFromDefinition(SpellDefinition const *def, uint64 hitGuid, uint64 casterGuid,
                             uint8 casterLevel, std::chrono::steady_clock::time_point now,
                             ISpellCastTables const *castTables, SpellCastOutcome *out) {
  if (!out || !def || !def->hasAuraEffect)
    return;

  uint32 durationMs = 0;
  if (castTables) {
    uint32 const idx =
        def->auraDurationIndex != 0u ? def->auraDurationIndex : def->durationIndex;
    durationMs = castTables->GetDurationMs(idx, casterLevel);
  }
  if (durationMs == 0u)
    durationMs = 3600000u;

  out->hasAuraApply = true;
  out->auraTargetGuid = hitGuid;
  out->auraCasterGuid = casterGuid;
  out->auraSpellId = def->id;
  out->auraEffectType = def->auraEffectType;
  out->auraEffectIndex = def->auraEffectIndex;
  out->auraBasePoints = def->auraBasePoints;
  out->auraDieSides = def->auraDieSides;
  out->auraDurationMs = durationMs;
  out->auraPeriodicPeriodMs = def->auraPeriodicPeriodMs;
  uint8 const level = casterLevel > 0 ? casterLevel : 1;
  if (def->auraEffectType == kSpellAuraPeriodicHeal) {
    out->auraPeriodicHealthDeltaPerTick =
        SpellEffectMagnitude::PeriodicHealTickAtLevel(
            def->auraBasePoints, def->auraDieSides, def->auraRealPointsPerLevel, level);
  } else if (def->auraEffectType == kSpellAuraPeriodicDamage) {
    out->auraPeriodicHealthDeltaPerTick =
        SpellEffectMagnitude::PeriodicDamageTickAtLevel(
            def->auraBasePoints, def->auraDieSides, def->auraRealPointsPerLevel, level);
  } else {
    out->auraPeriodicHealthDeltaPerTick = def->auraPeriodicHealthDeltaPerTick;
  }
  out->auraIsNegative = AuraIsNegative(*def);
  out->auraCasterLevel = level;
  (void)now;
}

} // namespace SpellHitEffects
} // namespace Firelands

#pragma once

#include <shared/Common.h>

namespace Firelands {

/// Client DBC lookups tied to `Spell.dbc` indices (`CastingTimeIndex`, `RangeIndex`).
/// Implementations are read-only after load; hot path must be O(1) without allocations.
class ISpellCastTables {
public:
  virtual ~ISpellCastTables() = default;

  /// Milliseconds from `SpellCastTimes.dbc` for `castingTimeIndex`; 0 if unknown or index 0.
  virtual uint32 GetCastTimeMs(uint32 castingTimeIndex) const = 0;

  /// `max(RangeMax[0], RangeMax[1])` yards from `SpellRange.dbc`; 0 if unknown / index 0.
  /// Callers treat 0 as "not loaded or no limit yet" until range validation exists.
  virtual float GetHostileRangeMaxYards(uint32 rangeIndex) const = 0;

  /// `SpellCooldowns.dbc` row for `Spell.dbc` `CooldownsID` (0 = leave outputs zeroed).
  virtual void GetCooldownTiming(uint32 cooldownsId, uint32 *categoryRecoveryMs,
                                 uint32 *recoveryMs, uint32 *startRecoveryMs) const = 0;

  /// `SpellPower.dbc` `ManaCost` for row id `spellPowerId` (Spell.dbc `PowerDisplayID`); 0 if
  /// id is 0 or row missing.
  virtual uint32 GetSpellPowerManaCost(uint32 spellPowerId) const = 0;

  /// `SpellCategories.dbc` `Category` field for row id `categoriesId` from `Spell.dbc`
  /// (`CategoriesID`). Used with `SpellCooldowns.dbc` `CategoryRecoveryTime` for category
  /// cooldown. Returns 0 if unknown or `categoriesId` is 0.
  virtual uint32 GetSpellCategoryGroupForCategoriesId(uint32 categoriesId) const = 0;
};

} // namespace Firelands

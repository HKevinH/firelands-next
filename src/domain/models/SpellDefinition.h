#pragma once

#include <shared/Common.h>

namespace Firelands {

/// Subset of client `Spell.dbc` (4.3.4) fields loaded for server-side spell logic.
/// Field indices follow TCPP `SpellEntryfmt` / `SpellEntry` in `DBCStructure.h`.
struct SpellDefinition {
  uint32 id = 0;
  uint32 attributes = 0;
  /// `Spell.dbc` `AttributesEx2` (SPELL_ATTR2_* / wowdev AttributesExB).
  uint32 attributesEx2 = 0;
  uint32 castingTimeIndex = 0;
  uint32 durationIndex = 0;
  uint32 powerType = 0;
  uint32 rangeIndex = 0;
  uint32 schoolMask = 0;
  /// `Spell.dbc` → `SpellCooldowns.dbc` (TCPP field `CooldownsID`).
  uint32 cooldownsId = 0;
  /// `Spell.dbc` field 42 (TCPP `PowerDisplayID`): row id in `SpellPower.dbc` for base mana.
  uint32 spellPowerId = 0;
  /// Resource1 cost: base from `SpellPower.dbc` via `spellPowerId`; optional `spell_dbc`
  /// `MvpManaCost` overrides when non-NULL at merge.
  uint32 manaCost = 0;
  /// Phase D MVP: applied to primary hit target when non-zero (negative = damage).
  /// Populated from optional `spell_dbc.MvpDirectHealthDelta` merge when present.
  int32 directHealthEffectBasePoints = 0;
};

} // namespace Firelands

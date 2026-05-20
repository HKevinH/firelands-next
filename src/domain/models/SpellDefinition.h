#pragma once

#include <shared/Common.h>
#include <shared/game/SpellAttributes.h>

namespace Firelands {

/// Subset of client `Spell.dbc` (4.3.4) fields loaded for server-side spell logic.
/// Field indices follow SpellEntryfmt / SpellEntry in DBCStructure.h.
struct SpellDefinition {
  uint32 id = 0;
  uint32 attributes = 0;
  /// `Spell.dbc` `AttributesEx` (SPELL_ATTR_EX_* / wowdev first extended attribute dword).
  uint32 attributesEx = 0;
  /// `Spell.dbc` `AttributesEx2` (SPELL_ATTR_EX2_* / wowdev AttributesExB).
  uint32 attributesEx2 = 0;
  /// `Spell.dbc` `AttributesEx8` — e.g. `SPELL_ATTR8_AURA_SEND_AMOUNT`.
  uint32 attributesEx8 = 0;
  uint32 castingTimeIndex = 0;
  uint32 durationIndex = 0;
  uint32 powerType = 0;
  uint32 rangeIndex = 0;
  uint32 schoolMask = 0;
  /// `Spell.dbc` → `SpellCategories.dbc` row id (CategoriesID).
  uint32 categoriesId = 0;
  /// `Spell.dbc` → `SpellCooldowns.dbc` (CooldownsID field).
  uint32 cooldownsId = 0;
  /// `Spell.dbc` field 42 (PowerDisplayID): row id in `SpellPower.dbc` for base mana.
  uint32 spellPowerId = 0;
  /// `Spell.dbc` `LevelsID` → `SpellLevels.dbc` row (0 = no level gate).
  uint32 levelsId = 0;
  /// Required character level from `SpellLevels.dbc` (`SpellLevel`); 0 = always allowed.
  uint8 requiredLevel = 0;
  /// Resource1 cost from `SpellPower.dbc` via `spellPowerId` (after merge loads ids).
  uint32 manaCost = 0;
  /// First spell-hit immediate HP delta from `SpellEffect.dbc` (school damage / generic heal).
  /// Negative removes HP (damage); positive restores HP (heal). Zero = none for Phase D simplification.
  int32 immediateHealthEffectDelta = 0;
  /// Any `SpellEffect.dbc` row uses `SPELL_EFFECT_HEAL` for this spell (all indices scanned).
  bool spellEffectHasHealKind = false;
  /// Any row uses school damage, health leech, or environmental damage (polarity hint when delta is 0).
  bool spellEffectHasHarmKind = false;

  /// True if any SpellEffect.dbc row has effect = SPELL_EFFECT_APPLY_AURA (6).
  bool hasAuraEffect = false;
  /// Aura effect subtype when hasAuraEffect (e.g., SPELL_AURA_MOD_STAT, SPELL_AURA_PERIODIC_HEAL).
  uint32 auraEffectType = 0;
  /// `SpellEffect.dbc` `EffectIndex` (0–2) for the chosen apply-aura row.
  uint8 auraEffectIndex = 0;
  /// Bits 0–2 set for each `SpellEffect.dbc` apply-aura row on this spell (wire `AFLAG_EFF_INDEX_*`).
  uint8 auraActiveEffectMask = 0;
  /// Base points from SpellEffect.dbc for the aura effect (used for aura magnitude).
  int32 auraBasePoints = 0;
  /// DieSides from SpellEffect.dbc for the aura effect (used for aura magnitude range).
  int32 auraDieSides = 0;
  /// `SpellEffect.dbc` `EffectRealPointsPerLevel` (field 6) for the chosen apply-aura row.
  float auraRealPointsPerLevel = 0.f;
  /// Duration index from Spell.dbc - used to determine aura duration.
  uint32 auraDurationIndex = 0;
  /// `SpellEffect.dbc` `EffectAuraPeriod` (ms) when the aura effect is periodic.
  uint32 auraPeriodicPeriodMs = 0;
  /// Signed HP change per periodic tick (negative = damage).
  int32 auraPeriodicHealthDeltaPerTick = 0;

  bool isPassiveSpell() const { return (attributes & SpellAttr0::kPassive) != 0u; }

  bool sendsAuraEffectAmountOnWire() const {
    return (attributesEx8 & SpellAttr8::kAuraSendAmount) != 0u;
  }

  /// False when `SPELL_ATTR0_CANT_CANCEL` is set (right-click buff dismiss blocked).
  bool playerCanCancelAuraByClient() const {
    return (attributes & SpellAttr0::kCantCancel) == 0u;
  }
};

} // namespace Firelands

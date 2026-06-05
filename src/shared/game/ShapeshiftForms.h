#pragma once

#include <shared/Common.h>

namespace Firelands {

/// `UnitBytes2` shapeshift form byte (client 4.3.4 `ShapeshiftForm` / `Unit.h`).
/// Warrior stances are the only forms in scope; values match `SpellShapeshiftForm.dbc`.
enum ShapeshiftForm : uint8 {
  FORM_NONE = 0,
  FORM_BATTLESTANCE = 17,
  FORM_DEFENSIVESTANCE = 18,
  FORM_BERSERKERSTANCE = 19,
};

/// Warrior stance spell ids (`Spell.dbc`). Each carries one `SPELL_AURA_MOD_SHAPESHIFT`
/// effect whose `EffectMiscValue` is the matching `ShapeshiftForm`.
constexpr uint32 kSpellBattleStance = 2457u;
constexpr uint32 kSpellDefensiveStance = 71u;
constexpr uint32 kSpellBerserkerStance = 2458u;

/// `SpellShapeshift.dbc` Stances/StancesNot bitmask for a form: `1 << (form - 1)`.
/// `FORM_NONE` (no shapeshift) maps to mask 0.
constexpr uint32 StanceMaskFromForm(uint8 form) {
  return form ? (1u << (form - 1u)) : 0u;
}

/// True for Battle/Defensive/Berserker stance spells.
bool IsWarriorStanceSpell(uint32 spellId);

/// `ShapeshiftForm` granted by a warrior stance spell (0 if `spellId` is not a stance).
uint8 WarriorStanceFormForSpell(uint32 spellId);

/// Inverse of `WarriorStanceFormForSpell` (0 if `form` is not a warrior stance form).
uint32 StanceSpellForForm(uint8 form);

/// Hardcoded `SpellShapeshift.dbc`-equivalent gating for warrior abilities (MVP: only the
/// stances are in scope, so abilities that require/forbid a stance are listed here instead of
/// parsing the DBC). Returns true and fills `stances`/`stancesNot` (form bitmasks) when the
/// spell has a stance requirement; false otherwise.
/// TODO(Preservation): replace this table by wrapping `SpellShapeshift.dbc` in
/// `SpellCastTablesDbc` and mapping `Spell.dbc` ShapeshiftID -> Stances/StancesNot.
bool TryGetWarriorAbilityStanceRequirement(uint32 spellId, uint32 &stances,
                                           uint32 &stancesNot);

/// Rage (POWER1) kept when switching warrior stance. Baseline resets to 0.
/// TODO(Preservation): Tactical Mastery talent keeps up to N rage on swap.
uint32 RageRetainedOnStanceSwitch(uint32 currentRage);

/// Passive damage modifiers granted by a warrior stance, as percent points applied to all
/// schools (e.g. `-10` = deal/receive 10% less). Fallback values used only when the spell's
/// `SpellEffect.dbc` rows are absent; tune freely. @return false for forms without modifiers.
/// Battle Stance is neutral; Defensive mitigates and softens output; Berserker is glass-cannon.
bool GetWarriorStanceDamageMods(uint8 form, int32 &damageDonePctPoints,
                                int32 &damageTakenPctPoints);

} // namespace Firelands

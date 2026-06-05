#pragma once

#include <shared/Common.h>

namespace Firelands {

/// Warrior Charge (`Spell.dbc` 100): `SPELL_EFFECT_CHARGE` rushes the caster to a hostile
/// target, triggers Charge Stun, and generates rage. Battle Stance gated (no talents).
constexpr uint32 kSpellCharge = 100u;
/// Charge Stun (`Spell.dbc` 7922): 1.5s `SPELL_AURA_MOD_STUN` applied to the charge target.
constexpr uint32 kSpellChargeStun = 7922u;
/// Charge Stun duration (ms).
constexpr uint32 kChargeStunDurationMs = 1500u;
/// Rage granted by Charge. POWER1 rage is stored as `rage * 10` (0..1000), so 20 rage = 200.
constexpr int32 kChargeRageGain = 200;

bool IsChargeSpell(uint32 spellId);

/// Fills the triggered stun spell id and rage gain for a charge-like spell.
/// @return false when `spellId` is not a charge ability.
bool TryGetWarriorChargeData(uint32 spellId, uint32 &triggeredStunSpellId,
                             int32 &rageGain);

/// True for warrior crowd-control debuffs that should set `UNIT_FLAG_STUNNED` on the target.
bool IsWarriorStunSpell(uint32 spellId);

} // namespace Firelands

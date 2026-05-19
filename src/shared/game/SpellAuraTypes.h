#pragma once

#include <shared/Common.h>

namespace Firelands {

/// `SpellEffect.dbc` effect types (subset).
constexpr uint32 kSpellEffectApplyAura = 6u;

/// `SpellEffect.dbc` aura names (`EffectAura` / misc) — subset for Phase F MVP.
constexpr uint32 kSpellAuraPeriodicDamage = 3u;
constexpr uint32 kSpellAuraPeriodicHeal = 8u;

} // namespace Firelands

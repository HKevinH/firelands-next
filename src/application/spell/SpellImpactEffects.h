#pragma once

#include <domain/models/SpellDefinition.h>
#include <shared/dbc/SpellVisualDbc.h>
#include <cstdint>

namespace Firelands {
namespace SpellImpactEffects {

/// Resolves impact kit from `Spell.dbc` `SpellVisualID` pair (fields 19 / 23 on 4.3.4).
/// Prefers the secondary visual (`spellVisualId1`) then primary — matches missile-hit VFX.
uint32 ResolveImpactKitForSpell(SpellDefinition const &def,
                                SpellVisualDbc const &spellVisualDbc);

} // namespace SpellImpactEffects
} // namespace Firelands

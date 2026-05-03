#pragma once

#include <domain/models/SpellDefinition.h>
#include <shared/Common.h>
#include <shared/network/SpellCastWire.h>

namespace Firelands {

struct SpellCastOutcome;

/// Phase D extensibility: compose server-side hit payloads (damage/heal/…) after cast
/// validation succeeds. Keep free functions here so `SpellManager` stays orchestration-only.
namespace SpellHitEffects {

/// Primary hit GUID for direct effects and SpellGo primary target (matches existing cast path).
uint64 ResolvePrimarySpellHitUnitGuid(uint32 clientTargetFlags, uint64 casterGuid,
                                      uint64 unitTargetGuid);

/// Sets `SpellCastOutcome` immediate-health fields when the definition carries a delta.
void ApplyImmediateHealthFromDefinition(SpellDefinition const *def, uint64 hitGuid,
                                        SpellCastOutcome *out);

} // namespace SpellHitEffects

} // namespace Firelands

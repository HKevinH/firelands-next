#pragma once

#include <shared/Common.h>
#include <shared/network/WorldPacket.h>

namespace Firelands {
namespace SpellPeriodicAuraLogWire {

/// Builds `SMSG_PERIODICAURALOG` for `SPELL_AURA_PERIODIC_HEAL` (one aura entry).
void BuildPeriodicHeal(WorldPacket &out, uint64 targetGuid, uint64 casterGuid,
                       uint32 spellId, uint32 healAmount, uint32 overHeal = 0u,
                       uint32 absorb = 0u, bool critical = false);

} // namespace SpellPeriodicAuraLogWire
} // namespace Firelands

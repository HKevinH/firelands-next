#pragma once

#include <application/spell/SpellManager.h>
#include <chrono>
#include <memory>

namespace Firelands {

class Map;

/// Applies aura from `SpellCastOutcome` (server state + `SMSG_AURA_UPDATE`). Call before
/// `SMSG_SPELL_START`/`GO` when possible — client expects aura state early.
void ApplySpellCastAuraOnMap(std::shared_ptr<Map> const &map, SpellCastOutcome const &outcome,
                             std::chrono::steady_clock::time_point now);

/// Applies direct health/power from `SpellCastOutcome` on `map` and broadcasts wire.
void ApplySpellCastOutcomeOnMap(uint32 mapId, std::shared_ptr<Map> const &map,
                                uint64 casterGuid, SpellCastOutcome const &outcome,
                                std::chrono::steady_clock::time_point now);

/// Sends `SMSG_AURA_UPDATE` remove for `visualSlot` on `unitGuid`.
void SendAuraRemoveOnMap(std::shared_ptr<Map> const &map, uint64 unitGuid,
                         uint8 visualSlot);

/// Removes an active aura on `unitGuid` and sends remove wire. @return false if not found.
bool RemovePlayerAuraOnMap(std::shared_ptr<Map> const &map, uint64 unitGuid,
                           uint32 spellId);

} // namespace Firelands

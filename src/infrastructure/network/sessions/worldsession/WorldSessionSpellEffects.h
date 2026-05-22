#pragma once

#include <application/spell/SpellManager.h>
#include <domain/world/Aura.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace Firelands {

class Map;

/// Applies aura from `SpellCastOutcome` (server state + `SMSG_AURA_UPDATE`). Call after
/// `SMSG_SPELL_GO` so the client uses the server visual slot (avoids GO-only ghost buffs).
void ApplySpellCastAuraOnMap(uint32 mapId, std::shared_ptr<Map> const &map,
                             SpellCastOutcome const &outcome,
                             std::chrono::steady_clock::time_point now);

/// Re-sends active auras for `unitGuid` (login / reconnect).
void SendActiveAurasOnMap(std::shared_ptr<Map> const &map, uint64 unitGuid,
                          std::chrono::steady_clock::time_point now);

/// Recomputes aura-derived stat/rating fields and broadcasts `SMSG_UPDATE_OBJECT`.
void BroadcastPlayerAuraStatBonusOnMap(uint32 mapId, std::shared_ptr<Map> const &map,
                                       uint64 unitGuid, uint8 casterLevel);

/// Applies passive auras for `candidateSpellIds` (login: all known passive combat spells).
void ApplyPassiveAurasForKnownSpellsOnMap(
    uint32 mapId, std::shared_ptr<Map> const &map, uint64_t unitGuid,
    uint8_t casterLevel, std::vector<uint32_t> const &candidateSpellIds,
    std::chrono::steady_clock::time_point now);

/// Applies direct health/power from `SpellCastOutcome` on `map` and broadcasts wire.
void ApplySpellCastOutcomeOnMap(uint32 mapId, std::shared_ptr<Map> const &map,
                                uint64 casterGuid, SpellCastOutcome const &outcome,
                                std::chrono::steady_clock::time_point now);

/// Broadcasts target impact VFX (`SMSG_PLAY_SPELL_VISUAL_KIT`) for a successful spell hit.
void BroadcastSpellImpactVisualOnMap(std::shared_ptr<Map> const &map, uint64 nearbyAnchorGuid,
                                     uint32 spellId, uint64 hitTargetGuid);

/// Sends `SMSG_AURA_UPDATE` remove (slot + spell id 0) and refreshes player aura stat fields.
void SendAuraRemovalOnMap(uint32 mapId, std::shared_ptr<Map> const &map,
                          uint64 unitGuid, AuraRemoval const &removal);

/// Sends HoT tick log, optional aura remaining refresh, and health values update.
void SendPeriodicHealTickOnMap(uint32 mapId, std::shared_ptr<Map> const &map,
                               uint64 unitGuid, AuraPeriodicTick const &tick);

/// Removes an active aura on `unitGuid` and sends remove wire. @return false if not found.
bool RemovePlayerAuraOnMap(uint32 mapId, std::shared_ptr<Map> const &map,
                           uint64 unitGuid, uint32 spellId, uint8 casterLevel);

/// Removes `spellId` on any unit; when `casterGuid` is set, only that caster's aura.
bool RemoveAuraOnMapBySpellId(uint32 mapId, std::shared_ptr<Map> const &map,
                              uint32 spellId, uint64 casterGuid = 0);

} // namespace Firelands

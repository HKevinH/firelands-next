#include <infrastructure/world/MapAuraTicker.h>

#include <application/services/WorldService.h>
#include <domain/world/Creature.h>
#include <domain/world/Map.h>
#include <domain/world/Player.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionSpellEffects.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionObjectUpdate.h>

namespace Firelands {

namespace {

namespace ws_obj = WorldSessionObjectUpdate;

template <typename UnitPtr>
void TickUnitAuras(uint32 mapId, std::shared_ptr<Map> const &map, uint64 guid,
                   UnitPtr const &unit, std::chrono::steady_clock::time_point now) {
  for (AuraRemoval const &removal : unit->UpdateAuras(now))
    SendAuraRemoveOnMap(map, guid, removal.visualSlot);

  for (AuraPeriodicTick const &tick : unit->TickPeriodicAuras(now)) {
    if (tick.healthDelta == 0)
      continue;
    unit->ApplyHealthDelta(tick.healthDelta);
    WorldPacket hpUpdate;
    ws_obj::BuildPlayerHealthValuesUpdate(static_cast<uint16>(mapId), guid,
                                          unit->GetLiveHealth(), unit->GetLiveMaxHealth(),
                                          hpUpdate);
    map->BroadcastPacketToNearby(guid, hpUpdate, true);
  }
}

void TickMap(uint32 mapId, std::shared_ptr<Map> const &map,
             std::chrono::steady_clock::time_point now) {
  map->ForEachPlayer([&](std::shared_ptr<Player> const &player) {
    TickUnitAuras(mapId, map, player->GetGuid(), player, now);
  });
  map->ForEachCreature([&](std::shared_ptr<Creature> const &creature) {
    TickUnitAuras(mapId, map, creature->GetGuid(), creature, now);
  });
}

} // namespace

void TickMapAuras(std::chrono::steady_clock::time_point now) {
  WorldService::Instance().ForEachMap(
      [&](uint32 mapId, std::shared_ptr<Map> const &map) {
        TickMap(mapId, map, now);
      });
}

} // namespace Firelands

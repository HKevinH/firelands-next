#include <infrastructure/network/sessions/worldsession/WorldSessionSpellEffects.h>

#include <domain/world/Aura.h>
#include <domain/world/Creature.h>
#include <domain/world/Map.h>
#include <domain/world/Player.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionObjectUpdate.h>
#include <shared/network/AuraUpdateWire.h>
#include <shared/network/WorldPacket.h>

namespace Firelands {

namespace {

namespace ws_obj = WorldSessionObjectUpdate;

void BroadcastUnitAuraPacket(std::shared_ptr<Map> const &map, uint64 unitGuid,
                             WorldPacket &pkt) {
  map->BroadcastPacket(unitGuid, pkt, true);
}

void BroadcastUnitHealthAfterDelta(uint32 mapId, std::shared_ptr<Map> const &map,
                                 uint64 unitGuid, uint32 health, uint32 maxHealth) {
  WorldPacket hpUpdate;
  ws_obj::BuildPlayerHealthValuesUpdate(static_cast<uint16>(mapId), unitGuid, health,
                                        maxHealth, hpUpdate);
  map->BroadcastPacketToNearby(unitGuid, hpUpdate, true);
}

void ApplyAuraFromOutcome(std::shared_ptr<Map> const &map,
                          SpellCastOutcome const &outcome,
                          std::chrono::steady_clock::time_point now) {
  if (!outcome.hasAuraApply || outcome.auraTargetGuid == 0)
    return;

  uint8 slot = 0;
  if (auto target = map->TryGetPlayer(outcome.auraTargetGuid)) {
    slot = target->AllocateAuraVisualSlot(outcome.auraSpellId);
    auto const expire = now + std::chrono::milliseconds(outcome.auraDurationMs);
    auto nextTick = now;
    if (outcome.auraPeriodicPeriodMs > 0u && outcome.auraPeriodicHealthDeltaPerTick != 0)
      nextTick = now + std::chrono::milliseconds(outcome.auraPeriodicPeriodMs);

    Aura aura(outcome.auraSpellId, outcome.auraEffectType, outcome.auraBasePoints,
              outcome.auraDieSides, outcome.auraCasterGuid, expire, slot,
              outcome.auraPeriodicPeriodMs, outcome.auraPeriodicHealthDeltaPerTick,
              nextTick);
    target->AddAura(aura);
  } else if (auto creature = map->TryGetCreature(outcome.auraTargetGuid)) {
    slot = creature->AllocateAuraVisualSlot(outcome.auraSpellId);
    auto const expire = now + std::chrono::milliseconds(outcome.auraDurationMs);
    auto nextTick = now;
    if (outcome.auraPeriodicPeriodMs > 0u && outcome.auraPeriodicHealthDeltaPerTick != 0)
      nextTick = now + std::chrono::milliseconds(outcome.auraPeriodicPeriodMs);

    Aura aura(outcome.auraSpellId, outcome.auraEffectType, outcome.auraBasePoints,
              outcome.auraDieSides, outcome.auraCasterGuid, expire, slot,
              outcome.auraPeriodicPeriodMs, outcome.auraPeriodicHealthDeltaPerTick,
              nextTick);
    creature->AddAura(aura);
  } else {
    return;
  }

  AuraUpdateWire::AuraApplyParams params{};
  params.visualSlot = slot;
  params.spellId = outcome.auraSpellId;
  params.effectIndex = outcome.auraEffectIndex;
  params.casterLevel = outcome.auraCasterLevel;
  params.casterGuid = outcome.auraCasterGuid;
  params.durationMs = outcome.auraDurationMs;
  params.remainingMs = outcome.auraDurationMs;
  params.isNegative = outcome.auraIsNegative;
  WorldPacket pkt;
  AuraUpdateWire::BuildAuraApply(pkt, outcome.auraTargetGuid, params);
  BroadcastUnitAuraPacket(map, outcome.auraTargetGuid, pkt);
}

} // namespace

void ApplySpellCastAuraOnMap(std::shared_ptr<Map> const &map,
                             SpellCastOutcome const &outcome,
                             std::chrono::steady_clock::time_point now) {
  if (!map)
    return;
  ApplyAuraFromOutcome(map, outcome, now);
}

void SendAuraRemoveOnMap(std::shared_ptr<Map> const &map, uint64 unitGuid,
                         uint8 visualSlot) {
  if (!map || unitGuid == 0)
    return;
  WorldPacket pkt;
  AuraUpdateWire::BuildAuraRemove(pkt, unitGuid, visualSlot);
  BroadcastUnitAuraPacket(map, unitGuid, pkt);
}

bool RemovePlayerAuraOnMap(std::shared_ptr<Map> const &map, uint64 unitGuid,
                           uint32 spellId) {
  if (!map || unitGuid == 0 || spellId == 0)
    return false;

  if (auto target = map->TryGetPlayer(unitGuid)) {
    auto const removal = target->TryRemoveAura(spellId);
    if (!removal)
      return false;
    SendAuraRemoveOnMap(map, unitGuid, removal->visualSlot);
    return true;
  }

  if (auto creature = map->TryGetCreature(unitGuid)) {
    auto const removal = creature->TryRemoveAura(spellId);
    if (!removal)
      return false;
    SendAuraRemoveOnMap(map, unitGuid, removal->visualSlot);
    return true;
  }

  return false;
}

void ApplySpellCastOutcomeOnMap(uint32 mapId, std::shared_ptr<Map> const &map,
                                uint64 casterGuid, SpellCastOutcome const &outcome,
                                std::chrono::steady_clock::time_point now) {
  (void)now;
  if (!map)
    return;

  if (outcome.hasDirectHealthEffect && outcome.directHealthDelta != 0) {
    if (auto target = map->TryGetPlayer(outcome.directHealthTargetGuid)) {
      target->ApplyHealthDelta(outcome.directHealthDelta);
      BroadcastUnitHealthAfterDelta(mapId, map, outcome.directHealthTargetGuid,
                                    target->GetLiveHealth(), target->GetLiveMaxHealth());
    } else if (auto cr = map->TryGetCreature(outcome.directHealthTargetGuid)) {
      cr->ApplyHealthDelta(outcome.directHealthDelta);
      BroadcastUnitHealthAfterDelta(mapId, map, outcome.directHealthTargetGuid,
                                    cr->GetLiveHealth(), cr->GetLiveMaxHealth());
    }
  }

  if (outcome.power1Delta != 0 && casterGuid != 0) {
    if (auto casterPl = map->TryGetPlayer(casterGuid)) {
      casterPl->ApplyPower1Delta(outcome.power1Delta);
      WorldPacket pwUpdate;
      ws_obj::BuildPlayerPower1ValuesUpdate(static_cast<uint16>(mapId), casterGuid,
                                            casterPl->GetLivePower1(),
                                            casterPl->GetLiveMaxPower1(), pwUpdate);
      map->BroadcastPacketToNearby(casterGuid, pwUpdate, true);
    }
  }
}

} // namespace Firelands

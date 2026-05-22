#include <application/combat/CombatEntityAdapters.h>
#include <application/combat/CombatHostility.h>
#include <application/combat/CombatService.h>
#include <application/combat/CreatureChaseMovement.h>
#include <application/services/WorldService.h>
#include <algorithm>
#include <boost/asio/redirect_error.hpp>
#include <chrono>
#include <cmath>
#include <vector>
#include <domain/world/Creature.h>
#include <domain/world/Map.h>
#include <domain/world/Player.h>
#include <infrastructure/network/asio/AsioAwaitables.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionObjectUpdate.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionSpellEffects.h>
#include <shared/game/MeleeRange.h>
#include <shared/network/MonsterMovePackets.h>
#include <shared/network/MovementFlags.h>
#include <shared/network/WorldPacket.h>
#include <shared/network/packets/server/CombatPackets.h>

namespace Firelands {

namespace {

constexpr std::chrono::milliseconds kCreatureMoveTickInterval{200};
constexpr float kCreatureMoveDeltaSeconds =
    static_cast<float>(kCreatureMoveTickInterval.count()) / 1000.0f;
constexpr float kCreatureHomeArrivalYards = 0.75f;
constexpr float kCreatureRunSpeedYardsPerSec = 7.0f;

/// Max distance from spawn/home before the creature abandons combat (evade).
constexpr float kCreatureLeashDistanceFromHomeYards = 50.0f;
/// Max distance to the chased player before the creature drops aggro.
constexpr float kCreatureMaxChaseTargetDistanceYards = 45.0f;

float DistanceSquared2d(float ax, float ay, float bx, float by) {
  float const dx = ax - bx;
  float const dy = ay - by;
  return dx * dx + dy * dy;
}

bool IsBeyondLeashFromHome(MovementInfo const &home, float x, float y) {
  float const leashSq = kCreatureLeashDistanceFromHomeYards * kCreatureLeashDistanceFromHomeYards;
  return DistanceSquared2d(home.x, home.y, x, y) > leashSq;
}

bool IsTargetTooFarToChase(float creatureX, float creatureY, float targetX,
                           float targetY) {
  float const maxSq =
      kCreatureMaxChaseTargetDistanceYards * kCreatureMaxChaseTargetDistanceYards;
  return DistanceSquared2d(creatureX, creatureY, targetX, targetY) > maxSq;
}

void SendMeleeSwingFailure(WorldSession &session, application::MeleeSwingResult result) {
  WorldPacket pkt;
  switch (result) {
  case application::MeleeSwingResult::DeadTarget:
    pkt = combat_wire::BuildAttackSwingDeadTarget();
    break;
  case application::MeleeSwingResult::CantAttack:
    pkt = combat_wire::BuildAttackSwingCantAttack();
    break;
  case application::MeleeSwingResult::NotInRange:
    pkt = combat_wire::BuildAttackSwingNotInRange();
    break;
  default:
    return;
  }
  session.SendPacket(pkt);
}

uint32_t HealthDamageDealt(uint32_t before, uint32_t after) {
  return before > after ? before - after : 0u;
}

void BroadcastMeleeHit(std::shared_ptr<Map> const &map, uint32 mapId, uint64 attackerGuid,
                       uint64 victimGuid, uint32 damage, uint32 victimHealth,
                       uint32 victimMaxHealth) {
  if (!map || damage == 0)
    return;
  WorldPacket state =
      combat_wire::BuildAttackerStateUpdate(attackerGuid, victimGuid, damage, victimHealth);
  map->BroadcastPacketToNearby(attackerGuid, state, true);
  BroadcastUnitHealthAfterDelta(mapId, map, victimGuid, victimHealth, victimMaxHealth);
}

bool SessionPlayerInMeleeRangeOf(WorldSession const &session, float targetX, float targetY,
                                 float targetZ) {
  MovementInfo const &pos = session.GetPosition();
  return IsWithinMeleeRange3d(pos.x, pos.y, pos.z, targetX, targetY, targetZ);
}

bool SessionPlayerInMeleeRangeOfNpc(WorldSession const &session,
                                    Firelands::Creature const &creature) {
  MovementInfo const &pos = session.GetPosition();
  return IsWithinMeleeRangeAgainstNpc(pos.x, pos.y, pos.z, creature.GetX(), creature.GetY(),
                                      creature.GetZ());
}

void BroadcastCreatureChaseMove(std::shared_ptr<Map> const &map, uint64 creatureGuid,
                                uint64_t faceTargetGuid, MovementInfo const &from,
                                MovementInfo const &to, int32_t splineId) {
  if (!map)
    return;
  uint32_t const durationMs = monster_move_wire::MonsterMoveDurationMs(
      from.x, from.y, from.z, to.x, to.y, to.z, kCreatureRunSpeedYardsPerSec);
  WorldPacket pkt = monster_move_wire::BuildMonsterMoveToPosition(
      creatureGuid, from.x, from.y, from.z, to.x, to.y, to.z, splineId, durationMs,
      monster_move_wire::FacingTarget, 0.f, faceTargetGuid);
  map->BroadcastPacketToNearby(creatureGuid, pkt, true);
}

void BroadcastCreatureReturnMove(std::shared_ptr<Map> const &map, uint64 creatureGuid,
                                 MovementInfo const &home, MovementInfo const &from,
                                 MovementInfo const &to, int32_t splineId) {
  if (!map)
    return;
  uint32_t const durationMs = monster_move_wire::MonsterMoveDurationMs(
      from.x, from.y, from.z, to.x, to.y, to.z, kCreatureRunSpeedYardsPerSec);
  WorldPacket pkt = monster_move_wire::BuildMonsterMoveToPosition(
      creatureGuid, from.x, from.y, from.z, to.x, to.y, to.z, splineId, durationMs,
      monster_move_wire::FacingSpot, 0.f, 0, home.x, home.y, home.z);
  map->BroadcastPacketToNearby(creatureGuid, pkt, true);
}

void BroadcastCreatureMonsterStop(std::shared_ptr<Map> const &map,
                                  std::shared_ptr<Creature> const &creature,
                                  int32_t splineId) {
  if (!map || !creature)
    return;
  MovementInfo const &pos = creature->GetPosition();
  WorldPacket pkt = monster_move_wire::BuildMonsterMoveStop(creature->GetGuid(), pos.x, pos.y,
                                                            pos.z, splineId);
  map->BroadcastPacketToNearby(creature->GetGuid(), pkt, true);
}

/// Steps `creature` toward a world position and broadcasts splines when it moved.
bool StepCreatureToward(std::shared_ptr<Map> const &map,
                         std::shared_ptr<Creature> const &creature, uint64_t faceTargetGuid,
                         float targetX, float targetY, float targetZ, float stopDistanceYards,
                         uint32_t &moveCounter) {
  if (!map || !creature)
    return false;

  MovementInfo const from = creature->GetPosition();
  application::combat::CreatureChaseConfig config{};
  config.stopDistanceYards = stopDistanceYards;
  auto const step = application::combat::StepCreatureTowardTarget(
      from, targetX, targetY, targetZ, kCreatureMoveDeltaSeconds, config);

  if (step.inStopRange)
    return true;

  MovementInfo pos = step.position;
  map->UpdateObjectPosition(creature->GetGuid(), pos);
  if (step.moved) {
    int32_t const splineId = static_cast<int32_t>(++moveCounter);
    BroadcastCreatureChaseMove(map, creature->GetGuid(), faceTargetGuid, from, pos, splineId);
  }
  return false;
}

bool StepCreatureReturnToward(std::shared_ptr<Map> const &map,
                              std::shared_ptr<Creature> const &creature,
                              MovementInfo const &home, uint32_t &moveCounter) {
  if (!map || !creature)
    return false;

  MovementInfo const from = creature->GetPosition();
  application::combat::CreatureChaseConfig config{};
  config.stopDistanceYards = kCreatureHomeArrivalYards;
  auto const step = application::combat::StepCreatureTowardTarget(
      from, home.x, home.y, home.z, kCreatureMoveDeltaSeconds, config);

  if (step.inStopRange)
    return true;

  MovementInfo pos = step.position;
  map->UpdateObjectPosition(creature->GetGuid(), pos);
  if (step.moved) {
    int32_t const splineId = static_cast<int32_t>(++moveCounter);
    BroadcastCreatureReturnMove(map, creature->GetGuid(), home, from, pos, splineId);
  }
  return false;
}

void ChaseCreatureTowardPlayer(std::shared_ptr<Map> const &map,
                               std::shared_ptr<Creature> const &creature,
                               std::shared_ptr<Player> const &target,
                               WorldSession const &session, uint32_t &moveCounter) {
  if (!map || !creature || !target)
    return;

  MovementInfo const playerPos = session.GetPosition();
  map->UpdateObjectPosition(target->GetGuid(), playerPos);

  application::combat::CreatureChaseConfig config{};
  (void)StepCreatureToward(map, creature, target->GetGuid(), playerPos.x, playerPos.y,
                           playerPos.z, config.stopDistanceYards, moveCounter);
}

} // namespace

void WorldSession::BeginCreatureReturnToHome(uint64_t creatureGuid,
                                           MovementInfo const &home,
                                           uint32_t initialMoveCounter) {
  if (_creatureReturningHome.find(creatureGuid) != _creatureReturningHome.end())
    return;

  auto map = WorldService::Instance().GetMap(_mapId);
  if (!map)
    return;

  auto creature = map->TryGetCreature(creatureGuid);
  if (!creature || creature->GetLiveHealth() == 0)
    return;

  uint32_t moveCounter = initialMoveCounter;
  float const arriveSq = kCreatureHomeArrivalYards * kCreatureHomeArrivalYards;
  if (DistanceSquared2d(creature->GetX(), creature->GetY(), home.x, home.y) <= arriveSq) {
    MovementInfo snapped = home;
    snapped.flags = MOVEMENTFLAG_NONE;
    map->UpdateObjectPosition(creatureGuid, snapped);
    int32_t const stopId = static_cast<int32_t>(++moveCounter);
    BroadcastCreatureMonsterStop(map, creature, stopId);
    return;
  }

  _creatureReturningHome.emplace(creatureGuid, CreatureCombatRuntime{home, moveCounter});
  ScheduleCreatureCombatMovement();
}

void WorldSession::StopCreatureAggro(uint64_t creatureGuid, bool sendAttackStopPackets) {
  auto it = _creatureAggroed.find(creatureGuid);
  if (it == _creatureAggroed.end())
    return;

  MovementInfo const home = it->second.home;
  uint32_t const moveCounter = it->second.moveCounter;
  _creatureAggroed.erase(it);

  if (_meleeVictimGuid == creatureGuid)
    _creatureRetaliating = false;

  uint64_t const player = _playerGuid;

  if (auto map = WorldService::Instance().GetMap(_mapId)) {
    if (auto creature = map->TryGetCreature(creatureGuid)) {
      int32_t const stopId = static_cast<int32_t>(moveCounter + 1u);
      BroadcastCreatureMonsterStop(map, creature, stopId);
    }
    if (sendAttackStopPackets && player != 0) {
      WorldPacket stopCreature =
          combat_wire::BuildAttackStop(creatureGuid, player, false);
      map->BroadcastPacketToNearby(creatureGuid, stopCreature, true);
    }
    BeginCreatureReturnToHome(creatureGuid, home, moveCounter);
  }

  if (_creatureAggroed.empty() && _creatureReturningHome.empty())
    (void)_creatureCombatMoveTimer.cancel();
}

void WorldSession::StopAllCreatureCombat(bool sendAttackStopPackets) {
  std::vector<uint64_t> const aggroGuids = [&] {
    std::vector<uint64_t> guids;
    guids.reserve(_creatureAggroed.size());
    for (auto const &entry : _creatureAggroed)
      guids.push_back(entry.first);
    return guids;
  }();
  for (uint64_t const guid : aggroGuids)
    StopCreatureAggro(guid, sendAttackStopPackets);
  _creatureReturningHome.clear();
  (void)_creatureCombatMoveTimer.cancel();
}

void WorldSession::StartCreatureAggro(uint64_t creatureGuid) {
  if (_playerGuid == 0 || creatureGuid == 0 || !_combatService)
    return;

  _creatureReturningHome.erase(creatureGuid);

  auto map = WorldService::Instance().GetMap(_mapId);
  if (!map)
    return;

  auto attackerPl = map->TryGetPlayer(_playerGuid);
  auto victimCr = map->TryGetCreature(creatureGuid);
  if (!attackerPl || !victimCr || attackerPl->GetLiveHealth() == 0 ||
      victimCr->GetLiveHealth() == 0) {
    return;
  }
  if (!application::CanMeleeAttack(*attackerPl, *victimCr, _factionTemplateDbc.get()))
    return;

  bool const alreadyAggro = _creatureAggroed.find(creatureGuid) != _creatureAggroed.end();
  auto &runtime = _creatureAggroed[creatureGuid];
  if (!alreadyAggro) {
    runtime.home = victimCr->GetPosition();
    runtime.moveCounter = 0;
  }

  application::adapters::CreatureCombatEntity creatureAttacker(victimCr);
  application::adapters::PlayerCombatEntity playerVictim(attackerPl);
  _combatService->StartCombat(creatureAttacker, playerVictim);

  if (!alreadyAggro) {
    WorldPacket creatureStart = combat_wire::BuildAttackStart(creatureGuid, _playerGuid);
    SendPacket(creatureStart);
    map->BroadcastPacketToNearby(creatureGuid, creatureStart, true);
  }

  ScheduleCreatureCombatMovement();
}

void WorldSession::TryAggroCreatureFromSpellDamage(uint64_t targetGuid,
                                                   int32_t healthDelta) {
  if (healthDelta >= 0 || targetGuid == 0)
    return;
  StartCreatureAggro(targetGuid);
}

void WorldSession::StopMeleeAutoAttack(bool sendStopPackets) {
  (void)_meleeAutoAttackTimer.cancel();
  uint64_t const victim = _meleeVictimGuid;
  uint64_t const player = _playerGuid;
  bool const victimWasCreature = _meleeVictimIsCreature;

  _meleeVictimGuid = 0;
  _meleeVictimIsCreature = false;
  _creatureRetaliating = false;

  if (victimWasCreature && victim != 0)
    StopCreatureAggro(victim, sendStopPackets);

  if (!sendStopPackets || player == 0 || victim == 0)
    return;

  auto map = WorldService::Instance().GetMap(_mapId);
  if (!map)
    return;

  bool victimDead = false;
  if (victimWasCreature) {
    if (auto cr = map->TryGetCreature(victim))
      victimDead = cr->GetLiveHealth() == 0;
  } else if (auto pl = map->TryGetPlayer(victim)) {
    victimDead = pl->GetLiveHealth() == 0;
  }

  WorldPacket stopPlayer = combat_wire::BuildAttackStop(player, victim, victimDead);
  SendPacket(stopPlayer);
  map->BroadcastPacketToNearby(player, stopPlayer, false);
}

void WorldSession::EvadeCreatureCombat(uint64_t creatureGuid) {
  StopCreatureAggro(creatureGuid, true);
  if (_meleeVictimGuid == creatureGuid)
    StopMeleeAutoAttack(true);
}

bool WorldSession::ShouldCreatureAbandonChase(std::shared_ptr<Map> const &map,
                                            std::shared_ptr<Creature> const &creature,
                                            std::shared_ptr<Player> const &target,
                                            MovementInfo const &home) const {
  if (!map || !creature || !target)
    return true;

  float const cx = creature->GetX();
  float const cy = creature->GetY();
  if (IsBeyondLeashFromHome(home, cx, cy))
    return true;
  if (IsTargetTooFarToChase(cx, cy, _position.x, _position.y))
    return true;
  return false;
}

void WorldSession::ProcessCreatureCombatMovementTick() {
  if (_playerGuid == 0)
    return;

  auto map = WorldService::Instance().GetMap(_mapId);
  if (!map) {
    StopAllCreatureCombat(false);
    StopMeleeAutoAttack(false);
    return;
  }

  auto attackerPl = map->TryGetPlayer(_playerGuid);
  if (!attackerPl) {
    StopAllCreatureCombat(false);
    return;
  }

  for (auto returnIt = _creatureReturningHome.begin();
       returnIt != _creatureReturningHome.end();) {
    uint64_t const creatureGuid = returnIt->first;
    MovementInfo const home = returnIt->second.home;
    uint32_t &returnCounter = returnIt->second.moveCounter;
    auto creature = map->TryGetCreature(creatureGuid);
    if (!creature || creature->GetLiveHealth() == 0) {
      returnIt = _creatureReturningHome.erase(returnIt);
      continue;
    }

    if (StepCreatureReturnToward(map, creature, home, returnCounter)) {
      MovementInfo snapped = home;
      snapped.flags = MOVEMENTFLAG_NONE;
      map->UpdateObjectPosition(creatureGuid, snapped);
      int32_t const stopId = static_cast<int32_t>(returnCounter + 1u);
      BroadcastCreatureMonsterStop(map, creature, stopId);
      returnIt = _creatureReturningHome.erase(returnIt);
      continue;
    }
    ++returnIt;
  }

  std::vector<uint64_t> evaded;
  for (auto &[creatureGuid, runtime] : _creatureAggroed) {
    auto creature = map->TryGetCreature(creatureGuid);
    if (!creature || creature->GetLiveHealth() == 0 || attackerPl->GetLiveHealth() == 0) {
      evaded.push_back(creatureGuid);
      continue;
    }
    if (ShouldCreatureAbandonChase(map, creature, attackerPl, runtime.home)) {
      evaded.push_back(creatureGuid);
      continue;
    }
    ChaseCreatureTowardPlayer(map, creature, attackerPl, *this, runtime.moveCounter);
  }

  for (uint64_t const guid : evaded) {
    StopCreatureAggro(guid, true);
    if (guid == _meleeVictimGuid)
      StopMeleeAutoAttack(true);
    SendNotification("Creature has evaded.");
  }

  if (!_creatureAggroed.empty() || !_creatureReturningHome.empty())
    ScheduleCreatureCombatMovement();
}

void WorldSession::ScheduleCreatureCombatMovement() {
  if (_creatureAggroed.empty() && _creatureReturningHome.empty())
    return;

  (void)_creatureCombatMoveTimer.cancel();
  _creatureCombatMoveTimer.expires_after(kCreatureMoveTickInterval);

  auto self = shared_from_this();
  Asio::SpawnDetached(_socket.get_executor(), [self, this]() -> Asio::awaitable<void> {
    boost::system::error_code ec;
    co_await _creatureCombatMoveTimer.async_wait(
        boost::asio::redirect_error(Asio::use_awaitable, ec));
    if (!ec)
      ProcessCreatureCombatMovementTick();
  });
}

void WorldSession::ProcessMeleeAutoAttackTick() {
  if (_playerGuid == 0 || _meleeVictimGuid == 0 || !_combatService)
    return;

  auto map = WorldService::Instance().GetMap(_mapId);
  if (!map) {
    StopMeleeAutoAttack(false);
    return;
  }

  auto attackerPl = map->TryGetPlayer(_playerGuid);
  if (!attackerPl || attackerPl->GetLiveHealth() == 0) {
    StopMeleeAutoAttack(true);
    return;
  }

  uint64_t const victimGuid = _meleeVictimGuid;

  application::adapters::PlayerCombatEntity attackerEntity(attackerPl);

  if (_meleeVictimIsCreature) {
    auto victimCr = map->TryGetCreature(victimGuid);
    if (!victimCr) {
      StopMeleeAutoAttack(true);
      return;
    }
    if (!SessionPlayerInMeleeRangeOfNpc(*this, *victimCr)) {
      WorldPacket pkt = combat_wire::BuildAttackSwingNotInRange();
      SendPacket(pkt);
      StopMeleeAutoAttack(true);
      return;
    }
    if (victimCr->GetLiveHealth() == 0) {
      StopMeleeAutoAttack(true);
      return;
    }

    application::adapters::CreatureCombatEntity victimEntity(victimCr);
    uint32_t const hpBefore = victimCr->GetLiveHealth();
    auto const hit = _combatService->ApplyMeleeHit(attackerEntity, victimEntity);
    if (hit != application::MeleeSwingResult::Success) {
      SendMeleeSwingFailure(*this, hit);
      StopMeleeAutoAttack(true);
      return;
    }
    uint32_t const dmg = HealthDamageDealt(hpBefore, victimCr->GetLiveHealth());
    BroadcastMeleeHit(map, _mapId, _playerGuid, victimGuid, dmg, victimCr->GetLiveHealth(),
                        victimCr->GetLiveMaxHealth());

    if (_creatureRetaliating && attackerPl->GetLiveHealth() > 0 &&
        SessionPlayerInMeleeRangeOf(*this, attackerPl->GetX(), attackerPl->GetY(),
                                    attackerPl->GetZ())) {
      application::adapters::CreatureCombatEntity creatureAttacker(victimCr);
      application::adapters::PlayerCombatEntity playerVictim(attackerPl);
      uint32_t const playerHpBefore = attackerPl->GetLiveHealth();
      if (_combatService->ApplyMeleeHit(creatureAttacker, playerVictim) ==
          application::MeleeSwingResult::Success) {
        uint32_t const playerDmg =
            HealthDamageDealt(playerHpBefore, attackerPl->GetLiveHealth());
        BroadcastMeleeHit(map, _mapId, victimGuid, _playerGuid, playerDmg,
                          attackerPl->GetLiveHealth(), attackerPl->GetLiveMaxHealth());
      }
    }

    if (victimCr->GetLiveHealth() == 0)
      StopMeleeAutoAttack(true);
    else
      ScheduleMeleeAutoAttack();
    return;
  }

  auto victimPl = map->TryGetPlayer(victimGuid);
  if (!victimPl) {
    StopMeleeAutoAttack(true);
    return;
  }
  if (!SessionPlayerInMeleeRangeOf(*this, victimPl->GetX(), victimPl->GetY(),
                                   victimPl->GetZ())) {
    WorldPacket pkt = combat_wire::BuildAttackSwingNotInRange();
    SendPacket(pkt);
    StopMeleeAutoAttack(true);
    return;
  }
  if (victimPl->GetLiveHealth() == 0) {
    StopMeleeAutoAttack(true);
    return;
  }

  application::adapters::PlayerCombatEntity victimEntity(victimPl);
  uint32_t const hpBefore = victimPl->GetLiveHealth();
  auto const hit = _combatService->ApplyMeleeHit(attackerEntity, victimEntity);
  if (hit != application::MeleeSwingResult::Success) {
    SendMeleeSwingFailure(*this, hit);
    StopMeleeAutoAttack(true);
    return;
  }
  uint32_t const dmg = HealthDamageDealt(hpBefore, victimPl->GetLiveHealth());
  BroadcastMeleeHit(map, _mapId, _playerGuid, victimGuid, dmg, victimPl->GetLiveHealth(),
                    victimPl->GetLiveMaxHealth());
  if (victimPl->GetLiveHealth() == 0)
    StopMeleeAutoAttack(true);
  else
    ScheduleMeleeAutoAttack();
}

void WorldSession::ScheduleMeleeAutoAttack() {
  if (_playerGuid == 0 || _meleeVictimGuid == 0)
    return;

  (void)_meleeAutoAttackTimer.cancel();
  _meleeAutoAttackTimer.expires_after(
      std::chrono::milliseconds(static_cast<int64_t>(kDefaultMainhandSwingMs)));

  auto self = shared_from_this();
  Asio::SpawnDetached(_socket.get_executor(), [self, this]() -> Asio::awaitable<void> {
    boost::system::error_code ec;
    co_await _meleeAutoAttackTimer.async_wait(
        boost::asio::redirect_error(Asio::use_awaitable, ec));
    if (!ec)
      ProcessMeleeAutoAttackTick();
  });
}

void WorldSession::HandleAttackStop(WorldPacket & /*packet*/) {
  StopMeleeAutoAttack(true);
}

bool WorldSession::PrepareMeleeRetarget(uint64_t victimGuid) {
  if (victimGuid != 0 && _meleeVictimGuid == victimGuid)
    return false;
  if (_meleeVictimGuid != 0)
    StopMeleeAutoAttack(true);
  return true;
}

void WorldSession::HandleAttackSwing(WorldPacket &packet) {
  if (_playerGuid == 0 || !_combatService)
    return;

  uint64_t const victimGuid = WorldSessionObjectUpdate::ReadClientTargetGuid(packet);
  if (victimGuid == 0 || victimGuid == _playerGuid)
    return;

  auto map = WorldService::Instance().GetMap(_mapId);
  if (!map)
    return;

  auto attackerPl = map->TryGetPlayer(_playerGuid);
  if (!attackerPl || attackerPl->GetLiveHealth() == 0)
    return;

  application::adapters::PlayerCombatEntity attackerEntity(attackerPl);

  if (auto victimPl = map->TryGetPlayer(victimGuid)) {
    if (!SessionPlayerInMeleeRangeOf(*this, victimPl->GetX(), victimPl->GetY(),
                                     victimPl->GetZ())) {
      WorldPacket pkt = combat_wire::BuildAttackSwingNotInRange();
      SendPacket(pkt);
      return;
    }
    if (victimPl->GetLiveHealth() == 0) {
      WorldPacket pkt = combat_wire::BuildAttackSwingDeadTarget();
      SendPacket(pkt);
      return;
    }
    if (!application::CanMeleeAttack(*attackerPl, *victimPl)) {
      WorldPacket pkt = combat_wire::BuildAttackSwingCantAttack();
      SendPacket(pkt);
      return;
    }
    if (!PrepareMeleeRetarget(victimGuid))
      return;

    application::adapters::PlayerCombatEntity victimEntity(victimPl);
    uint32_t const hpBefore = victimPl->GetLiveHealth();
    auto const result = _combatService->BeginMeleeSwing(attackerEntity, victimEntity);
    if (result != application::MeleeSwingResult::Success) {
      SendMeleeSwingFailure(*this, result);
      return;
    }

    _meleeVictimGuid = victimGuid;
    _meleeVictimIsCreature = false;
    _creatureRetaliating = false;

    WorldPacket attackStart = combat_wire::BuildAttackStart(_playerGuid, victimGuid);
    SendPacket(attackStart);

    uint32_t const dmg = HealthDamageDealt(hpBefore, victimPl->GetLiveHealth());
    BroadcastMeleeHit(map, _mapId, _playerGuid, victimGuid, dmg, victimPl->GetLiveHealth(),
                      victimPl->GetLiveMaxHealth());
    ScheduleMeleeAutoAttack();
    return;
  }

  if (auto victimCr = map->TryGetCreature(victimGuid)) {
    if (!SessionPlayerInMeleeRangeOfNpc(*this, *victimCr)) {
      WorldPacket pkt = combat_wire::BuildAttackSwingNotInRange();
      SendPacket(pkt);
      return;
    }
    if (victimCr->GetLiveHealth() == 0) {
      WorldPacket pkt = combat_wire::BuildAttackSwingDeadTarget();
      SendPacket(pkt);
      return;
    }
    if (!application::CanMeleeAttack(*attackerPl, *victimCr,
                                     _factionTemplateDbc.get())) {
      WorldPacket pkt = combat_wire::BuildAttackSwingCantAttack();
      SendPacket(pkt);
      return;
    }
    if (!PrepareMeleeRetarget(victimGuid))
      return;

    application::adapters::CreatureCombatEntity victimEntity(victimCr);
    uint32_t const hpBefore = victimCr->GetLiveHealth();
    auto const result = _combatService->BeginMeleeSwing(attackerEntity, victimEntity);
    if (result != application::MeleeSwingResult::Success) {
      SendMeleeSwingFailure(*this, result);
      return;
    }

    _meleeVictimGuid = victimGuid;
    _meleeVictimIsCreature = true;

    WorldPacket attackStart = combat_wire::BuildAttackStart(_playerGuid, victimGuid);
    SendPacket(attackStart);
    map->BroadcastPacketToNearby(_playerGuid, attackStart, false);

    uint32_t const dmg = HealthDamageDealt(hpBefore, victimCr->GetLiveHealth());
    BroadcastMeleeHit(map, _mapId, _playerGuid, victimGuid, dmg, victimCr->GetLiveHealth(),
                      victimCr->GetLiveMaxHealth());

    _creatureRetaliating = true;
    StartCreatureAggro(victimGuid);

    ScheduleMeleeAutoAttack();
    return;
  }

  WorldPacket pkt = combat_wire::BuildAttackSwingCantAttack();
  SendPacket(pkt);
}

} // namespace Firelands

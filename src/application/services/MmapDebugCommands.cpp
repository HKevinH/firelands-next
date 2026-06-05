#include "MmapDebugCommands.h"

#include <application/logic/CreatureSpawnLogic.h>
#include <application/ports/IMapCollisionQueries.h>
#include <application/services/CommandTextUtils.h>
#include <application/services/OnlineCharacterSessionRegistry.h>
#include <application/services/WorldService.h>
#include <domain/repositories/INpcTemplateSearchRepository.h>
#include <domain/world/Creature.h>
#include <domain/world/Map.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionObjectUpdate.h>
#include <shared/Common.h>
#include <shared/Logger.h>
#include <shared/game/UnitCombatStats.h>
#include <shared/game/WowGuid.h>
#include <shared/network/MovementInfo.h>
#include <shared/network/UpdateData.h>
#include <shared/network/UpdateFields.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Firelands {

namespace {

constexpr float kMmapGridSize = 533.3333f;
constexpr float kMmapNavMeshOrigin = -17066.66656f;

std::atomic<uint32_t> g_nextMmapMarkerLow{0x71000000u};

uint64_t AllocateMmapMarkerGuid(uint32_t creatureEntry) {
  uint32_t const low =
      g_nextMmapMarkerLow.fetch_add(1u, std::memory_order_relaxed);
  return MakeCreatureObjectGuid(creatureEntry, low);
}

std::string FormatVec3(Vec3 const &v) {
  std::ostringstream ss;
  ss << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return ss.str();
}

bool IsMmapSubcommand(std::string const &token, char const *name) {
  return AsciiEqualsLower(token, name);
}

std::pair<int32_t, int32_t> ComputeMmapGridTile(float x, float y) {
  int32_t gx = 32 - static_cast<int32_t>(x / kMmapGridSize);
  int32_t gy = 32 - static_cast<int32_t>(y / kMmapGridSize);
  return {gx, gy};
}

std::pair<int32_t, int32_t> ComputeMmapNavTile(float x, float y) {
  int32_t tileX = static_cast<int32_t>(std::floor((x - kMmapNavMeshOrigin) /
                                                  kMmapGridSize));
  int32_t tileY = static_cast<int32_t>(std::floor((y - kMmapNavMeshOrigin) /
                                                  kMmapGridSize));
  return {tileX, tileY};
}

bool HandleMmapLoadedTiles(std::shared_ptr<ICommandSession> const &session,
                           IMapCollisionQueries const &collision,
                           uint32_t mapId) {
  if (!collision.IsNavMeshDataAvailable(mapId)) {
    session->SendNotification("NavMesh not loaded for current map.");
    return true;
  }

  session->SendNotification("mmap loadedtiles:");
  auto tiles = collision.GetLoadedTiles(mapId);
  if (tiles.empty()) {
    session->SendNotification("  (none)");
    return true;
  }

  for (auto const &[tileX, tileY] : tiles) {
    session->SendNotification("[" + (tileX < 10 ? std::string("0") : std::string()) +
                              std::to_string(tileX) + ", " +
                              (tileY < 10 ? std::string("0") : std::string()) +
                              std::to_string(tileY) + "]");
  }
  return true;
}

bool HandleMmapLoc(std::shared_ptr<ICommandSession> const &session,
                   IMapCollisionQueries const &collision,
                   uint32_t mapId) {
  auto const &pos = session->GetPosition();
  session->SendNotification("mmap tileloc:");

  if (!collision.IsNavMeshDataAvailable(mapId)) {
    session->SendNotification("NavMesh not loaded for current map.");
    return true;
  }

  auto const [tileX, tileY] = ComputeMmapNavTile(pos.x, pos.y);
  session->SendNotification(std::to_string(mapId) + "_" +
                            std::to_string(tileX) + "_" +
                            std::to_string(tileY) + ".mmtile");
  session->SendNotification("tileloc [" +
                            (tileX < 10 ? std::string("0") : std::string()) +
                            std::to_string(tileX) + ", " +
                            (tileY < 10 ? std::string("0") : std::string()) +
                            std::to_string(tileY) + "]");

  auto const [gridX, gridY] = ComputeMmapGridTile(pos.x, pos.y);
  session->SendNotification("legacy [" + std::to_string(gridY) + ", " +
                            std::to_string(gridX) + "]");

  session->SendNotification("Calc   [" +
                            (tileX < 10 ? std::string("0") : std::string()) +
                            std::to_string(tileX) + ", " +
                            (tileY < 10 ? std::string("0") : std::string()) +
                            std::to_string(tileY) + "]");

  auto tiles = collision.GetLoadedTiles(mapId);
  bool const loaded = std::find(tiles.begin(), tiles.end(), std::make_pair(
                                         static_cast<uint32_t>(tileX),
                                         static_cast<uint32_t>(tileY))) !=
                      tiles.end();
  if (loaded)
    session->SendNotification("Dt     [" +
                              (tileX < 10 ? std::string("0") : std::string()) +
                              std::to_string(tileX) + "," +
                              (tileY < 10 ? std::string("0") : std::string()) +
                              std::to_string(tileY) + "]");
  else
    session->SendNotification("Dt     [??,??] (no tile loaded)");

  return true;
}

bool HandleMmapStats(std::shared_ptr<ICommandSession> const &session,
                     IMapCollisionQueries const &collision,
                     uint32_t mapId) {
  session->SendNotification("mmap stats:");
  session->SendNotification(std::string("  global mmap pathfinding is ") +
                            (collision.IsNavMeshDataAvailable(mapId) ? "enabled"
                                                                    : "disabled"));
  session->SendNotification(" " + std::to_string(collision.GetLoadedMapCount()) +
                            " maps loaded with " +
                            std::to_string(collision.GetLoadedTileCount()) +
                            " tiles overall");
  session->SendNotification("Navmesh stats:");
  session->SendNotification(" " + std::to_string(collision.GetLoadedTiles(mapId).size()) +
                            " tiles loaded");
  return true;
}

bool HandleMmapTestArea(std::shared_ptr<ICommandSession> const &session,
                        IMapCollisionQueries const &collision,
                        std::shared_ptr<Map> const &map,
                        uint32_t mapId) {
  if (!map) {
    session->SendNotification("MMAP: current map is not available.");
    return true;
  }

  float radius = 40.0f;
  float const cellRadius = 1.0f;
  float const playerX = session->GetPosition().x;
  float const playerY = session->GetPosition().y;
  float const playerZ = session->GetPosition().z;

  uint32_t creatureCount = 0;
  uint32_t pathCount = 0;
  auto const started = std::chrono::steady_clock::now();

  map->ForEachCreatureNear(playerX, playerY, static_cast<int>(cellRadius),
                           [&](std::shared_ptr<Creature> const &creature) {
                             ++creatureCount;
                             FindPathRequest req;
                             req.mapId = mapId;
                             req.startX = creature->GetX();
                             req.startY = creature->GetY();
                             req.startZ = creature->GetZ();
                             req.endX = playerX;
                             req.endY = playerY;
                             req.endZ = playerZ;
                             req.smoothPath = true;
                             req.allowPartialPath = true;
                             if (collision.FindPath(req).status != FindPathStatus::NavMeshMissing)
                               ++pathCount;
                           });

  auto const elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - started)
                           .count();

  if (creatureCount != 0) {
    session->SendNotification("Found " + std::to_string(creatureCount) +
                              " Creatures.");
    session->SendNotification("Generated " + std::to_string(pathCount) +
                              " paths in " + std::to_string(elapsed) + " ms");
  } else {
    session->SendNotification("No creatures in " + std::to_string(radius) +
                              " yard range.");
  }
  return true;
}

std::shared_ptr<Creature> ResolveMmapChaseCreature(
    std::shared_ptr<ICommandSession> const &session,
    std::shared_ptr<Map> const &map) {
  if (!map)
    return nullptr;

  uint64_t const targetGuid = session->GetClientSelectionGuid();
  if (targetGuid != 0) {
    if (auto selected = map->TryGetCreature(targetGuid))
      return selected;
  }

  MovementInfo const &playerPos = session->GetPosition();
  std::shared_ptr<Creature> nearest;
  float nearestDistSq = std::numeric_limits<float>::max();
  constexpr int kSearchCellRadius = 2;

  map->ForEachCreatureNear(
      playerPos.x, playerPos.y, kSearchCellRadius,
      [&](std::shared_ptr<Creature> const &creature) {
        float const dx = creature->GetX() - playerPos.x;
        float const dy = creature->GetY() - playerPos.y;
        float const dz = creature->GetZ() - playerPos.z;
        float const distSq = dx * dx + dy * dy + dz * dz;
        if (distSq < nearestDistSq) {
          nearestDistSq = distSq;
          nearest = creature;
        }
      });

  return nearest;
}

void SendMmapMarkerCreate(std::shared_ptr<ICommandSession> const &session,
                          uint32_t mapId, uint64_t markerGuid,
                          Vec3 const &pos, uint32_t entry,
                          uint32_t displayId,
                          uint32_t factionTemplate) {
  auto marker = std::make_shared<Creature>(markerGuid, entry, displayId, 100u, 1u,
                                            factionTemplate);
  marker->SetPosition(MovementInfo{.x = pos.x, .y = pos.y, .z = pos.z, .orientation = 0.0f});
  marker->SetCombatStats(BuildCreatureCombatStats(1u, 1u));
  WorldService::Instance().AddCreatureToMap(mapId, std::move(marker));

  MovementInfo move{};
  move.x = pos.x;
  move.y = pos.y;
  move.z = pos.z;
  move.orientation = 0.0f;

  UpdateData update(static_cast<uint16>(mapId));
  update.AddCreateObject(
      markerGuid, TYPEID_UNIT, move,
      WorldSessionObjectUpdate::BuildMinimalNpcUnitCreateFields(
          markerGuid, entry, displayId, 1u, 1u, 1u, 0u, factionTemplate));
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  session->SendPacket(pkt);
}

void SendMmapMarkerDespawn(std::shared_ptr<ICommandSession> const &session,
                           uint32_t mapId, uint64_t markerGuid) {
  WorldService::Instance().RemoveCreatureFromMap(mapId, markerGuid);
  // The background sweep may run when the player is offline: still remove the
  // creature from the map, but only push the out-of-range packet if a session is
  // there to receive it.
  if (!session)
    return;
  UpdateData update(static_cast<uint16>(mapId));
  update.AddOutOfRangeObjects({markerGuid});
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  session->SendPacket(pkt);
}

} // namespace

bool MmapDebugCommands::Handle(std::shared_ptr<ICommandSession> session,
                               const std::vector<std::string> &args,
                               PrivilegeOrigin origin,
                               OnlineCharacterSessionRegistry *online) {
  (void)origin;
  auto collision = WorldService::Instance().GetCollisionQueries();
  if (!collision) {
    LOG_MMAP_ERROR("[MMAP] collision service is not configured.");
    session->SendNotification("MMAP: collision service is not configured.");
    return true;
  }

  MovementInfo const &playerPos = session->GetPosition();
  uint32_t mapId = session->GetMapId();
  uint64_t const playerGuid = session->GetActiveCharacterObjectGuid();
  auto map = WorldService::Instance().GetMap(mapId);

  LOG_MMAP_DEBUG("[MMAP] request: playerGuid={} mapId={} args={}", playerGuid, mapId,
            args.empty() ? std::string("<target>") : JoinArgs(args.begin(), args.end()));

  if (!args.empty()) {
    if (IsMmapSubcommand(args[0], "loadedtiles"))
      return HandleMmapLoadedTiles(session, *collision, mapId);
    if (IsMmapSubcommand(args[0], "loc"))
      return HandleMmapLoc(session, *collision, mapId);
    if (IsMmapSubcommand(args[0], "stats"))
      return HandleMmapStats(session, *collision, mapId);
    if (IsMmapSubcommand(args[0], "testarea"))
      return HandleMmapTestArea(session, *collision, map, mapId);
  }

  // Auto-remove expired markers (older than 9s). The background sweep in
  // PollScheduledRestart does this on time without another .mmap call; this keeps
  // it prompt on the next invocation too.
  SweepExpiredMmapMarkers(online);

  // .mmap clear — remove visual markers
  if (!args.empty() && args[0] == "clear") {
    LOG_MMAP_DEBUG("[MMAP] clear: playerGuid={} mapId={}", playerGuid, mapId);
    ClearMmapMarkers(session, playerGuid, mapId);
    session->SendNotification("[MMAP] visual markers removed.");
    return true;
  }

  std::vector<std::string> pathArgs = args;
  bool const chasePath =
      !pathArgs.empty() && IsMmapSubcommand(pathArgs[0], "chase");
  if (!pathArgs.empty() &&
      (IsMmapSubcommand(pathArgs[0], "path") ||
       IsMmapSubcommand(pathArgs[0], "chase")))
    pathArgs.erase(pathArgs.begin());

  // Determine start/end positions for pathfinding
  Vec3 startPos{playerPos.x, playerPos.y, playerPos.z};
  Vec3 endPos{playerPos.x, playerPos.y, playerPos.z};
  bool checkPath = false;
  std::string pathLabel;

  try {
    if (!pathArgs.empty() || chasePath ||
        (!args.empty() && IsMmapSubcommand(args[0], "path"))) {
      bool const numericArgs = pathArgs.size() >= 3 &&
                               IsAllDigitAscii(pathArgs[0].empty() ? std::string() : pathArgs[0]);
      if (pathArgs.size() < 3) {
        if (!IsMmapSubcommand(args[0], "path") && !chasePath) {
          LOG_MMAP_WARN("MMAP invalid coordinates: playerGuid={} mapId={} argCount={}",
                   playerGuid, mapId, pathArgs.size());
          session->SendNotification("Usage: .mmap [x y z [mapId]]  |  .mmap path  |  .mmap chase  |  .mmap loc  |  .mmap stats  |  .mmap loadedtiles  |  .mmap testarea  |  .mmap clear");
          return false;
        }
      }
      if (pathArgs.size() >= 3 && numericArgs) {
        endPos.x = std::stof(pathArgs[0]);
        endPos.y = std::stof(pathArgs[1]);
        endPos.z = std::stof(pathArgs[2]);
        if (pathArgs.size() > 3)
          mapId = static_cast<uint32_t>(std::stoul(pathArgs[3]));
        pathLabel = "you -> " + FormatVec3(endPos);
        checkPath = true;
      } else if (IsMmapSubcommand(args[0], "path") || chasePath ||
                 pathArgs.empty()) {
        auto creature = chasePath ? ResolveMmapChaseCreature(session, map)
                                  : nullptr;
        if (!creature) {
          uint64_t const targetGuid = session->GetClientSelectionGuid();
          if (targetGuid != 0 && map)
            creature = map->TryGetCreature(targetGuid);
        }
        if (creature) {
          MovementInfo const &crPos = creature->GetPosition();
          startPos = Vec3{crPos.x, crPos.y, crPos.z};
          endPos = Vec3{playerPos.x, playerPos.y, playerPos.z};
          pathLabel = std::string("creature(") +
                      std::to_string(creature->GetEntry()) + ") -> you";
          checkPath = true;
          LOG_MMAP_DEBUG("MMAP chase creature resolved: playerGuid={} entry={} mapId={} start=({}, {}, {})",
                    playerGuid, creature->GetEntry(), mapId, crPos.x, crPos.y,
                    crPos.z);
        }
        if (!checkPath) {
          session->SendNotification(
              chasePath ? "MMAP: no nearby creature found for chase path."
                        : "MMAP: targeted object is not a creature.");
        }
      }
      if (!checkPath && pathArgs.size() >= 3 && !numericArgs) {
        LOG_MMAP_WARN("MMAP invalid coordinates: playerGuid={} mapId={} argCount={}",
                 playerGuid, mapId, pathArgs.size());
        session->SendNotification("Usage: .mmap [x y z [mapId]]  |  .mmap path  |  .mmap chase  |  .mmap loc  |  .mmap stats  |  .mmap loadedtiles  |  .mmap testarea  |  .mmap clear");
        return false;
      }
    }
  } catch (std::exception const &) {
    LOG_MMAP_ERROR("MMAP invalid coordinates parse error: playerGuid={} mapId={} args={}",
              playerGuid, mapId,
              pathArgs.empty() ? std::string("<target>") : JoinArgs(pathArgs.begin(), pathArgs.end()));
    session->SendNotification("MMAP: invalid coordinates.");
    return false;
  }

  bool const available = collision->IsNavMeshDataAvailable(mapId);
  LOG_MMAP_DEBUG("MMAP navmesh availability: mapId={} available={}", mapId, available);
  float const height = collision->GetHeight(mapId, playerPos.x, playerPos.y, playerPos.z);
  std::ostringstream head;
  head << "MMAP map=" << mapId << " navmesh=" << (available ? "loaded" : "missing")
       << " player=" << FormatVec3(Vec3{playerPos.x, playerPos.y, playerPos.z})
       << " height=" << height;
  session->SendNotification(head.str());

  if (!checkPath)
    return true;

  FindPathRequest req;
  req.mapId = mapId;
  req.startX = startPos.x;
  req.startY = startPos.y;
  req.startZ = startPos.z;
  req.endX = endPos.x;
  req.endY = endPos.y;
  req.endZ = endPos.z;
  req.smoothPath = true;
  req.allowPartialPath = true;

  FindPathResult result = collision->FindPath(req);
  LOG_MMAP_DEBUG("MMAP path result: mapId={} status={} waypoints={}", mapId,
            FindPathStatusName(result.status), result.waypoints.size());
  std::ostringstream path;
  path << "MMAP path " << FormatVec3(startPos) << " -> "
       << FormatVec3(endPos) << "  (" << pathLabel << ")"
       << " status=" << FindPathStatusName(result.status)
       << " waypoints=" << result.waypoints.size();
  session->SendNotification(path.str());

  constexpr size_t kMaxPrintedWaypoints = 8;
  for (size_t i = 0; i < result.waypoints.size() && i < kMaxPrintedWaypoints; ++i) {
    session->SendNotification("  wp[" + std::to_string(i) + "] " +
                              FormatVec3(result.waypoints[i]));
  }
  if (result.waypoints.size() > kMaxPrintedWaypoints) {
    session->SendNotification("  ... " +
                              std::to_string(result.waypoints.size() -
                                             kMaxPrintedWaypoints) +
                              " more waypoint(s)");
  }

  // Spawn visual markers at each waypoint (auto-despawn after 9s)
  ClearMmapMarkers(session, playerGuid, mapId);

  if (!result.waypoints.empty()) {
    constexpr uint32_t kMarkerEntry = 1u;
    uint32_t kMarkerDisplayId = 15688u;
    if (auto const repo = WorldService::Instance().GetNpcTemplateSearch()) {
      if (auto const tpl = repo->TryGetByEntry(kMarkerEntry)) {
        kMarkerDisplayId = ResolveCreatureDisplayId(
            0, tpl->displayIds[0], tpl->displayIds[1], tpl->displayIds[2],
            tpl->displayIds[3]);
      }
    }
    // Float the marker well above a player model so it's visible even when
    // start ≈ end and the waypoint lands inside the caster or the target.
    constexpr float kMarkerVisualLiftYards = 3.0f;
    auto const now = std::chrono::steady_clock::now();
    std::vector<std::pair<uint64_t, std::chrono::steady_clock::time_point>> markers;
    markers.reserve(result.waypoints.size());

    for (size_t i = 0; i < result.waypoints.size(); ++i) {
      auto const &wp = result.waypoints[i];
      // Re-ground each waypoint Z before placing the marker. Detour's
      // findStraightPath returns Z from the navmesh corridor, and on
      // corrupted/ghost mmtiles that Z can be tens of yards above the real
      // floor. Anchoring to the live GetHeight (which goes through the
      // floor-under-(x,y) query) keeps markers visible on the ground.
      float const groundZ =
          collision->GetHeight(mapId, wp.x, wp.y, wp.z);
      float const z = groundZ + kMarkerVisualLiftYards;
      uint64_t const markerGuid = AllocateMmapMarkerGuid(kMarkerEntry);
      SendMmapMarkerCreate(session, mapId, markerGuid,
                           Vec3{wp.x, wp.y, z}, kMarkerEntry,
                           kMarkerDisplayId, Creature::kDefaultFactionTemplate);
      LOG_MMAP_DEBUG(
          "MMAP marker spawn: mapId={} guid={} wpIndex={} wp=({}, {}, {}) "
          "groundZ={} placedZ={}",
          mapId, markerGuid, i, wp.x, wp.y, wp.z, groundZ, z);
      markers.emplace_back(markerGuid, now);
    }
    {
      std::lock_guard<std::mutex> lock(_mmapMarkersMutex);
      auto &set = _mmapMarkers[playerGuid];
      set.mapId = mapId;
      set.markers = std::move(markers);
    }
    session->SendNotification("MMAP: " +
                              std::to_string(result.waypoints.size()) +
                              " marker(s) spawned (despawn in 9s). |cffffffff.mmap clear|r to remove earlier.");
  } else {
    session->SendNotification(
        "MMAP: no waypoints to mark (start and end resolved to the same "
        "navmesh point).");
  }
  return true;
}

void MmapDebugCommands::ClearMmapMarkers(std::shared_ptr<ICommandSession> session,
                                         uint64_t playerGuid, uint32_t mapId) {
  (void)mapId;
  std::lock_guard<std::mutex> lock(_mmapMarkersMutex);
  auto it = _mmapMarkers.find(playerGuid);
  if (it == _mmapMarkers.end())
    return;

  // Remove all markers (despawn on the map they were spawned on).
  for (auto &[guid, spawnTime] : it->second.markers) {
    (void)spawnTime;
    SendMmapMarkerDespawn(session, it->second.mapId, guid);
  }
  _mmapMarkers.erase(it);
}

void MmapDebugCommands::SweepExpiredMmapMarkers(
    OnlineCharacterSessionRegistry *online) {
  std::lock_guard<std::mutex> lock(_mmapMarkersMutex);
  if (_mmapMarkers.empty())
    return;
  auto const now = std::chrono::steady_clock::now();
  for (auto it = _mmapMarkers.begin(); it != _mmapMarkers.end();) {
    std::shared_ptr<ICommandSession> const session =
        online ? online->TryResolveByObjectGuid(it->first) : nullptr;
    auto &set = it->second;
    set.markers.erase(
        std::remove_if(set.markers.begin(), set.markers.end(),
                       [&](auto const &p) {
                         if (now - p.second > std::chrono::seconds(9)) {
                           SendMmapMarkerDespawn(session, set.mapId, p.first);
                           return true;
                         }
                         return false;
                       }),
        set.markers.end());
    if (set.markers.empty())
      it = _mmapMarkers.erase(it);
    else
      ++it;
  }
}

} // namespace Firelands

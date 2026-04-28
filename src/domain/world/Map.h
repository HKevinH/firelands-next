#pragma once

#include <domain/world/WorldObject.h>
#include <memory>
#include <mutex>
#include <shared/Common.h>
#include <unordered_map>
#include <vector>

namespace Firelands {

class WorldSession;

class Map {
public:
  explicit Map(uint32 id) : m_id(id) {}

  void AddObject(std::shared_ptr<WorldObject> obj);
  void RemoveObject(uint64 guid);

  void UpdateObjectPosition(uint64 guid, const MovementInfo &pos);

  void BroadcastPacket(uint64 senderGuid, class WorldPacket &packet,
                       bool includeSelf = false);
  void BroadcastPacketToNearby(std::shared_ptr<WorldObject> sender,
                               class WorldPacket &packet,
                               bool includeSelf = false);
  void BroadcastPacketToNearby(uint64 senderGuid, class WorldPacket &packet,
                               bool includeSelf = false);

private:
  struct GridCoord {
    int x, y;
    bool operator==(const GridCoord &other) const {
      return x == other.x && y == other.y;
    }
  };

  struct Cell {
    std::unordered_map<uint64, std::shared_ptr<WorldObject>> objects;
  };

  GridCoord GetCoord(float x, float y);

  uint32 m_id;
  std::unordered_map<uint64, std::shared_ptr<WorldObject>> m_objects;
  std::unordered_map<uint64, GridCoord> m_objectCoords;
  Cell m_grid[64][64];
  std::mutex m_mapMutex;
};

} // namespace Firelands

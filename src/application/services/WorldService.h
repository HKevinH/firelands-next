#pragma once

#include <domain/world/Map.h>
#include <domain/world/Player.h>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace Firelands {

class WorldService {
public:
  static WorldService &Instance() {
    static WorldService instance;
    return instance;
  }

  std::shared_ptr<Map> GetMap(uint32 mapId) {
    std::lock_guard<std::mutex> lock(m_worldMutex);
    auto it = m_maps.find(mapId);
    if (it == m_maps.end()) {
      m_maps[mapId] = std::make_shared<Map>(mapId);
    }
    return m_maps[mapId];
  }

  void AddPlayerToMap(uint32 mapId, std::shared_ptr<Player> player) {
    GetMap(mapId)->AddObject(player);
  }

  void RemovePlayerFromMap(uint32 mapId, uint64 guid) {
    GetMap(mapId)->RemoveObject(guid);
  }

private:
  WorldService() = default;
  std::unordered_map<uint32, std::shared_ptr<Map>> m_maps;
  std::mutex m_worldMutex;
};

} // namespace Firelands

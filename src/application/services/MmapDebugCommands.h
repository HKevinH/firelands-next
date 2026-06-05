#ifndef FIRELANDS_APPLICATION_SERVICES_MMAP_DEBUG_COMMANDS_H
#define FIRELANDS_APPLICATION_SERVICES_MMAP_DEBUG_COMMANDS_H

#include <application/ports/ICommandSession.h>
#include <shared/game/AccessLevel.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Firelands {

class OnlineCharacterSessionRegistry;

/// Handles the `.mmap` GM debug command: navmesh stats / tile inspection,
/// pathfinding queries, and the per-player visual path markers. Extracted from
/// CommandService so the marker state and packet plumbing live with the feature
/// instead of bloating the command router.
class MmapDebugCommands {
public:
  /// Runs `.mmap <args>`. Returns false on usage errors (mirrors the command
  /// handler contract). `online` resolves marker owners during the prompt sweep.
  bool Handle(std::shared_ptr<ICommandSession> session,
              const std::vector<std::string> &args, PrivilegeOrigin origin,
              OnlineCharacterSessionRegistry *online);

  /// Despawn `.mmap` path markers older than 9s. Called from the main loop so
  /// markers vanish on time even without another `.mmap` call. `online` resolves
  /// the owning player's session to push the despawn packet (may be offline).
  void SweepExpiredMmapMarkers(OnlineCharacterSessionRegistry *online);

private:
  void ClearMmapMarkers(std::shared_ptr<ICommandSession> session,
                        uint64_t playerGuid, uint32_t mapId);

  /// .mmap path visual markers per player (key = player object guid). mapId is
  /// stored so the background sweep can despawn them without a live session map.
  struct MmapMarkerSet {
    uint32_t mapId = 0;
    std::vector<std::pair<uint64_t, std::chrono::steady_clock::time_point>>
        markers;
  };
  std::mutex _mmapMarkersMutex;
  std::unordered_map<uint64_t, MmapMarkerSet> _mmapMarkers;
};

} // namespace Firelands

#endif // FIRELANDS_APPLICATION_SERVICES_MMAP_DEBUG_COMMANDS_H

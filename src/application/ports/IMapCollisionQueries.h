#ifndef FIRELANDS_APPLICATION_PORTS_I_MAP_COLLISION_QUERIES_H
#define FIRELANDS_APPLICATION_PORTS_I_MAP_COLLISION_QUERIES_H

#include <cstdint>

namespace Firelands {

/// Port for mmap/vmap-backed queries (LoS, height, navmesh). Stub implementation
/// returns permissive values until extracted data is wired.
class IMapCollisionQueries {
public:
  virtual ~IMapCollisionQueries() = default;

  /// When false, callers should assume no navmesh files are loaded for `mapId`.
  virtual bool IsNavMeshDataAvailable(uint32_t mapId) const = 0;

  /// Line of sight between two world points. Stub returns true (open line).
  virtual bool LineOfSight(uint32_t mapId, float x0, float y0, float z0,
                           float x1, float y1, float z1) const = 0;
};

} // namespace Firelands

#endif

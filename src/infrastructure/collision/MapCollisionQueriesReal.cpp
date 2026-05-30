#include <infrastructure/collision/MapCollisionQueriesReal.h>

namespace Firelands {

MapCollisionQueriesReal::MapCollisionQueriesReal(std::string dataRoot)
    : _navMeshManager(std::move(dataRoot)) {}

bool MapCollisionQueriesReal::IsNavMeshDataAvailable(uint32_t mapId) const {
  if (_navMeshManager.IsNavMeshLoaded(mapId))
    return true;
  return _navMeshManager.LoadMapNavMesh(mapId);
}

bool MapCollisionQueriesReal::LineOfSight(uint32_t /*mapId*/, float x0,
                                           float y0, float z0, float x1,
                                           float y1, float z1) const {
  // TODO: Integrate VMapManager2 for real LoS checks
  (void)x0; (void)y0; (void)z0;
  (void)x1; (void)y1; (void)z1;
  return true;
}

FindPathResult MapCollisionQueriesReal::FindPath(
    FindPathRequest const& req) const {
  if (!_navMeshManager.IsNavMeshLoaded(req.mapId)) {
    if (!_navMeshManager.LoadMapNavMesh(req.mapId)) {
      FindPathResult result;
      result.status = FindPathStatus::NavMeshMissing;
      return result;
    }
  }

  return _navMeshManager.FindPath(req);
}

float MapCollisionQueriesReal::GetHeight(uint32_t /*mapId*/, float /*x*/,
                                          float /*y*/, float zHint) const {
  // TODO: Integrate VMapManager2 for real height queries
  return zHint;
}

} // namespace Firelands

#include <infrastructure/world/MapCollisionQueriesStub.h>

namespace Firelands {

MapCollisionQueriesStub::MapCollisionQueriesStub(std::string dataRoot)
    : _dataRoot(std::move(dataRoot)) {}

bool MapCollisionQueriesStub::IsNavMeshDataAvailable(uint32_t /*mapId*/) const {
  return !_dataRoot.empty();
}

bool MapCollisionQueriesStub::LineOfSight(uint32_t /*mapId*/, float /*x0*/,
                                           float /*y0*/, float /*z0*/,
                                           float /*x1*/, float /*y1*/,
                                           float /*z1*/) const {
  (void)_dataRoot;
  return true;
}

} // namespace Firelands

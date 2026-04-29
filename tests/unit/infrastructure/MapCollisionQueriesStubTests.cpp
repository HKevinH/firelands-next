#include <gtest/gtest.h>
#include <infrastructure/world/MapCollisionQueriesStub.h>

using namespace Firelands;

TEST(MapCollisionQueriesStub, LineOfSightAlwaysOpen) {
  MapCollisionQueriesStub stub("");
  EXPECT_TRUE(stub.LineOfSight(0, 0, 0, 0, 10, 10, 10));
}

TEST(MapCollisionQueriesStub, NavMeshUnavailableWhenDataRootEmpty) {
  MapCollisionQueriesStub empty("");
  EXPECT_FALSE(empty.IsNavMeshDataAvailable(530));

  MapCollisionQueriesStub withData("/tmp/maps");
  EXPECT_TRUE(withData.IsNavMeshDataAvailable(0));
}

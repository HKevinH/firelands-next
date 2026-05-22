#include <gtest/gtest.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionMovementChecks.h>

using namespace Firelands;

TEST(WorldSessionMovementChecksTest, AdoptsSmallPlausibleStep) {
  MovementInfo current{};
  current.x = 0.f;
  current.y = 0.f;
  current.z = 0.f;
  current.time = 1000u;

  MovementInfo parsed{};
  parsed.x = 2.f;
  parsed.y = 0.f;
  parsed.z = 0.f;
  parsed.time = 1200u;

  EXPECT_TRUE(WsTryAdoptParsedMovementPosition(current, parsed, 7.f));
  EXPECT_FLOAT_EQ(current.x, 2.f);
}

TEST(WorldSessionMovementChecksTest, RejectsTeleportSizedStep) {
  MovementInfo current{};
  current.x = 0.f;
  current.y = 0.f;
  current.z = 0.f;
  current.time = 1000u;

  MovementInfo parsed{};
  parsed.x = 50.f;
  parsed.y = 0.f;
  parsed.z = 0.f;
  parsed.time = 1100u;

  EXPECT_FALSE(WsTryAdoptParsedMovementPosition(current, parsed, 7.f));
  EXPECT_FLOAT_EQ(current.x, 0.f);
}

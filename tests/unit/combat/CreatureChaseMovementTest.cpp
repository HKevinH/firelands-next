#include <application/combat/CreatureChaseMovement.h>
#include <gtest/gtest.h>
#include <shared/network/MovementFlags.h>

namespace {

Firelands::MovementInfo MakePos(float x, float y, float z) {
  Firelands::MovementInfo m{};
  m.x = x;
  m.y = y;
  m.z = z;
  return m;
}

} // namespace

TEST(CreatureChaseMovementTest, StepsTowardTargetAtConfiguredSpeed) {
  application::combat::CreatureChaseConfig config{};
  config.runSpeedYardsPerSec = 10.0f;
  config.stopDistanceYards = 2.0f;

  auto const result = application::combat::StepCreatureTowardTarget(
      MakePos(0.f, 0.f, 0.f), 20.f, 0.f, 0.f, 0.5f, config);

  EXPECT_TRUE(result.moved);
  EXPECT_FALSE(result.inStopRange);
  EXPECT_NEAR(result.position.x, 5.f, 0.05f);
  EXPECT_EQ(result.position.flags, Firelands::MOVEMENTFLAG_FORWARD);
}

TEST(CreatureChaseMovementTest, StopsWithinMeleeStopDistance) {
  application::combat::CreatureChaseConfig config{};
  config.stopDistanceYards = 2.0f;

  auto const result = application::combat::StepCreatureTowardTarget(
      MakePos(0.f, 0.f, 0.f), 1.5f, 0.f, 0.f, 1.0f, config);

  EXPECT_FALSE(result.moved);
  EXPECT_TRUE(result.inStopRange);
  EXPECT_EQ(result.position.flags, Firelands::MOVEMENTFLAG_NONE);
}

TEST(CreatureChaseMovementTest, ProjectionAdvancesFartherThanSingleTick) {
  application::combat::CreatureChaseConfig config{};
  config.runSpeedYardsPerSec = 7.0f;
  config.stopDistanceYards = 2.0f;

  auto const single = application::combat::StepCreatureTowardTarget(
      MakePos(0.f, 0.f, 0.f), 30.f, 0.f, 0.f, 0.2f, config);
  auto const projected = application::combat::ProjectCreatureTowardTarget(
      MakePos(0.f, 0.f, 0.f), 30.f, 0.f, 0.f, 1.5f, config);

  EXPECT_TRUE(projected.moved);
  EXPECT_GT(projected.position.x, single.position.x);
}

TEST(CreatureChaseMovementTest, ChaseTargetRelocatedDetectsMeaningfulMove) {
  EXPECT_FALSE(application::combat::ChaseTargetRelocated(0.f, 0.f, 0.f, 0.2f, 0.f, 0.f));
  EXPECT_TRUE(application::combat::ChaseTargetRelocated(0.f, 0.f, 0.f, 2.f, 0.f, 0.f));
}

TEST(CreatureChaseMovementTest, StandPositionIsStopDistanceFromTarget) {
  auto const stand = application::combat::ComputeChaseStandPosition(
      MakePos(10.f, 0.f, 0.f), 0.f, 0.f, 0.f, 1.0f);
  EXPECT_NEAR(stand.x, 1.0f, 0.01f);
  EXPECT_NEAR(stand.y, 0.f, 0.01f);
}

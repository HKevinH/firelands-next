#include <gtest/gtest.h>
#include <shared/game/MeleeRange.h>

TEST(MeleeRangeTest, DefaultReachIsEightPointFiveYards) {
  EXPECT_FLOAT_EQ(
      Firelands::MeleeRangeMaxYards(Firelands::kDefaultUnitCombatReachYards,
                                    Firelands::kDefaultUnitCombatReachYards),
      8.5f);
  EXPECT_FLOAT_EQ(Firelands::MeleeRangeMaxSquared2d(
                      Firelands::kDefaultUnitCombatReachYards,
                      Firelands::kDefaultUnitCombatReachYards),
                  8.5f * 8.5f);
}

TEST(MeleeRangeTest, RejectsBeyondDefaultMeleeRange) {
  EXPECT_TRUE(Firelands::IsWithinMeleeRange2d(0.f, 0.f, 8.f, 0.f));
  EXPECT_FALSE(Firelands::IsWithinMeleeRange2d(0.f, 0.f, 8.6f, 0.f));
}

TEST(MeleeRangeTest, VerticalSlopAllowsSmallHeightDelta) {
  EXPECT_TRUE(Firelands::IsWithinMeleeRange3d(0.f, 0.f, 0.f, 0.f, 0.f, 2.5f));
  EXPECT_FALSE(Firelands::IsWithinMeleeRange3d(0.f, 0.f, 0.f, 0.f, 0.f, 4.f));
}

TEST(MeleeRangeTest, NpcToleranceAllowsMinorSplineDelay) {
  // 9 yd is inside 8.5 + 0.5 NPC slop; 10 yd is beyond it.
  EXPECT_FALSE(Firelands::IsWithinMeleeRange2d(0.f, 0.f, 10.f, 0.f));
  EXPECT_FALSE(Firelands::IsWithinMeleeRangeAgainstNpc(0.f, 0.f, 0.f, 10.f, 0.f, 0.f));
  EXPECT_TRUE(Firelands::IsWithinMeleeRangeAgainstNpc(0.f, 0.f, 0.f, 9.f, 0.f, 0.f));
}

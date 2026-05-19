#include <gtest/gtest.h>
#include <shared/game/SpellEffectMagnitude.h>

using namespace Firelands;
using namespace SpellEffectMagnitude;

TEST(SpellEffectMagnitudeTests, NeutralMagnitudeNegativeBasePointsLikeRejuvenation) {
  EXPECT_EQ(NeutralMagnitude(-5, 0), 5);
  EXPECT_EQ(NeutralMagnitude(-4, 0), 4);
}

TEST(SpellEffectMagnitudeTests, NeutralMagnitudePositiveBasePoints) {
  EXPECT_EQ(NeutralMagnitude(4, 0), 5);
  EXPECT_EQ(NeutralMagnitude(9, 1), 11);
}

TEST(SpellEffectMagnitudeTests, PeriodicHealTickMatchesNeutral) {
  EXPECT_EQ(PeriodicHealTick(-5, 0), 5);
  EXPECT_GT(PeriodicHealTick(4, 0), 0);
}

TEST(SpellEffectMagnitudeTests, PeriodicDamageTickIsNegative) {
  EXPECT_EQ(PeriodicDamageTick(-3, 0), -3);
  EXPECT_EQ(PeriodicDamageTick(4, 0), -5);
}

TEST(SpellEffectMagnitudeTests, SignedImmediateHealUsesNeutral) {
  constexpr uint32 kHeal = 10u;
  EXPECT_EQ(SignedImmediateHealthDelta(kHeal, -4, 0), 4);
  EXPECT_EQ(SignedImmediateHealthDelta(kHeal, 4, 0), 5);
}

TEST(SpellEffectMagnitudeTests, PeriodicHealTickAtLevelAddsRealPointsPerLevel) {
  EXPECT_EQ(PeriodicHealTickAtLevel(0, 0, 1.5f, 80), 120);
  EXPECT_EQ(PeriodicHealTickAtLevel(-4, 0, 0.f, 80), 4);
  EXPECT_EQ(PeriodicHealTickAtLevel(0, 0, 0.f, 80), 1);
}

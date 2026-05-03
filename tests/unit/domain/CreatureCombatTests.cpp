#include <gtest/gtest.h>
#include <domain/world/Creature.h>

using namespace Firelands;

TEST(CreatureCombatTests, DefaultConstructor_FullHealth) {
  Creature c(1ull, 1u, 1u);
  EXPECT_EQ(c.GetLiveMaxHealth(), 100u);
  EXPECT_EQ(c.GetLiveHealth(), 100u);
}

TEST(CreatureCombatTests, CustomMaxHealth_InitializedFull) {
  Creature c(1ull, 1u, 1u, 250u);
  EXPECT_EQ(c.GetLiveMaxHealth(), 250u);
  EXPECT_EQ(c.GetLiveHealth(), 250u);
}

TEST(CreatureCombatTests, ApplyHealthDelta_Clamped) {
  Creature c(2ull, 1u, 1u, 80u);
  c.ApplyHealthDelta(-200);
  EXPECT_EQ(c.GetLiveHealth(), 0u);
  c.ApplyHealthDelta(500);
  EXPECT_EQ(c.GetLiveHealth(), 80u);
}

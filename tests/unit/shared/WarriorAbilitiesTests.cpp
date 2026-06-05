#include <gtest/gtest.h>
#include <shared/game/WarriorAbilities.h>

using namespace Firelands;

TEST(WarriorAbilitiesTests, IdentifiesChargeSpell) {
  EXPECT_TRUE(IsChargeSpell(kSpellCharge));
  EXPECT_FALSE(IsChargeSpell(kSpellChargeStun));
  EXPECT_FALSE(IsChargeSpell(7384u));
}

TEST(WarriorAbilitiesTests, ChargeDataHasStunAndRage) {
  uint32 stun = 0u;
  int32 rage = 0;
  ASSERT_TRUE(TryGetWarriorChargeData(kSpellCharge, stun, rage));
  EXPECT_EQ(stun, kSpellChargeStun);
  EXPECT_EQ(rage, kChargeRageGain);
  EXPECT_GT(rage, 0);

  EXPECT_FALSE(TryGetWarriorChargeData(1680u, stun, rage));
}

TEST(WarriorAbilitiesTests, ChargeStunIsAStunSpell) {
  EXPECT_TRUE(IsWarriorStunSpell(kSpellChargeStun));
  EXPECT_FALSE(IsWarriorStunSpell(kSpellCharge));
}

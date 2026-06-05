#include <gtest/gtest.h>
#include <shared/game/ShapeshiftForms.h>

using namespace Firelands;

TEST(ShapeshiftFormsTests, StanceMaskFromForm) {
  EXPECT_EQ(StanceMaskFromForm(FORM_NONE), 0u);
  EXPECT_EQ(StanceMaskFromForm(FORM_BATTLESTANCE), 1u << 16);
  EXPECT_EQ(StanceMaskFromForm(FORM_DEFENSIVESTANCE), 1u << 17);
  EXPECT_EQ(StanceMaskFromForm(FORM_BERSERKERSTANCE), 1u << 18);
}

TEST(ShapeshiftFormsTests, StanceSpellRoundTrip) {
  EXPECT_TRUE(IsWarriorStanceSpell(kSpellBattleStance));
  EXPECT_TRUE(IsWarriorStanceSpell(kSpellDefensiveStance));
  EXPECT_TRUE(IsWarriorStanceSpell(kSpellBerserkerStance));
  EXPECT_FALSE(IsWarriorStanceSpell(7384u));

  EXPECT_EQ(WarriorStanceFormForSpell(kSpellBattleStance), FORM_BATTLESTANCE);
  EXPECT_EQ(WarriorStanceFormForSpell(kSpellDefensiveStance), FORM_DEFENSIVESTANCE);
  EXPECT_EQ(WarriorStanceFormForSpell(kSpellBerserkerStance), FORM_BERSERKERSTANCE);
  EXPECT_EQ(WarriorStanceFormForSpell(7384u), FORM_NONE);

  EXPECT_EQ(StanceSpellForForm(FORM_BATTLESTANCE), kSpellBattleStance);
  EXPECT_EQ(StanceSpellForForm(FORM_DEFENSIVESTANCE), kSpellDefensiveStance);
  EXPECT_EQ(StanceSpellForForm(FORM_BERSERKERSTANCE), kSpellBerserkerStance);
  EXPECT_EQ(StanceSpellForForm(FORM_NONE), 0u);
}

TEST(ShapeshiftFormsTests, AbilityStanceRequirement) {
  uint32 stances = 0u;
  uint32 stancesNot = 0u;

  EXPECT_TRUE(TryGetWarriorAbilityStanceRequirement(7384u, stances, stancesNot));
  EXPECT_EQ(stances, StanceMaskFromForm(FORM_BATTLESTANCE));
  EXPECT_EQ(stancesNot, 0u);

  EXPECT_TRUE(TryGetWarriorAbilityStanceRequirement(871u, stances, stancesNot));
  EXPECT_EQ(stances, 0u);
  EXPECT_EQ(stancesNot, StanceMaskFromForm(FORM_BERSERKERSTANCE));

  EXPECT_FALSE(TryGetWarriorAbilityStanceRequirement(kSpellBattleStance, stances, stancesNot));
}

TEST(ShapeshiftFormsTests, RageRetainedOnStanceSwitchResetsToZero) {
  EXPECT_EQ(RageRetainedOnStanceSwitch(0u), 0u);
  EXPECT_EQ(RageRetainedOnStanceSwitch(1000u), 0u);
}

TEST(ShapeshiftFormsTests, StanceDamageModsByForm) {
  int32 done = 0;
  int32 taken = 0;

  EXPECT_FALSE(GetWarriorStanceDamageMods(FORM_BATTLESTANCE, done, taken));
  EXPECT_EQ(done, 0);
  EXPECT_EQ(taken, 0);

  ASSERT_TRUE(GetWarriorStanceDamageMods(FORM_DEFENSIVESTANCE, done, taken));
  EXPECT_LT(done, 0);  // deals less
  EXPECT_LT(taken, 0); // takes less

  ASSERT_TRUE(GetWarriorStanceDamageMods(FORM_BERSERKERSTANCE, done, taken));
  EXPECT_GT(done, 0);  // deals more
  EXPECT_GT(taken, 0); // takes more
}

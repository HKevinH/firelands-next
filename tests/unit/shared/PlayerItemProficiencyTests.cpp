#include <gtest/gtest.h>
#include <shared/game/PlayerItemProficiency.h>

using namespace Firelands;

TEST(PlayerItemProficiencyTests, WeaponMaskFromStarterSkills) {
  std::vector<StarterSkillGrant> skills{{43u, 1, 1}, {55u, 1, 1}};
  uint32_t const mask = ComputeWeaponProficiencyMask(skills);
  EXPECT_NE(mask, 0u);
  EXPECT_NE(mask, 0xFFFFFFFFu);
}

TEST(PlayerItemProficiencyTests, ArmorMaskFromPlateSkill) {
  std::vector<StarterSkillGrant> skills{{293u, 1, 1}};
  uint32_t const mask = ComputeArmorProficiencyMask(skills);
  EXPECT_NE(mask & (1u << 4u), 0u);
}

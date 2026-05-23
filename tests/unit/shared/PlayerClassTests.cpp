#include <gtest/gtest.h>
#include <shared/game/PlayerClass.h>
#include <shared/game/QuestMask.h>

using namespace Firelands;

TEST(PlayerClassTests, ChrClassesIdsMatchBlizzard) {
  EXPECT_EQ(ToClassId(PlayerClass::Warrior), 1u);
  EXPECT_EQ(ToClassId(PlayerClass::Paladin), 2u);
  EXPECT_EQ(ToClassId(PlayerClass::Hunter), 3u);
  EXPECT_EQ(ToClassId(PlayerClass::Rogue), 4u);
  EXPECT_EQ(ToClassId(PlayerClass::Priest), 5u);
  EXPECT_EQ(ToClassId(PlayerClass::DeathKnight), 6u);
  EXPECT_EQ(ToClassId(PlayerClass::Shaman), 7u);
  EXPECT_EQ(ToClassId(PlayerClass::Mage), 8u);
  EXPECT_EQ(ToClassId(PlayerClass::Warlock), 9u);
  EXPECT_EQ(ToClassId(PlayerClass::Druid), 11u);
}

TEST(PlayerClassTests, ToPlayerClassRoundTrip) {
  EXPECT_EQ(ToPlayerClass(1u), PlayerClass::Warrior);
  EXPECT_EQ(ToPlayerClass(11u), PlayerClass::Druid);
}

TEST(PlayerClassTests, IsValidPlayerClass) {
  EXPECT_TRUE(IsValidPlayerClass(PlayerClass::Warlock));
  EXPECT_FALSE(IsValidPlayerClass(PlayerClass::None));
  EXPECT_FALSE(IsValidPlayerClass(PlayerClass::Unused));
  EXPECT_FALSE(IsValidPlayerClass(99u));
}

TEST(PlayerClassTests, PlayerClassMaskAcceptsEnum) {
  EXPECT_EQ(PlayerClassMask(PlayerClass::Druid), 1024u);
  EXPECT_EQ(PlayerClassMask(PlayerClass::Warrior), 1u);
}

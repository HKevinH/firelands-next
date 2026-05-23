#include <gtest/gtest.h>
#include <domain/models/Character.h>

using namespace Firelands;

TEST(CharacterDisplayIdTests, GoblinUsesChrRacesDisplayIds) {
  // 4.3.4 ChrRaces.dbc race 9 — 21245/21246 are ogre models
  // and must not be used for player goblins.
  EXPECT_EQ(Character::GetDefaultDisplayId(9, 0), 6894u);
  EXPECT_EQ(Character::GetDefaultDisplayId(9, 1), 6895u);
}

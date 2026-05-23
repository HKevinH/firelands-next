#include <application/logic/GossipLogic.h>
#include <shared/game/PlayerClass.h>
#include <shared/game/QuestMask.h>
#include <gtest/gtest.h>
#include <vector>

namespace Firelands {

TEST(QuestMaskTests, PlayerClassMask_Matches) {
  EXPECT_EQ(PlayerClassMask(PlayerClass::Druid), 1024u);
  EXPECT_EQ(PlayerClassMask(PlayerClass::Warrior), 1u);
}

TEST(QuestMaskTests, PlayerRaceMask_Troll) {
  EXPECT_EQ(PlayerRaceMask(8), 128u); // Troll
}

TEST(QuestGossipFilterTests, Jinthala_ShowsOnlyDruidQuestForTrollDruid) {
  std::vector<QuestGossipSummary> all;
  auto add = [&](uint32_t id, uint32_t classes, uint32_t races) {
    QuestGossipSummary s;
    s.questId = id;
    s.title = "The Rise of the Darkspear";
    s.allowableClasses = classes;
    s.allowableRaces = races;
    all.push_back(std::move(s));
  };
  add(24607, 1, 946);
  add(24750, 128, 946);
  add(24758, 64, 946);
  add(24764, 1024, 946);
  add(24770, 8, 946);
  add(24776, 4, 946);
  add(24782, 16, 946);
  add(26272, 256, 946);

  auto filtered = FilterQuestGossipForPlayer(
      all, ToClassId(PlayerClass::Druid), 8);
  ASSERT_EQ(filtered.size(), 1u);
  EXPECT_EQ(filtered[0].questId, 24764u);
}

TEST(QuestGossipFilterTests, ZeroMasksAllowAllClassesAndRaces) {
  QuestGossipSummary open;
  open.questId = 1;
  EXPECT_TRUE(QuestGossipAllowsPlayer(open, ToClassId(PlayerClass::Druid), 8));
  EXPECT_TRUE(QuestGossipAllowsPlayer(open, ToClassId(PlayerClass::Warrior), 1));
}

} // namespace Firelands

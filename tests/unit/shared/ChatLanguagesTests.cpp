#include <gtest/gtest.h>
#include <domain/models/Chat.h>
#include <shared/game/ChatLanguages.h>

#include <algorithm>
#include <unordered_set>
#include <vector>

using namespace Firelands;

TEST(ChatLanguagesTests, DraeneiGetsCommonDefaultAndRacialLanguage) {
  EXPECT_EQ(DefaultLanguageForRace(11), 7u); // Common (Alliance trade language)

  EXPECT_EQ(LanguageSpellIdForLang(35), 29932u);
  EXPECT_EQ(LanguageSkillIdForLang(35), 759u);
  EXPECT_EQ(LanguageSpellIdForLang(7), 668u);
  EXPECT_EQ(LanguageSkillIdForLang(7), 98u);

  std::vector<uint32> spells;
  AppendRacialLanguageSpells(11, spells);
  auto hasSpell = [&](uint32 id) {
    return std::find(spells.begin(), spells.end(), id) != spells.end();
  };
  EXPECT_TRUE(hasSpell(29932u)) << "Language: Draenei passive";
  EXPECT_TRUE(hasSpell(668u)) << "Language: Common passive";

  std::vector<uint32> skills;
  AppendRacialLanguageSkills(11, skills);
  auto hasSkill = [&](uint32 id) {
    return std::find(skills.begin(), skills.end(), id) != skills.end();
  };
  EXPECT_TRUE(hasSkill(759u)) << "Draenei language skill line";
  EXPECT_TRUE(hasSkill(98u)) << "Common language skill line";
}

TEST(ChatLanguagesTests, PlayerKnowsDraeneiWhenPassiveSpellPresent) {
  std::unordered_set<uint32> known{29932u, 668u};
  EXPECT_TRUE(PlayerKnowsLanguage(known, 35));
  EXPECT_TRUE(PlayerKnowsLanguage(known, 7));
  EXPECT_FALSE(PlayerKnowsLanguage(known, 1)); // Orcish
}

TEST(ChatLanguagesTests, PrioritizeDefaultLanguageSpellMovesPrimaryFirstForDraenei) {
  std::vector<uint32> spells{29932u, 668u};
  PrioritizeDefaultLanguageSpell(11, spells);
  ASSERT_GE(spells.size(), 2u);
  EXPECT_EQ(spells.front(), 29932u);
}

TEST(ChatLanguagesTests, AllianceHumanGetsCommonLanguage) {
  EXPECT_EQ(DefaultLanguageForRace(1), LANG_COMMON);

  std::vector<uint32> spells;
  AppendRacialLanguageSpells(1, spells);
  ASSERT_EQ(spells.size(), 1u);
  EXPECT_EQ(spells[0], 668u);

  std::vector<uint32> skills;
  AppendRacialLanguageSkills(1, skills);
  ASSERT_EQ(skills.size(), 1u);
  EXPECT_EQ(skills[0], 98u);
}

TEST(ChatLanguagesTests, NormalizePlayerChatLanguageMapsUniversalAndAddonToDefault) {
  std::unordered_set<uint32> known{668u};

  EXPECT_EQ(NormalizePlayerChatLanguage(LANG_UNIVERSAL, CHAT_MSG_SAY, 1, known),
            LANG_COMMON);
  EXPECT_EQ(NormalizePlayerChatLanguage(CHAT_LANG_ADDON, CHAT_MSG_SAY, 1, known),
            LANG_COMMON);
  EXPECT_EQ(NormalizePlayerChatLanguage(LANG_COMMON, CHAT_MSG_SAY, 1, known),
            LANG_COMMON);
  EXPECT_EQ(NormalizePlayerChatLanguage(LANG_ORCISH, CHAT_MSG_SAY, 1, known),
            LANG_COMMON);
}

TEST(ChatLanguagesTests, NormalizePlayerChatLanguageKeepsKnownRacialLanguage) {
  std::unordered_set<uint32> draenei{668u, 29932u};
  EXPECT_EQ(NormalizePlayerChatLanguage(LANG_DRAENEI, CHAT_MSG_SAY, 11, draenei),
            LANG_DRAENEI);
}

TEST(ChatLanguagesTests, GoblinKnowsGoblinAndOrcishByRaceGrant) {
  std::unordered_set<uint32> empty;
  EXPECT_TRUE(PlayerKnowsLanguage(empty, LANG_GOBLIN, 9));
  EXPECT_TRUE(PlayerKnowsLanguage(empty, LANG_GOBLIN_BINARY, 9));
  EXPECT_TRUE(PlayerKnowsLanguage(empty, LANG_ORCISH, 9));
  EXPECT_EQ(LanguageSpellIdForLang(LANG_GOBLIN_BINARY), 69269u);
  EXPECT_EQ(LanguageSkillIdForLang(LANG_GOBLIN_BINARY), 792u);
}

TEST(ChatLanguagesTests, GoblinNormalizeKeepsGoblinLanguageWithoutSpellbook) {
  std::unordered_set<uint32> empty;
  EXPECT_EQ(NormalizePlayerChatLanguage(LANG_GOBLIN, CHAT_MSG_SAY, 9, empty),
            LANG_GOBLIN);
  EXPECT_EQ(NormalizePlayerChatLanguage(LANG_UNIVERSAL, CHAT_MSG_SAY, 9, empty),
            LANG_GOBLIN);
}

TEST(ChatLanguagesTests, UndeadPrimaryLanguageIsGutterspeak) {
  EXPECT_EQ(PrimaryLanguageForRace(5), LANG_GUTTERSPEAK);
  EXPECT_EQ(DefaultLanguageForRace(5), LANG_ORCISH);

  std::unordered_set<uint32> empty;
  EXPECT_EQ(NormalizePlayerChatLanguage(LANG_UNIVERSAL, CHAT_MSG_SAY, 5, empty),
            LANG_GUTTERSPEAK);
  EXPECT_EQ(NormalizePlayerChatLanguage(LANG_GUTTERSPEAK, CHAT_MSG_SAY, 5, empty),
            LANG_GUTTERSPEAK);
}

TEST(ChatLanguagesTests, GoblinGetsGoblinAndOrcishLanguagePassives) {
  std::vector<uint32> spells;
  AppendRacialLanguageSpells(9, spells);
  auto has = [&](uint32 id) {
    return std::find(spells.begin(), spells.end(), id) != spells.end();
  };
  EXPECT_TRUE(has(69269u));
  EXPECT_TRUE(has(669u));

  std::vector<uint32> skills;
  AppendRacialLanguageSkills(9, skills);
  auto hasSkill = [&](uint32 id) {
    return std::find(skills.begin(), skills.end(), id) != skills.end();
  };
  EXPECT_TRUE(hasSkill(792u));
  EXPECT_TRUE(hasSkill(109u));
}

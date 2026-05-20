#include <gtest/gtest.h>
#include <shared/dbc/StarterSpellsDbc.h>

#include <algorithm>
#include <string>

using namespace Firelands;

namespace {

std::string const kDbcDir =
    std::string(FIRELANDS_TEST_DATA_DIR) + "/data/dbc";

} // namespace

TEST(StarterSpellsDbcTests, LoadMissingFile_ReturnsFalse) {
  StarterSpellsDbc dbc;
  EXPECT_FALSE(dbc.Load("/nonexistent/SkillLineAbility.dbc",
                        "/nonexistent/SkillRaceClassInfo.dbc"));
  EXPECT_FALSE(dbc.IsLoaded());
}

TEST(StarterSpellsDbcTests, LoadBundledDbc_TrollDruidHasCoreSpells) {
  StarterSpellsDbc dbc;
  ASSERT_TRUE(dbc.Load(kDbcDir + "/SkillLineAbility.dbc",
                        kDbcDir + "/SkillRaceClassInfo.dbc"));
  ASSERT_TRUE(dbc.IsLoaded());

  std::vector<uint32_t> spells = dbc.GetStarterSpells(8, 11);
  ASSERT_FALSE(spells.empty());

  auto has = [&](uint32_t id) {
    return std::find(spells.begin(), spells.end(), id) != spells.end();
  };
  EXPECT_TRUE(has(5176u)) << "Wrath";
  EXPECT_TRUE(has(5185u)) << "Healing Touch";
  EXPECT_TRUE(has(8921u)) << "Moonfire";
  EXPECT_TRUE(has(768u)) << "Cat Form";
  EXPECT_FALSE(has(33388u)) << "riding spell";
}

TEST(StarterSpellsDbcTests, LoadBundledDbc_HumanWarriorMatchesReferenceSubset) {
  StarterSpellsDbc dbc;
  ASSERT_TRUE(dbc.Load(kDbcDir + "/SkillLineAbility.dbc",
                        kDbcDir + "/SkillRaceClassInfo.dbc"));

  std::vector<uint32_t> spells = dbc.GetStarterSpells(1, 1);
  ASSERT_FALSE(spells.empty());

  auto has = [&](uint32_t id) {
    return std::find(spells.begin(), spells.end(), id) != spells.end();
  };
  EXPECT_TRUE(has(78u));
  EXPECT_TRUE(has(2457u));
  EXPECT_TRUE(has(6673u));
}

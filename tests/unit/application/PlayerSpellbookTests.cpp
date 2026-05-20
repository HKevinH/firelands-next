#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <application/services/PlayerSpellbook.h>
#include <application/services/PlayerCreateInfoService.h>
#include <domain/models/SpellDefinition.h>
#include <domain/repositories/IPlayerCreateInfoRepository.h>
#include <domain/repositories/ISpellDefinitionStore.h>

using namespace Firelands;
using namespace testing;

class MockSpellStore : public ISpellDefinitionStore {
public:
  MOCK_METHOD(bool, HasSpell, (uint32), (const, override));
  MOCK_METHOD(std::optional<SpellDefinition>, GetDefinition, (uint32),
              (const, override));
};

class MockPciRepo : public IPlayerCreateInfoRepository {
public:
  MOCK_METHOD(std::optional<PlayerCreateInfo>, GetStartPosition, (uint8, uint8),
              (override));
  MOCK_METHOD(std::vector<PlayerCreateVisualItem>, GetVisualItems,
              (uint8, uint8, uint8, uint8), (override));
  MOCK_METHOD(std::vector<StarterItemGrant>, GetExtraCreateItems, (uint8, uint8),
              (override));
  MOCK_METHOD(std::vector<uint32_t>, GetStarterSpells, (uint8_t, uint8_t),
              (override));
  MOCK_METHOD(std::vector<StarterSkillGrant>, GetStarterSkills, (uint8_t, uint8_t),
              (override));
  MOCK_METHOD(std::optional<PlayerClassLevelStats>, GetClassLevelStats,
              (uint8_t, uint8_t), (override));
  MOCK_METHOD(std::optional<PlayerRaceStats>, GetRaceStats, (uint8_t),
              (override));
  MOCK_METHOD(uint32_t, GetXpForNextLevel, (uint8_t), (const, override));
};

TEST(PlayerSpellbook, FiltersExtraCharacterSpellsByRequiredLevel) {
  auto repo = std::make_shared<MockPciRepo>();
  EXPECT_CALL(*repo, GetStarterSpells(_, _))
      .WillRepeatedly(Return(std::vector<uint32_t>{}));
  PlayerCreateInfoService svc(repo, "", "");

  MockSpellStore spells;
  SpellDefinition low;
  low.id = 118;
  low.requiredLevel = 1;
  SpellDefinition high;
  high.id = 116;
  high.requiredLevel = 10;

  EXPECT_CALL(spells, GetDefinition(118)).WillRepeatedly(Return(low));
  EXPECT_CALL(spells, GetDefinition(116)).WillRepeatedly(Return(high));
  EXPECT_CALL(spells, HasSpell(_)).WillRepeatedly(Return(true));

  auto known =
      PlayerSpellbook::BuildKnownSpells(1, 8, 5, svc, &spells, {118u, 116u});
  EXPECT_THAT(known, Contains(118u));
  EXPECT_THAT(known, Not(Contains(116u)));
}

TEST(PlayerSpellbook, StarterSpellsFilteredByCharacterLevel) {
  auto repo = std::make_shared<MockPciRepo>();
  EXPECT_CALL(*repo, GetStarterSpells(8, 11))
      .WillRepeatedly(Return(std::vector<uint32_t>{5185u, 5176u, 8921u}));
  PlayerCreateInfoService svc(repo, "", "");

  MockSpellStore spells;
  SpellDefinition healingTouch;
  healingTouch.id = 5185;
  healingTouch.requiredLevel = 78;
  SpellDefinition wrath;
  wrath.id = 5176;
  wrath.requiredLevel = 1;
  SpellDefinition moonfire;
  moonfire.id = 8921;
  moonfire.requiredLevel = 4;

  EXPECT_CALL(spells, GetDefinition(5185))
      .WillRepeatedly(Return(healingTouch));
  EXPECT_CALL(spells, GetDefinition(5176)).WillRepeatedly(Return(wrath));
  EXPECT_CALL(spells, GetDefinition(8921)).WillRepeatedly(Return(moonfire));
  EXPECT_CALL(spells, HasSpell(_)).WillRepeatedly(Return(true));

  auto known =
      PlayerSpellbook::BuildKnownSpells(8, 11, 1, svc, &spells, {});
  EXPECT_THAT(known, Contains(5176u)) << "Wrath at level 1";
  EXPECT_THAT(known, Not(Contains(8921u))) << "Moonfire requires level 4";
  EXPECT_THAT(known, Not(Contains(5185u))) << "Healing Touch requires level 78";

  auto known5 =
      PlayerSpellbook::BuildKnownSpells(8, 11, 5, svc, &spells, {});
  EXPECT_THAT(known5, Contains(5176u));
  EXPECT_THAT(known5, Contains(8921u)) << "Moonfire unlocked at level 5";
  EXPECT_THAT(known5, Not(Contains(5185u))) << "Healing Touch still locked";
}

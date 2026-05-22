#include <gtest/gtest.h>
#include <application/spell/SpellImpactEffects.h>
#include <domain/models/SpellDefinition.h>
#include <shared/dbc/SpellVisualDbc.h>
#include <shared/Logger.h>

#include <string>

using namespace Firelands;

namespace {

std::string const kSpellVisualDbcPath =
    std::string(FIRELANDS_TEST_DATA_DIR) + "/data/dbc/SpellVisual.dbc";

class SpellImpactEffectsTests : public ::testing::Test {
protected:
  void SetUp() override {
    if (!Logger::IsInitialized())
      Logger::Init(LoggerBuilder().WithConsole(false).Build());
    ASSERT_TRUE(m_dbc.Load(kSpellVisualDbcPath));
  }

  SpellVisualDbc m_dbc;
};

} // namespace

TEST_F(SpellImpactEffectsTests, PrefersSecondarySpellVisualForImpactKit) {
  SpellDefinition def{};
  def.id = 133;
  def.spellVisualId0 = 185;
  def.spellVisualId1 = 5044;

  EXPECT_EQ(SpellImpactEffects::ResolveImpactKitForSpell(def, m_dbc), 348u);
}

TEST_F(SpellImpactEffectsTests, FallsBackToPrimaryVisualWhenSecondaryHasNoKit) {
  SpellDefinition def{};
  def.spellVisualId0 = 185;
  def.spellVisualId1 = 999999;

  EXPECT_EQ(SpellImpactEffects::ResolveImpactKitForSpell(def, m_dbc), 313u);
}

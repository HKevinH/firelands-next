#include <gtest/gtest.h>
#include <shared/dbc/SpellVisualDbc.h>
#include <shared/Logger.h>

#include <string>

using namespace Firelands;

namespace {

std::string const kSpellVisualDbcPath =
    std::string(FIRELANDS_TEST_DATA_DIR) + "/data/dbc/SpellVisual.dbc";

class SpellVisualDbcTests : public ::testing::Test {
protected:
  void SetUp() override {
    if (!Logger::IsInitialized())
      Logger::Init(LoggerBuilder().WithConsole(false).Build());
  }
};

} // namespace

TEST_F(SpellVisualDbcTests, ResolvesFireballImpactKitFromLocalDbc) {
  SpellVisualDbc dbc;
  ASSERT_TRUE(dbc.Load(kSpellVisualDbcPath));

  EXPECT_EQ(dbc.ResolveImpactKitId(185u), 313u);
  EXPECT_EQ(dbc.ResolveImpactKitId(5044u), 348u);
  EXPECT_EQ(dbc.ResolveImpactKitId(999999u), 0u);
}

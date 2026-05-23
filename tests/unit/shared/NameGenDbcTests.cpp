#include <gtest/gtest.h>
#include <shared/dbc/NameGenDbc.h>
#include <shared/Logger.h>

#include <set>
#include <string>

using namespace Firelands;

namespace {

std::string const kNameGenDbcPath =
    std::string(FIRELANDS_TEST_DATA_DIR) + "/data/dbc/NameGen.dbc";

class NameGenDbcTests : public ::testing::Test {
protected:
  void SetUp() override {
    if (!Logger::IsInitialized())
      Logger::Init(LoggerBuilder().WithConsole(false).Build());
  }
};

} // namespace

TEST_F(NameGenDbcTests, LoadMissingFile_ReturnsFalse) {
  NameGenDbc dbc;
  EXPECT_FALSE(dbc.Load("/nonexistent/NameGen.dbc"));
  EXPECT_FALSE(dbc.IsLoaded());
}

TEST_F(NameGenDbcTests, PicksGenderSpecificNamesFromLocalDbc) {
  NameGenDbc dbc;
  ASSERT_TRUE(dbc.Load(kNameGenDbcPath));

  std::set<std::string> maleNames;
  std::set<std::string> femaleNames;
  for (int i = 0; i < 40; ++i) {
    auto male = dbc.PickRandomName(1, 0);
    auto female = dbc.PickRandomName(1, 1);
    ASSERT_TRUE(male.has_value());
    ASSERT_TRUE(female.has_value());
    maleNames.insert(*male);
    femaleNames.insert(*female);
  }

  EXPECT_FALSE(maleNames.empty());
  EXPECT_FALSE(femaleNames.empty());
  EXPECT_TRUE(maleNames.count("Agamand") > 0 || maleNames.count("Andromath") > 0);
  EXPECT_TRUE(femaleNames.count("Aegwynn") > 0 || femaleNames.count("Taretha") > 0);
  EXPECT_EQ(maleNames.count("Aegwynn"), 0u);
  EXPECT_EQ(femaleNames.count("Agamand"), 0u);
}

TEST_F(NameGenDbcTests, InvalidRaceOrGender_ReturnsEmpty) {
  NameGenDbc dbc;
  ASSERT_TRUE(dbc.Load(kNameGenDbcPath));
  EXPECT_FALSE(dbc.PickRandomName(0, 0).has_value());
  EXPECT_FALSE(dbc.PickRandomName(99, 0).has_value());
  EXPECT_FALSE(dbc.PickRandomName(1, 2).has_value());
}

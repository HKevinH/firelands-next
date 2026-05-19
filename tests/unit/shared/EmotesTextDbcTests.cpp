#include <gtest/gtest.h>
#include <shared/dbc/EmotesTextDbc.h>

#include <string>

using namespace Firelands;

TEST(EmotesTextDbcTests, LoadMissingFile_ReturnsFalse) {
  EmotesTextDbc dbc;
  EXPECT_FALSE(dbc.Load("/nonexistent/dir/EmotesText.dbc"));
  EXPECT_FALSE(dbc.IsLoaded());
  EXPECT_FALSE(dbc.LookupEmoteAnim(111).has_value());
}

TEST(EmotesTextDbcTests, LoadBundledDbc_ResolvesKnownRow) {
  EmotesTextDbc dbc;
  ASSERT_TRUE(
      dbc.Load(std::string(FIRELANDS_TEST_DATA_DIR) + "/data/dbc/EmotesText.dbc"));
  ASSERT_TRUE(dbc.IsLoaded());
  auto anim = dbc.LookupEmoteAnim(111);
  ASSERT_TRUE(anim.has_value());
  EXPECT_EQ(*anim, 112u);
}

#include <infrastructure/persistence/SqlStatementSplitter.h>

#include <gtest/gtest.h>

namespace Firelands {
namespace {

TEST(SqlStatementSplitterTests, SingleStatementWithEscapedApostrophe) {
  std::string const sql =
      "REPLACE INTO t VALUES (12459,0,0,N'I''m here to escort.',49050);";
  auto const parts = SplitSqlStatements(sql);
  ASSERT_EQ(parts.size(), 1U);
  EXPECT_NE(parts[0].find("I''m here"), std::string::npos);
}

TEST(SqlStatementSplitterTests, SemicolonInsideGenderTokenDoesNotSplit) {
  std::string const sql =
      "REPLACE INTO t VALUES (153,N'Heya, $g champ : lady;. What''s shakin''?');";
  auto const parts = SplitSqlStatements(sql);
  ASSERT_EQ(parts.size(), 1U);
  EXPECT_NE(parts[0].find("$g champ : lady;"), std::string::npos);
}

TEST(SqlStatementSplitterTests, DoubleQuotesInsideSingleQuotedLiteral) {
  std::string const sql =
      "REPLACE INTO t VALUES "
      "(12549,0,0,N'Hit the button labeled \"Just past the Grim Guzzler\".',49848);";
  auto const parts = SplitSqlStatements(sql);
  ASSERT_EQ(parts.size(), 1U);
  EXPECT_NE(parts[0].find("\"Just past the Grim Guzzler\""), std::string::npos);
}

TEST(SqlStatementSplitterTests, NestedQuoteLabelsInGossipText) {
  std::string const sql =
      "REPLACE INTO t VALUES "
      "(12691,1,0,N'Press the button labeled ''Metal and Scraps.''',50717);";
  auto const parts = SplitSqlStatements(sql);
  ASSERT_EQ(parts.size(), 1U);
  EXPECT_NE(parts[0].find("''Metal and Scraps.''"), std::string::npos);
  EXPECT_NE(parts[0].find(",50717"), std::string::npos);
}

TEST(SqlStatementSplitterTests, MultipleStatementsSplitOnSemicolonOutsideStrings) {
  std::string const sql = "USE `firelands_world`;\nDELETE FROM `creature`;\n";
  auto const parts = SplitSqlStatements(sql);
  ASSERT_EQ(parts.size(), 2U);
}

} // namespace
} // namespace Firelands

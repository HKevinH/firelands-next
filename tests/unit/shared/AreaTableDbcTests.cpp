#include <shared/dbc/AreaTableDbc.h>
#include <gtest/gtest.h>

namespace Firelands {
namespace {

TEST(AreaTableDbcTests, UnloadedResolveReturnsClientHint) {
  AreaTableDbc const table;
  EXPECT_EQ(table.ResolveAreaForPhasing(654, 4756u), 4756u);
  EXPECT_EQ(table.ResolveAreaForPhasing(0, 0u), 0u);
}

} // namespace
} // namespace Firelands

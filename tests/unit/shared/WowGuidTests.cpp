#include <shared/game/WowGuid.h>
#include <gtest/gtest.h>

namespace Firelands {

TEST(WowGuidTests, CreatureObjectGuid_RoundTripsEntry) {
  uint64_t const guid = MakeCreatureObjectGuid(68, 0x0000ABCDu);
  EXPECT_EQ(ExtractCreatureEntryFromUnitObjectGuid(guid), 68u);
  EXPECT_EQ(guid & 0xFFFFFFFFu, 0x0000ABCDu);

  uint64_t const guidLarge = MakeCreatureObjectGuid(50000, 1u);
  EXPECT_EQ(ExtractCreatureEntryFromUnitObjectGuid(guidLarge), 50000u);
}

TEST(WowGuidTests, ExtractCreatureEntryFromUnitObjectGuid_ZeroGuid) {
  EXPECT_EQ(ExtractCreatureEntryFromUnitObjectGuid(0), 0u);
}

} // namespace Firelands

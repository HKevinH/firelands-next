#include <application/logic/GossipLogic.h>
#include <gtest/gtest.h>

namespace Firelands {

TEST(GossipLogicTests, ResolveGossipMenuId_UsesTemplateWhenSet) {
  EXPECT_EQ(ResolveGossipMenuIdForTemplate(435), 435u);
  EXPECT_EQ(ResolveGossipMenuIdForTemplate(0), kDefaultNpcGossipMenuId);
}

TEST(GossipLogicTests, FilterGossipOptionsByNpcFlags_MatchesRoleFlags) {
  GossipMenuItem vendor;
  vendor.optionIndex = 0;
  vendor.optionNpcflag = 128;

  GossipMenuItem gossip;
  gossip.optionIndex = 1;
  gossip.optionNpcflag = 1;

  GossipMenuItem ungated;
  ungated.optionIndex = 2;
  ungated.optionNpcflag = 0;

  std::vector<GossipMenuItem> items = {vendor, gossip, ungated};
  auto filtered = FilterGossipOptionsByNpcFlags(std::move(items), 128);

  ASSERT_EQ(filtered.size(), 2u);
  EXPECT_EQ(filtered[0].optionIndex, 0u);
  EXPECT_EQ(filtered[1].optionIndex, 2u);
}

TEST(GossipLogicTests, FilterGossipOptionsByNpcFlags_ZeroFlagsKeepsAll) {
  GossipMenuItem a;
  a.optionNpcflag = 128;
  std::vector<GossipMenuItem> items = {a};
  auto filtered = FilterGossipOptionsByNpcFlags(std::move(items), 0);
  EXPECT_EQ(filtered.size(), 1u);
}

TEST(GossipLogicTests, FindGossipMenuItem_ByOptionIndex) {
  GossipMenuItem a;
  a.optionIndex = 3;
  std::vector<GossipMenuItem> items = {a};
  EXPECT_NE(FindGossipMenuItem(items, 3), nullptr);
  EXPECT_EQ(FindGossipMenuItem(items, 9), nullptr);
}

} // namespace Firelands

#include <application/world/PhaseAreaCatalog.h>
#include <gtest/gtest.h>

namespace Firelands {
namespace {

TEST(PhaseAreaCatalogTests, DirectAreaLookup) {
  PhaseAreaCatalog catalog;
  catalog.Load({{4756u, {169u}}});
  auto const phases = catalog.ResolveForArea(4756);
  ASSERT_EQ(phases.size(), 1u);
  EXPECT_EQ(phases[0], 169u);
}

TEST(PhaseAreaCatalogTests, ParentAreaInheritsPhase) {
  PhaseAreaCatalog catalog;
  catalog.Load({{4714u, {105u}}});

  auto const parentOf = [](uint32 area) -> uint32 {
    if (area == 4756u)
      return 4714u;
    return 0u;
  };

  auto const phases = catalog.ResolveForArea(4756, parentOf);
  ASSERT_EQ(phases.size(), 1u);
  EXPECT_EQ(phases[0], 105u);
}

} // namespace
} // namespace Firelands

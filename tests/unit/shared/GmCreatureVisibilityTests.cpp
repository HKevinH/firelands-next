#include <gtest/gtest.h>

#include <shared/game/CreatureExtraFlags.h>
#include <shared/game/GmCreatureVisibility.h>
#include <shared/game/PhaseShift.h>

namespace Firelands {
namespace {

TEST(GmCreatureVisibilityTests, GmBypassesPhaseMismatch) {
  PhaseShift viewer;
  viewer.flags = static_cast<uint32>(PhaseShiftFlags::Unphased);

  PhaseShift creature;
  InitDbCreaturePhaseShift(creature, kPhaseUseFlagsNone, 170, 0, nullptr);

  EXPECT_FALSE(CreatureVisibleToViewer(viewer, creature, false));
  EXPECT_TRUE(CreatureVisibleToViewer(viewer, creature, true));
}

TEST(GmCreatureVisibilityTests, GmClearsNotSelectableUnitFlag) {
  uint32_t const flags = 0x01000000u | kUnitFieldFlagNotSelectable;
  EXPECT_EQ(WireUnitFieldFlagsForCreature(flags, false), flags);
  EXPECT_EQ(WireUnitFieldFlagsForCreature(flags, true), 0x01000000u);
}

TEST(GmCreatureVisibilityTests, GmUsesTemplateNpcFlags) {
  constexpr uint32_t kTemplate = 0x80u;
  constexpr uint32_t kQuestAdjusted = 0x02u;
  EXPECT_EQ(WireNpcFlagsForCreature(kTemplate, kQuestAdjusted, false), kQuestAdjusted);
  EXPECT_EQ(WireNpcFlagsForCreature(kTemplate, kQuestAdjusted, true), kTemplate);
}

TEST(GmCreatureVisibilityTests, TriggerUsesInvisibleDisplayForNormalPlayers) {
  constexpr uint32_t kEchoIslesQuestBunnyDisplay = 20570u;
  EXPECT_EQ(WireDisplayIdForCreature(kEchoIslesQuestBunnyDisplay, kCreatureExtraFlagTrigger,
                                     false),
            kInvisibleTriggerDisplayId);
  EXPECT_EQ(WireDisplayIdForCreature(kEchoIslesQuestBunnyDisplay, kCreatureExtraFlagTrigger,
                                     true),
            kEchoIslesQuestBunnyDisplay);
  EXPECT_EQ(WireDisplayIdForCreature(kEchoIslesQuestBunnyDisplay, 0u, false),
            kEchoIslesQuestBunnyDisplay);
}

} // namespace
} // namespace Firelands

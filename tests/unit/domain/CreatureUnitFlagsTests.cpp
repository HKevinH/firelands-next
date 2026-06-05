#include <domain/world/Creature.h>
#include <gtest/gtest.h>
#include <shared/game/CreatureExtraFlags.h>
#include <shared/game/UnitFieldFlags.h>
#include <shared/game/UnitNpcFlags.h>

namespace Firelands {
namespace {

TEST(CreatureUnitFlagsTests, PersistsUnitFieldAndExtraFlagsFromTemplate) {
  Creature bunny(1u, 38003u, 20570u, 1u, 1u, Creature::kDefaultFactionTemplate, 0u,
                 kUnitFieldFlagNotSelectable, 2048u,
                 kCreatureExtraFlagTrigger | 0x2000u);

  EXPECT_EQ(bunny.GetNpcFlags(), 0u);
  EXPECT_EQ(bunny.GetUnitFieldFlags(), kUnitFieldFlagNotSelectable);
  EXPECT_EQ(bunny.GetUnitFieldFlags2(), 2048u);
  EXPECT_EQ(bunny.GetExtraFlags(), kCreatureExtraFlagTrigger | 0x2000u);
}

TEST(CreatureUnitFlagsTests, StunFlagSetAndCleared) {
  Creature mob(1u, 1u, 1u, 100u, 1u);
  EXPECT_FALSE(mob.IsStunned());

  mob.MarkStunned();
  EXPECT_TRUE(mob.IsStunned());
  EXPECT_NE(mob.GetUnitFieldFlags() & kUnitFlagStunned, 0u);

  mob.ClearStunned();
  EXPECT_FALSE(mob.IsStunned());
  EXPECT_EQ(mob.GetUnitFieldFlags() & kUnitFlagStunned, 0u);
}

TEST(CreatureUnitFlagsTests, ActsAsScriptTriggerWhenTemplateMatchesProxyPattern) {
  Creature bunny(1u, 38003u, 20570u, 1u, 1u, Creature::kDefaultFactionTemplate, 0u,
                 kUnitFieldFlagNotSelectable, 0u, kCreatureExtraFlagTrigger);
  EXPECT_TRUE(bunny.ActsAsScriptTrigger());

  Creature vendor(2u, 1u, 1u, 1u, 1u, Creature::kDefaultFactionTemplate,
                  kUnitNpcFlagGossip);
  EXPECT_FALSE(vendor.ActsAsScriptTrigger());
}

} // namespace
} // namespace Firelands

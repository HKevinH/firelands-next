#include <gtest/gtest.h>
#include <application/spell/SpellHitEffects.h>
#include <application/spell/SpellManager.h>

using namespace Firelands;

TEST(SpellHitEffectsTests, ResolvePrimarySpellHitSelfWhenNoUnitTargetBit) {
  uint64 const g = 0x100ULL;
  EXPECT_EQ(SpellHitEffects::ResolvePrimarySpellHitUnitGuid(0, g, 0x200ULL), g);
}

TEST(SpellHitEffectsTests, ResolvePrimarySpellHitUsesUnitWhenMaskedAndGuidNonZero) {
  uint64 const caster = 0x100ULL;
  uint64 const target = 0x200ULL;
  uint32 const flags = SpellCastWire::TARGET_FLAG_UNIT;
  EXPECT_EQ(SpellHitEffects::ResolvePrimarySpellHitUnitGuid(flags, caster, target),
            target);
}

TEST(SpellHitEffectsTests,
     ResolvePrimarySpellHitFallsBackToCasterWhenMaskedButUnitGuidZero) {
  uint64 const caster = 0x100ULL;
  uint32 const flags = SpellCastWire::TARGET_FLAG_UNIT;
  EXPECT_EQ(SpellHitEffects::ResolvePrimarySpellHitUnitGuid(flags, caster, 0u), caster);
}

TEST(SpellHitEffectsTests, ApplyImmediateHealthDoesNothingWhenDefNull) {
  SpellCastOutcome out{};
  out.hasDirectHealthEffect = false;
  SpellHitEffects::ApplyImmediateHealthFromDefinition(nullptr, 0x42ULL, &out);
  EXPECT_FALSE(out.hasDirectHealthEffect);
  EXPECT_EQ(out.directHealthTargetGuid, 0u);
  EXPECT_EQ(out.directHealthDelta, 0);
}

TEST(SpellHitEffectsTests, ApplyImmediateHealthDoesNothingWhenDeltaZero) {
  SpellDefinition def{};
  def.immediateHealthEffectDelta = 0;
  SpellCastOutcome out{};
  SpellHitEffects::ApplyImmediateHealthFromDefinition(&def, 0x99ULL, &out);
  EXPECT_FALSE(out.hasDirectHealthEffect);
}

TEST(SpellHitEffectsTests, ApplyImmediateHealthWritesOutcomeWhenDeltaNonZero) {
  SpellDefinition def{};
  def.immediateHealthEffectDelta = -42;
  SpellCastOutcome out{};
  uint64 const hit = 0xABCDULL;
  SpellHitEffects::ApplyImmediateHealthFromDefinition(&def, hit, &out);
  EXPECT_TRUE(out.hasDirectHealthEffect);
  EXPECT_EQ(out.directHealthTargetGuid, hit);
  EXPECT_EQ(out.directHealthDelta, -42);
}

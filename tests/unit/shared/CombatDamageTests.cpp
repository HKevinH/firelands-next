#include <gtest/gtest.h>
#include <shared/game/CombatDamage.h>
#include <shared/game/SpellSchool.h>
#include <shared/game/UnitCombatStats.h>

using namespace Firelands;

TEST(CombatDamageTests, ArmorCapsAtSeventyFivePercent) {
  uint32 const raw = 1000u;
  uint32 const mitigated = CalcArmorReducedDamage(85, 50000u, raw);
  EXPECT_LE(mitigated, 265u);
  EXPECT_GT(mitigated, 0u);
}

TEST(CombatDamageTests, LowLevelTargetTakesLessMitigationThanHighArmor) {
  uint32 const vsCloth = CalcArmorReducedDamage(10, 200u, 100u);
  uint32 const vsPlate = CalcArmorReducedDamage(10, 3000u, 100u);
  EXPECT_GT(vsCloth, vsPlate);
}

TEST(CombatDamageTests, ResistanceBuffModsReduceMagicDamage) {
  UnitCombatStats caster{};
  caster.level = 10;

  UnitCombatStats victim{};
  victim.level = 10;
  victim.resistance[5] = 0u;
  victim.resistanceBuffPos[5] = 100;

  int32 const mitigated = ResolveMitigatedHealthDelta(
      -100, kSpellSchoolMaskShadow, &caster, caster.level, &victim);
  EXPECT_LT(-mitigated, 100);
}

TEST(CombatDamageTests, MagicResistReducesFireDamage) {
  UnitCombatStats caster{};
  caster.level = 10;

  UnitCombatStats victim{};
  victim.level = 10;
  victim.resistance[2] = 100u;

  int32 const mitigated = ResolveMitigatedHealthDelta(
      -100, kSpellSchoolMaskFire, &caster, caster.level, &victim);
  EXPECT_LT(-mitigated, 100);
}

TEST(CombatDamageTests, SpellPowerIncreasesOutgoingMagicDamage) {
  UnitCombatStats weakCaster{};
  weakCaster.level = 20;

  UnitCombatStats strongCaster = weakCaster;
  strongCaster.spellDamageDonePos[2] = 200;

  UnitCombatStats victim{};
  victim.level = 20;
  victim.resistance[2] = 0;

  int32 const weak = ResolveMitigatedHealthDelta(-50, kSpellSchoolMaskFire, &weakCaster,
                                                 weakCaster.level, &victim);
  int32 const strong =
      ResolveMitigatedHealthDelta(-50, kSpellSchoolMaskFire, &strongCaster,
                                  strongCaster.level, &victim);
  EXPECT_GT(weak, strong);
}

TEST(CombatDamageTests, HealsAreNotMitigated) {
  EXPECT_EQ(ResolveMitigatedHealthDelta(25, kSpellSchoolMaskHoly, nullptr, 1, nullptr),
            25);
}

TEST(CombatDamageTests, MeleeSwingUsesAttackPowerAndArmor) {
  UnitCombatStats attacker{};
  attacker.level = 20;
  attacker.attackPower = 200;

  UnitCombatStats softTarget{};
  softTarget.level = 20;
  softTarget.armor = 100u;

  UnitCombatStats armoredTarget = softTarget;
  armoredTarget.armor = 4000u;

  uint32 const vsSoft = ComputeMeleeSwingDamage(attacker, softTarget);
  uint32 const vsArmored = ComputeMeleeSwingDamage(attacker, armoredTarget);
  EXPECT_GT(vsSoft, vsArmored);
  EXPECT_GT(vsSoft, 10u);
}

TEST(CombatDamageTests, StanceDamageDoneMultiplierScalesMelee) {
  UnitCombatStats attacker{};
  attacker.level = 60;
  attacker.attackPower = 1000;
  UnitCombatStats victim{};
  victim.level = 60;
  victim.armor = 1000u;

  uint32 const neutral = ComputeMeleeSwingDamage(attacker, victim);

  UnitCombatStats berserker = attacker;
  berserker.damageDonePctMultiplier[0] = 1.1f; // +10% physical
  uint32 const boosted = ComputeMeleeSwingDamage(berserker, victim);
  EXPECT_GT(boosted, neutral);

  UnitCombatStats defensive = attacker;
  defensive.damageDonePctMultiplier[0] = 0.9f; // -10% physical
  uint32 const softened = ComputeMeleeSwingDamage(defensive, victim);
  EXPECT_LT(softened, neutral);
}

TEST(CombatDamageTests, StanceDamageTakenMultiplierMitigatesMelee) {
  UnitCombatStats attacker{};
  attacker.level = 60;
  attacker.attackPower = 1000;

  UnitCombatStats victim{};
  victim.level = 60;
  victim.armor = 1000u;
  uint32 const neutral = ComputeMeleeSwingDamage(attacker, victim);

  UnitCombatStats tank = victim;
  tank.damageTakenPctMultiplier[0] = 0.75f; // -25% taken
  uint32 const mitigated = ComputeMeleeSwingDamage(attacker, tank);
  EXPECT_LT(mitigated, neutral);
}

TEST(CombatDamageTests, StanceMultipliersApplyToSpellDamage) {
  UnitCombatStats caster{};
  caster.level = 60;
  caster.damageDonePctMultiplier[1] = 1.5f; // +50% holy (school index 1)
  UnitCombatStats victim{};
  victim.level = 60;

  int32 const boosted = ResolveMitigatedHealthDelta(-100, kSpellSchoolMaskHoly, &caster, 60,
                                                    &victim);
  UnitCombatStats neutralCaster{};
  neutralCaster.level = 60;
  int32 const neutral = ResolveMitigatedHealthDelta(-100, kSpellSchoolMaskHoly,
                                                    &neutralCaster, 60, &victim);
  EXPECT_LT(boosted, neutral); // more damage = more negative delta
}

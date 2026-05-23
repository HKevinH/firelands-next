#include <application/combat/PlayerCombatStats.h>

#include <shared/game/UnitCombatStats.h>

namespace Firelands {

bool UsesAgilityForMeleeAttackPower(PlayerClass klass) {
  switch (klass) {
  case PlayerClass::Hunter:
  case PlayerClass::Rogue:
  case PlayerClass::Druid:
    return true;
  default:
    return false;
  }
}

bool UsesAgilityForMeleeAttackPower(uint8 classId) {
  return UsesAgilityForMeleeAttackPower(ToPlayerClass(classId));
}

int32 ComputeBaseMeleeAttackPower(Character const &character) {
  uint32 const str = character.GetPrimaryStat(0);
  uint32 const agi = character.GetPrimaryStat(1);
  uint32 const level = character.GetLevel();
  int32 ap = static_cast<int32>(level) * 3;
  if (UsesAgilityForMeleeAttackPower(character.GetClass()))
    ap += static_cast<int32>(agi) * 2;
  else
    ap += static_cast<int32>(str) * 2;
  return ap < 0 ? 0 : ap;
}

bool UsesBaselineSpellPowerFromIntellect(PlayerClass klass) {
  switch (klass) {
  case PlayerClass::Paladin:
  case PlayerClass::Hunter:
  case PlayerClass::Priest:
  case PlayerClass::DeathKnight:
  case PlayerClass::Shaman:
  case PlayerClass::Mage:
  case PlayerClass::Warlock:
  case PlayerClass::Druid:
    return true;
  default:
    return false;
  }
}

bool UsesBaselineSpellPowerFromIntellect(uint8 classId) {
  return UsesBaselineSpellPowerFromIntellect(ToPlayerClass(classId));
}

UnitCombatStats BuildPlayerCombatStats(Character const &character) {
  UnitCombatStats stats{};
  stats.level = character.GetLevel();
  PlayerClass const klass = character.GetClass();
  uint32 const agi = character.GetPrimaryStat(1);
  uint32 const str = character.GetPrimaryStat(0);
  uint32 const sta = character.GetPrimaryStat(2);
  uint32 const inte = character.GetPrimaryStat(3);
  uint32 const spi = character.GetPrimaryStat(4);
  uint32 const lv = static_cast<uint32>(stats.level);

  stats.armor = ComputeBaselineArmor(ToClassId(klass), stats.level, agi, str, sta);
  uint32 const mr = lv / 2u + spi / 6u;
  for (uint32 i = 1; i <= 6; ++i)
    stats.resistance[i] = mr;

  stats.attackPower = ComputeBaseMeleeAttackPower(character);

  uint32 spellPower = 0;
  if (UsesBaselineSpellPowerFromIntellect(klass))
    spellPower = inte;
  for (uint32 school = 1; school <= 6; ++school)
    stats.spellDamageDonePos[school] = static_cast<int32>(spellPower);

  return stats;
}

} // namespace Firelands

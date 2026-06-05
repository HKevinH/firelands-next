#include <shared/game/WarriorAbilities.h>

namespace Firelands {

bool IsChargeSpell(uint32 spellId) {
  return spellId == kSpellCharge;
}

bool TryGetWarriorChargeData(uint32 spellId, uint32 &triggeredStunSpellId,
                             int32 &rageGain) {
  if (spellId == kSpellCharge) {
    triggeredStunSpellId = kSpellChargeStun;
    rageGain = kChargeRageGain;
    return true;
  }
  return false;
}

bool IsWarriorStunSpell(uint32 spellId) {
  return spellId == kSpellChargeStun;
}

} // namespace Firelands

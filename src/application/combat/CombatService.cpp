#include "CombatService.h"

namespace application {

void CombatService::StartCombat(::combat::ICombatEntity &attacker,
                                ::combat::ICombatEntity &victim) {
  _engine->Engage(attacker, victim);
}

MeleeSwingResult CombatService::BeginMeleeSwing(::combat::ICombatEntity &attacker,
                                                ::combat::ICombatEntity &victim) {
  if (attacker.GetGuid() == 0 || victim.GetGuid() == 0)
    return MeleeSwingResult::InvalidTarget;
  if (!victim.IsAlive())
    return MeleeSwingResult::DeadTarget;
  if (!attacker.IsAlive())
    return MeleeSwingResult::CantAttack;

  StartCombat(attacker, victim);
  _engine->Update(attacker, victim);
  return MeleeSwingResult::Success;
}

MeleeSwingResult CombatService::ApplyMeleeHit(::combat::ICombatEntity &attacker,
                                              ::combat::ICombatEntity &victim) {
  if (attacker.GetGuid() == 0 || victim.GetGuid() == 0)
    return MeleeSwingResult::InvalidTarget;
  if (!victim.IsAlive())
    return MeleeSwingResult::DeadTarget;
  if (!attacker.IsAlive())
    return MeleeSwingResult::CantAttack;

  _engine->Update(attacker, victim);
  return MeleeSwingResult::Success;
}

} // namespace application

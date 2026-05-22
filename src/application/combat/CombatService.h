#pragma once

#include <domain/combat/CombatEngine.h>
#include <domain/combat/entities/ICombatEntity.h>
#include <cstdint>
#include <memory>

namespace application {

enum class MeleeSwingResult : uint8_t {
  Success = 0,
  InvalidTarget,
  DeadTarget,
  CantAttack,
  NotInRange,
};

class CombatService {
public:
  explicit CombatService(std::shared_ptr<::combat::CombatEngine> engine)
      : _engine(std::move(engine)) {}

  void StartCombat(::combat::ICombatEntity &attacker, ::combat::ICombatEntity &victim);

  /// First swing: enter combat and apply one hit.
  MeleeSwingResult BeginMeleeSwing(::combat::ICombatEntity &attacker,
                                   ::combat::ICombatEntity &victim);

  /// Auto-attack tick: damage only (combat already started).
  MeleeSwingResult ApplyMeleeHit(::combat::ICombatEntity &attacker,
                                 ::combat::ICombatEntity &victim);

private:
  std::shared_ptr<::combat::CombatEngine> _engine;
};

} // namespace application

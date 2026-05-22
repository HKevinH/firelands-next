#pragma once

#include <shared/Common.h>
#include <shared/game/PlayerPowerType.h>

namespace Firelands {

/// True when the spell's `Spell.dbc` `PowerType` matches the unit's primary POWER1 type.
inline bool SpellUsesCasterPrimaryPower(uint32 spellPowerType, uint8 casterPrimaryPowerType) {
  return spellPowerType == static_cast<uint32>(casterPrimaryPowerType);
}

/// Default primary power for a class when the session has no explicit snapshot byte.
inline uint8 DefaultCasterPrimaryPowerType(uint8 klass) {
  return static_cast<uint8>(GetDefaultPlayerPowerType(klass));
}

} // namespace Firelands

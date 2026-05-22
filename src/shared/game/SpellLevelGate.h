#pragma once

#include <shared/Common.h>

namespace Firelands {

/// True when `casterLevel` meets `SpellLevels.dbc` gate on the spell definition (0 = no gate).
inline bool SpellMeetsCasterLevelRequirement(uint8 requiredLevel, uint8 casterLevel) {
  if (requiredLevel == 0u)
    return true;
  return requiredLevel <= casterLevel;
}

} // namespace Firelands

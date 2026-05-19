#pragma once

#include <shared/Common.h>
#include <vector>

namespace Firelands {

struct PersistedSpellCooldown {
  uint32 spellId = 0;
  uint32 remainingMs = 0;
};

struct PersistedCategoryCooldown {
  uint32 category = 0;
  uint32 remainingMs = 0;
};

struct CharacterCooldownState {
  std::vector<PersistedSpellCooldown> spellCooldowns;
  std::vector<PersistedCategoryCooldown> categoryCooldowns;
};

} // namespace Firelands

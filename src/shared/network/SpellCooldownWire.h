#pragma once

#include <shared/Common.h>
#include <shared/network/WorldPacket.h>

namespace Firelands {
namespace SpellCooldownWire {

/// `SMSG_SPELL_COOLDOWN` flags (cmangos / mangos).
enum SpellCooldownFlags : uint8 {
  kNone = 0,
  kIncludeGcd = 1,
};

struct SpellCooldownEntry {
  uint32 spellId = 0;
  /// Remaining cooldown in milliseconds; GCD row uses 0 with `kIncludeGcd`.
  int32 remainingMs = 0;
};

/// Builds `SMSG_SPELL_COOLDOWN`: unit guid, flags, then `count` (spellId, remainingMs) pairs.
void BuildSpellCooldown(WorldPacket &out, uint64 unitGuid, uint8 flags,
                        SpellCooldownEntry const *entries, size_t entryCount);

struct CategoryCooldownEntry {
  int32 category = 0;
  int32 remainingMs = 0;
};

/// Builds `SMSG_CATEGORY_COOLDOWN` (count in 23 bits, then category + remaining pairs).
void BuildCategoryCooldown(WorldPacket &out, CategoryCooldownEntry const *entries,
                           size_t entryCount);

} // namespace SpellCooldownWire
} // namespace Firelands

#include <shared/network/BitWriter.h>
#include <shared/network/SpellCooldownWire.h>
#include <shared/network/WorldOpcodes.h>

namespace Firelands {
namespace SpellCooldownWire {

void BuildSpellCooldown(WorldPacket &out, uint64 unitGuid, uint8 flags,
                        SpellCooldownEntry const *entries, size_t entryCount) {
  size_t const reserve = 16u + entryCount * 8u;
  out = WorldPacket(SMSG_SPELL_COOLDOWN, reserve);
  out.Append<uint64>(unitGuid);
  out.Append<uint8>(flags);
  for (size_t i = 0; i < entryCount; ++i) {
    out.Append<uint32>(entries[i].spellId);
    out.Append<int32>(entries[i].remainingMs);
  }
}

void BuildCategoryCooldown(WorldPacket &out, CategoryCooldownEntry const *entries,
                           size_t entryCount) {
  out = WorldPacket(SMSG_CATEGORY_COOLDOWN, 8u + entryCount * 8u);
  BitWriter bw(out);
  bw.WriteBits(static_cast<uint32>(entryCount), 23);
  bw.Flush();
  for (size_t i = 0; i < entryCount; ++i) {
    out.Append<int32>(entries[i].category);
    out.Append<int32>(entries[i].remainingMs);
  }
}

} // namespace SpellCooldownWire
} // namespace Firelands

#include <shared/network/BitWriter.h>
#include <shared/network/SpellCooldownWire.h>
#include <shared/network/WorldOpcodes.h>

namespace Firelands {
namespace SpellCooldownWire {

namespace {

uint8 GuidByteLe(uint64 guid, unsigned index) {
  return static_cast<uint8>((guid >> (index * 8)) & 0xFFu);
}

} // namespace

void BuildCooldownEvent(WorldPacket &out, int32 spellId, bool isPet) {
  out = WorldPacket(SMSG_COOLDOWN_EVENT, 4u + 1u);
  out.Append<int32>(spellId);
  BitWriter bw(out);
  bw.WriteBit(isPet ? 1u : 0u);
  bw.Flush();
}

void BuildSpellCooldown(WorldPacket &out, uint64 unitGuid, uint8 flags,
                        std::vector<SpellCooldownStruct> const &entries) {
  size_t const reserve = 16u + entries.size() * 12u;
  out = WorldPacket(SMSG_SPELL_COOLDOWN, reserve);
  out.WritePackedGuid(unitGuid);
  out.Append<uint8>(flags);
  out.Append<uint32>(static_cast<uint32>(entries.size()));
  for (SpellCooldownStruct const &row : entries) {
    out.Append<uint32>(row.spellId);
    out.Append<uint32>(row.forcedCooldownMs);
    out.Append<float>(row.modRate);
  }
}

void BuildSpellCooldown(WorldPacket &out, uint64 unitGuid, uint8 flags,
                        SpellCooldownStruct const &entry) {
  BuildSpellCooldown(out, unitGuid, flags, std::vector<SpellCooldownStruct>{entry});
}

void BuildClearCooldowns(WorldPacket &out, uint64 unitGuid, uint32 const *spellIds,
                         size_t spellIdCount) {
  auto const G = [&](unsigned i) { return GuidByteLe(unitGuid, i); };
  size_t const reserve = 24u + spellIdCount * 4u;
  out = WorldPacket(SMSG_CLEAR_COOLDOWNS, reserve);

  BitWriter bw(out);
  bw.WriteBitMask(G(1));
  bw.WriteBitMask(G(3));
  bw.WriteBitMask(G(6));
  bw.WriteBits(static_cast<uint32>(spellIdCount), 24);
  bw.WriteBitMask(G(7));
  bw.WriteBitMask(G(5));
  bw.WriteBitMask(G(2));
  bw.WriteBitMask(G(4));
  bw.WriteBitMask(G(0));
  bw.Flush();

  out.WriteByteSeq(G(7));
  out.WriteByteSeq(G(2));
  out.WriteByteSeq(G(4));
  out.WriteByteSeq(G(5));
  out.WriteByteSeq(G(1));
  out.WriteByteSeq(G(3));
  for (size_t i = 0; i < spellIdCount; ++i)
    out.Append<uint32>(spellIds[i]);
  out.WriteByteSeq(G(0));
  out.WriteByteSeq(G(6));
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

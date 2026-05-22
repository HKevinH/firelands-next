#include <shared/network/PlaySpellVisualKitWire.h>

#include <shared/network/BitWriter.h>
#include <shared/network/WorldOpcodes.h>

namespace Firelands {
namespace PlaySpellVisualKitWire {

namespace {

uint8 GuidByteLe(uint64 guid, unsigned index) {
  return static_cast<uint8>((guid >> (index * 8)) & 0xFFu);
}

} // namespace

void BuildPlaySpellVisualKit(WorldPacket &out, uint64 unitGuid, int32 kitRecId,
                             int32 kitType, uint32 durationMs) {
  out = WorldPacket(SMSG_PLAY_SPELL_VISUAL_KIT, 32u);
  out.Append<uint32>(durationMs);
  out.Append<int32>(kitRecId);
  out.Append<int32>(kitType);

  auto G = [&](unsigned i) { return GuidByteLe(unitGuid, i); };
  BitWriter bw(out);
  bw.WriteBitMask(G(4));
  bw.WriteBitMask(G(7));
  bw.WriteBitMask(G(5));
  bw.WriteBitMask(G(3));
  bw.WriteBitMask(G(1));
  bw.WriteBitMask(G(2));
  bw.WriteBitMask(G(0));
  bw.WriteBitMask(G(6));
  bw.Flush();

  out.WriteByteSeq(G(0));
  out.WriteByteSeq(G(4));
  out.WriteByteSeq(G(1));
  out.WriteByteSeq(G(6));
  out.WriteByteSeq(G(7));
  out.WriteByteSeq(G(2));
  out.WriteByteSeq(G(3));
  out.WriteByteSeq(G(5));
}

} // namespace PlaySpellVisualKitWire
} // namespace Firelands

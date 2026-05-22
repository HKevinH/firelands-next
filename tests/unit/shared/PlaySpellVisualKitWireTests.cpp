#include <gtest/gtest.h>
#include <shared/network/BitReader.h>
#include <shared/network/PlaySpellVisualKitWire.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>

using namespace Firelands;

namespace {

uint8 GuidByteLe(uint64 guid, unsigned index) {
  return static_cast<uint8>((guid >> (index * 8)) & 0xFFu);
}

uint8 EncodedByteSeq(uint8 guidByte) { return static_cast<uint8>(guidByte ^ 1u); }

} // namespace

TEST(PlaySpellVisualKitWireTests, BuildMatchesCataLayout) {
  uint64 const unitGuid = 0x0000000000000F01ULL;
  WorldPacket pkt;
  PlaySpellVisualKitWire::BuildPlaySpellVisualKit(pkt, unitGuid, 348, 0, 0);

  EXPECT_EQ(pkt.GetOpcode(), static_cast<uint32>(SMSG_PLAY_SPELL_VISUAL_KIT));
  pkt.SetReadPos(0);
  EXPECT_EQ(pkt.Read<uint32>(), 0u);
  EXPECT_EQ(pkt.Read<int32>(), 348);
  EXPECT_EQ(pkt.Read<int32>(), 0);

  auto G = [&](unsigned i) { return GuidByteLe(unitGuid, i); };
  BitReader br(pkt);
  EXPECT_EQ(br.ReadBit(), G(4) != 0);
  EXPECT_EQ(br.ReadBit(), G(7) != 0);
  EXPECT_EQ(br.ReadBit(), G(5) != 0);
  EXPECT_EQ(br.ReadBit(), G(3) != 0);
  EXPECT_EQ(br.ReadBit(), G(1) != 0);
  EXPECT_EQ(br.ReadBit(), G(2) != 0);
  EXPECT_EQ(br.ReadBit(), G(0) != 0);
  EXPECT_EQ(br.ReadBit(), G(6) != 0);
  br.AlignToByteBoundary();

  EXPECT_EQ(pkt.Read<uint8>(), EncodedByteSeq(G(0)));
  EXPECT_EQ(pkt.Read<uint8>(), EncodedByteSeq(G(1)));
}

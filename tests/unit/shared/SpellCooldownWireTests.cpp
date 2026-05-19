#include <gtest/gtest.h>
#include <shared/network/BitReader.h>
#include <shared/network/SpellCooldownWire.h>
#include <shared/network/WorldOpcodes.h>

using namespace Firelands;

TEST(SpellCooldownWireTests, BuildSpellCooldownBatchesMultipleSpells) {
  SpellCooldownWire::SpellCooldownEntry entries[] = {{100u, 5000}, {200u, 3000}};
  WorldPacket pkt;
  SpellCooldownWire::BuildSpellCooldown(pkt, 0x99ULL, SpellCooldownWire::kNone, entries, 2);
  pkt.SetReadPos(0);
  EXPECT_EQ(pkt.Read<uint64>(), 0x99ULL);
  EXPECT_EQ(pkt.Read<uint8>(), SpellCooldownWire::kNone);
  EXPECT_EQ(pkt.Read<uint32>(), 100u);
  EXPECT_EQ(pkt.Read<int32>(), 5000);
  EXPECT_EQ(pkt.Read<uint32>(), 200u);
  EXPECT_EQ(pkt.Read<int32>(), 3000);
}

TEST(SpellCooldownWireTests, BuildSpellCooldownRecoveryAndGcdPackets) {
  SpellCooldownWire::SpellCooldownEntry gcdEntry{774u, 0};
  WorldPacket gcdPkt;
  SpellCooldownWire::BuildSpellCooldown(gcdPkt, 0x50ULL, SpellCooldownWire::kIncludeGcd,
                                        &gcdEntry, 1);
  EXPECT_EQ(gcdPkt.GetOpcode(), SMSG_SPELL_COOLDOWN);
  gcdPkt.SetReadPos(0);
  EXPECT_EQ(gcdPkt.Read<uint64>(), 0x50ULL);
  EXPECT_EQ(gcdPkt.Read<uint8>(), SpellCooldownWire::kIncludeGcd);
  EXPECT_EQ(gcdPkt.Read<uint32>(), 774u);
  EXPECT_EQ(gcdPkt.Read<int32>(), 0);

  SpellCooldownWire::SpellCooldownEntry cdEntry{774u, 8000};
  WorldPacket cdPkt;
  SpellCooldownWire::BuildSpellCooldown(cdPkt, 0x50ULL, SpellCooldownWire::kNone, &cdEntry,
                                        1);
  cdPkt.SetReadPos(0);
  EXPECT_EQ(cdPkt.Read<uint64>(), 0x50ULL);
  EXPECT_EQ(cdPkt.Read<uint8>(), SpellCooldownWire::kNone);
  EXPECT_EQ(cdPkt.Read<uint32>(), 774u);
  EXPECT_EQ(cdPkt.Read<int32>(), 8000);
}

TEST(SpellCooldownWireTests, BuildCategoryCooldownEmptyCount) {
  WorldPacket pkt;
  SpellCooldownWire::BuildCategoryCooldown(pkt, nullptr, 0);
  EXPECT_EQ(pkt.GetOpcode(), SMSG_CATEGORY_COOLDOWN);
  pkt.SetReadPos(0);
  BitReader br(pkt);
  EXPECT_EQ(br.ReadBits(23), 0u);
  br.AlignToByteBoundary();
  EXPECT_EQ(pkt.GetReadPos(), pkt.Size());
}

TEST(SpellCooldownWireTests, BuildCategoryCooldownEntries) {
  SpellCooldownWire::CategoryCooldownEntry entries[] = {{11, 4000}, {22, 9000}};
  WorldPacket pkt;
  SpellCooldownWire::BuildCategoryCooldown(pkt, entries, 2);
  pkt.SetReadPos(0);
  BitReader br(pkt);
  EXPECT_EQ(br.ReadBits(23), 2u);
  br.AlignToByteBoundary();
  EXPECT_EQ(pkt.Read<int32>(), 11);
  EXPECT_EQ(pkt.Read<int32>(), 4000);
  EXPECT_EQ(pkt.Read<int32>(), 22);
  EXPECT_EQ(pkt.Read<int32>(), 9000);
}

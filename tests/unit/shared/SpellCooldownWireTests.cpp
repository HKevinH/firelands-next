#include <gtest/gtest.h>
#include <shared/network/BitReader.h>
#include <shared/network/SpellCooldownWire.h>
#include <shared/network/WorldOpcodes.h>

using namespace Firelands;

TEST(SpellCooldownWireTests, BuildCooldownEventWritesSpellIdAndPetBit) {
  WorldPacket pkt;
  SpellCooldownWire::BuildCooldownEvent(pkt, 133, false);
  EXPECT_EQ(pkt.GetOpcode(), SMSG_COOLDOWN_EVENT);
  pkt.SetReadPos(0);
  EXPECT_EQ(pkt.Read<int32>(), 133);
  BitReader br(pkt);
  EXPECT_EQ(br.ReadBit(), 0u);
  br.AlignToByteBoundary();
  EXPECT_EQ(pkt.GetReadPos(), pkt.Size());
}

TEST(SpellCooldownWireTests, BuildSpellCooldownUsesCataclysmRowLayout) {
  std::vector<SpellCooldownWire::SpellCooldownStruct> rows{
      {100u, 5000u, 1.0f}, {200u, 3000u, 1.0f}};
  WorldPacket pkt;
  SpellCooldownWire::BuildSpellCooldown(pkt, 0x0BULL, SpellCooldownWire::kNone, rows);
  EXPECT_EQ(pkt.GetOpcode(), SMSG_SPELL_COOLDOWN);
  pkt.SetReadPos(0);
  EXPECT_EQ(pkt.ReadPackedGuid(), 0x0BULL);
  EXPECT_EQ(pkt.Read<uint8>(), SpellCooldownWire::kNone);
  EXPECT_EQ(pkt.Read<uint32>(), 2u);
  EXPECT_EQ(pkt.Read<uint32>(), 100u);
  EXPECT_EQ(pkt.Read<uint32>(), 5000u);
  EXPECT_FLOAT_EQ(pkt.Read<float>(), 1.0f);
  EXPECT_EQ(pkt.Read<uint32>(), 200u);
  EXPECT_EQ(pkt.Read<uint32>(), 3000u);
  EXPECT_FLOAT_EQ(pkt.Read<float>(), 1.0f);
}

TEST(SpellCooldownWireTests, BuildSpellCooldownRecoveryAndGcdPackets) {
  SpellCooldownWire::SpellCooldownStruct gcdRow{774u, 1500u, 1.0f};
  WorldPacket gcdPkt;
  SpellCooldownWire::BuildSpellCooldown(gcdPkt, 0x50ULL, SpellCooldownWire::kIncludeGcd,
                                        gcdRow);
  gcdPkt.SetReadPos(0);
  EXPECT_EQ(gcdPkt.ReadPackedGuid(), 0x50ULL);
  EXPECT_EQ(gcdPkt.Read<uint8>(), SpellCooldownWire::kIncludeGcd);
  EXPECT_EQ(gcdPkt.Read<uint32>(), 1u);
  EXPECT_EQ(gcdPkt.Read<uint32>(), 774u);
  EXPECT_EQ(gcdPkt.Read<uint32>(), 1500u);
  EXPECT_FLOAT_EQ(gcdPkt.Read<float>(), 1.0f);

  SpellCooldownWire::SpellCooldownStruct cdRow{774u, 8000u, 1.0f};
  WorldPacket cdPkt;
  SpellCooldownWire::BuildSpellCooldown(cdPkt, 0x50ULL, SpellCooldownWire::kNone, cdRow);
  cdPkt.SetReadPos(0);
  EXPECT_EQ(cdPkt.ReadPackedGuid(), 0x50ULL);
  EXPECT_EQ(cdPkt.Read<uint8>(), SpellCooldownWire::kNone);
  EXPECT_EQ(cdPkt.Read<uint32>(), 1u);
  EXPECT_EQ(cdPkt.Read<uint32>(), 774u);
  EXPECT_EQ(cdPkt.Read<uint32>(), 8000u);
  EXPECT_FLOAT_EQ(cdPkt.Read<float>(), 1.0f);
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

TEST(SpellCooldownWireTests, BuildClearCooldownsWritesSpellIdList) {
  uint32 const spellIds[] = {133u, 16857u};
  WorldPacket pkt;
  SpellCooldownWire::BuildClearCooldowns(pkt, 0x0000000000000040ULL, spellIds, 2);
  EXPECT_EQ(pkt.GetOpcode(), SMSG_CLEAR_COOLDOWNS);
  pkt.SetReadPos(0);
  BitReader br(pkt);
  EXPECT_EQ(br.ReadBit(), 0u);
  EXPECT_EQ(br.ReadBit(), 0u);
  EXPECT_EQ(br.ReadBit(), 0u);
  EXPECT_EQ(br.ReadBits(24), 2u);
  EXPECT_EQ(br.ReadBit(), 0u);
  EXPECT_EQ(br.ReadBit(), 0u);
  EXPECT_EQ(br.ReadBit(), 0u);
  EXPECT_EQ(br.ReadBit(), 0u);
  EXPECT_EQ(br.ReadBit(), 1u);
  br.AlignToByteBoundary();
  EXPECT_EQ(pkt.Read<uint32>(), 133u);
  EXPECT_EQ(pkt.Read<uint32>(), 16857u);
  EXPECT_EQ(pkt.Read<uint8>(), 0x41u);
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

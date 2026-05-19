#include <gtest/gtest.h>
#include <shared/network/AuraUpdateWire.h>
#include <shared/network/WorldOpcodes.h>

using namespace Firelands;

TEST(AuraUpdateWireTests, BuildAuraApplySelfCastUsesCataUInt16Flags) {
  WorldPacket pkt;
  AuraUpdateWire::AuraApplyParams params{};
  params.visualSlot = 0;
  params.spellId = 774;
  params.effectIndex = 0;
  params.casterGuid = 0x50ULL;
  params.durationMs = 3000;
  AuraUpdateWire::BuildAuraApply(pkt, 0x50ULL, params);

  pkt.SetReadPos(0);
  (void)pkt.ReadPackedGuid();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<int32>();
  EXPECT_EQ(pkt.Read<uint16>(),
            static_cast<uint16>(AuraUpdateWire::kEffectIndex0 |
                                AuraUpdateWire::kNotCaster |
                                AuraUpdateWire::kPositive | AuraUpdateWire::kDuration));
}

TEST(AuraUpdateWireTests, BuildAuraApplySelfCastOmitsCasterGuid) {
  WorldPacket pkt;
  AuraUpdateWire::AuraApplyParams params{};
  params.visualSlot = 1;
  params.spellId = 774;
  params.effectIndex = 0;
  params.casterLevel = 80;
  params.casterGuid = 0x100ULL;
  params.durationMs = 12000;
  params.remainingMs = 12000;
  params.isNegative = false;
  AuraUpdateWire::BuildAuraApply(pkt, 0x100ULL, params);

  EXPECT_EQ(pkt.GetOpcode(), SMSG_AURA_UPDATE);
  pkt.SetReadPos(0);
  EXPECT_EQ(pkt.ReadPackedGuid(), 0x100ULL);
  EXPECT_EQ(pkt.Read<uint8>(), 1u);
  EXPECT_EQ(pkt.Read<int32>(), 774);
  uint16 const flags = pkt.Read<uint16>();
  EXPECT_NE(flags & AuraUpdateWire::kEffectIndex0, 0u);
  EXPECT_NE(flags & AuraUpdateWire::kNotCaster, 0u);
  EXPECT_NE(flags & AuraUpdateWire::kPositive, 0u);
  EXPECT_NE(flags & AuraUpdateWire::kDuration, 0u);
  EXPECT_EQ(flags & AuraUpdateWire::kNegative, 0u);
  (void)pkt.Read<uint8>();
  (void)pkt.Read<uint8>();
  EXPECT_EQ(pkt.Read<int32>(), 12000);
  EXPECT_EQ(pkt.Read<int32>(), 12000);
  EXPECT_EQ(pkt.GetReadPos(), pkt.Size());
}

TEST(AuraUpdateWireTests, BuildAuraApplyOtherTargetIncludesCasterGuid) {
  WorldPacket pkt;
  AuraUpdateWire::AuraApplyParams params{};
  params.visualSlot = 2;
  params.spellId = 774;
  params.effectIndex = 0;
  params.casterLevel = 80;
  params.casterGuid = 0x100ULL;
  params.durationMs = 15000;
  params.remainingMs = 15000;
  params.isNegative = false;
  AuraUpdateWire::BuildAuraApply(pkt, 0x200ULL, params);

  pkt.SetReadPos(0);
  EXPECT_EQ(pkt.ReadPackedGuid(), 0x200ULL);
  EXPECT_EQ(pkt.Read<uint8>(), 2u);
  EXPECT_EQ(pkt.Read<int32>(), 774);
  uint16 const flags = pkt.Read<uint16>();
  EXPECT_NE(flags & AuraUpdateWire::kEffectIndex0, 0u);
  EXPECT_EQ(flags & AuraUpdateWire::kNotCaster, 0u);
  EXPECT_NE(flags & AuraUpdateWire::kPositive, 0u);
  (void)pkt.Read<uint8>();
  (void)pkt.Read<uint8>();
  EXPECT_EQ(pkt.ReadPackedGuid(), 0x100ULL);
  EXPECT_EQ(pkt.Read<int32>(), 15000);
  EXPECT_EQ(pkt.Read<int32>(), 15000);
}

TEST(AuraUpdateWireTests, BuildAuraRemoveIsSlotAndZeroSpellOnly) {
  WorldPacket pkt;
  AuraUpdateWire::BuildAuraRemove(pkt, 0x300ULL, 4);
  pkt.SetReadPos(0);
  EXPECT_EQ(pkt.ReadPackedGuid(), 0x300ULL);
  EXPECT_EQ(pkt.Read<uint8>(), 4u);
  EXPECT_EQ(pkt.Read<int32>(), 0);
  EXPECT_EQ(pkt.GetReadPos(), pkt.Size());
}

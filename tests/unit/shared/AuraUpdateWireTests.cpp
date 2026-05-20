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

TEST(AuraUpdateWireTests, BuildAuraApplyRejuvenationSendsTwoEffectAmounts) {
  WorldPacket pkt;
  AuraUpdateWire::AuraApplyParams params{};
  params.visualSlot = 0;
  params.spellId = 774;
  params.effectIndex = 0;
  params.activeEffectMask = 0x03;
  params.casterLevel = 80;
  params.casterGuid = 0x100ULL;
  params.durationMs = 12000;
  params.remainingMs = 12000;
  params.sendEffectAmount = true;
  params.effectAmount = 42;
  AuraUpdateWire::BuildAuraApply(pkt, 0x100ULL, params);

  pkt.SetReadPos(0);
  (void)pkt.ReadPackedGuid();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<int32>();
  uint16 const flags = pkt.Read<uint16>();
  EXPECT_NE(flags & AuraUpdateWire::kEffectIndex0, 0u);
  EXPECT_NE(flags & AuraUpdateWire::kEffectIndex1, 0u);
  EXPECT_NE(flags & AuraUpdateWire::kAnyEffectAmountSent, 0u);
  (void)pkt.Read<uint8>();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<int32>();
  (void)pkt.Read<int32>();
  EXPECT_EQ(pkt.Read<int32>(), 42);
  EXPECT_EQ(pkt.Read<int32>(), 0);
  EXPECT_EQ(pkt.GetReadPos(), pkt.Size());
}

TEST(AuraUpdateWireTests, BuildAuraApplyPeriodicHealIncludesEffectAmount) {
  WorldPacket pkt;
  AuraUpdateWire::AuraApplyParams params{};
  params.visualSlot = 0;
  params.spellId = 774;
  params.effectIndex = 0;
  params.casterLevel = 80;
  params.casterGuid = 0x100ULL;
  params.durationMs = 12000;
  params.remainingMs = 12000;
  params.sendEffectAmount = true;
  params.effectAmount = 42;
  AuraUpdateWire::BuildAuraApply(pkt, 0x100ULL, params);

  pkt.SetReadPos(0);
  (void)pkt.ReadPackedGuid();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<int32>();
  uint16 const flags = pkt.Read<uint16>();
  EXPECT_NE(flags & AuraUpdateWire::kAnyEffectAmountSent, 0u);
  (void)pkt.Read<uint8>();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<int32>();
  (void)pkt.Read<int32>();
  EXPECT_EQ(pkt.Read<int32>(), 42);
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

TEST(AuraUpdateWireTests, BuildAuraExpireSetsRemainingToZero) {
  WorldPacket pkt;
  AuraUpdateWire::AuraApplyParams params{};
  params.visualSlot = 2;
  params.spellId = 774;
  params.effectIndex = 0;
  params.casterLevel = 80;
  params.casterGuid = 0x100ULL;
  params.durationMs = 12000;
  params.remainingMs = 12000;
  params.sendEffectAmount = true;
  params.effectAmount = 10;
  AuraUpdateWire::BuildAuraExpire(pkt, 0x100ULL, params);

  pkt.SetReadPos(0);
  (void)pkt.ReadPackedGuid();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<int32>();
  (void)pkt.Read<uint16>();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<uint8>();
  EXPECT_EQ(pkt.Read<int32>(), 12000);
  EXPECT_EQ(pkt.Read<int32>(), 0);
  EXPECT_EQ(pkt.Read<int32>(), 10);
}

TEST(AuraUpdateWireTests, BuildAuraUpdateAllEncodesMultipleAuras) {
  WorldPacket pkt;
  std::vector<AuraUpdateWire::AuraApplyParams> auras(2);
  auras[0].visualSlot = 0;
  auras[0].spellId = 774;
  auras[0].effectIndex = 0;
  auras[0].activeEffectMask = 0x03;
  auras[0].durationMs = 12000;
  auras[0].remainingMs = 9000;
  auras[0].explicitRemaining = true;
  auras[0].sendEffectAmount = true;
  auras[0].effectAmount = 10;
  auras[1].visualSlot = 1;
  auras[1].spellId = 1126;
  auras[1].effectIndex = 0;
  auras[1].durationMs = 600000;
  auras[1].remainingMs = 600000;
  AuraUpdateWire::BuildAuraUpdateAll(pkt, 0x200ULL, auras);

  EXPECT_EQ(pkt.GetOpcode(), SMSG_AURA_UPDATE_ALL);
  pkt.SetReadPos(0);
  EXPECT_EQ(pkt.ReadPackedGuid(), 0x200ULL);
  EXPECT_EQ(pkt.Read<uint8>(), 0u);
  EXPECT_EQ(pkt.Read<int32>(), 774);
  (void)pkt.Read<uint16>();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<uint8>();
  (void)pkt.Read<int32>();
  (void)pkt.Read<int32>();
  EXPECT_EQ(pkt.Read<int32>(), 10);
  EXPECT_EQ(pkt.Read<int32>(), 0);
  EXPECT_EQ(pkt.Read<uint8>(), 1u);
  EXPECT_EQ(pkt.Read<int32>(), 1126);
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

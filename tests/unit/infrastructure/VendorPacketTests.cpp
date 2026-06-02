#include <shared/game/WowGuid.h>
#include <shared/network/BitReader.h>
#include <shared/network/packets/server/VendorPackets.h>
#include <shared/network/WorldOpcodes.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

namespace Firelands {
namespace {

/// Mirrors WowPacketParser `V4_3_4_15595` `HandleVendorInventoryList` read order.
void ParseVendorInventoryEmpty(WorldPacket const &pkt, uint64_t &outGuid, uint32_t &outCount,
                               uint8_t &outReason) {
  WorldPacket copy = pkt;
  copy.SetReadPos(0);
  outGuid = 0;

  uint8_t mask[8] = {};
  BitReader br(copy);
  mask[1] = br.ReadBit();
  mask[0] = br.ReadBit();
  outCount = br.ReadBits(21);
  mask[3] = br.ReadBit();
  mask[6] = br.ReadBit();
  mask[5] = br.ReadBit();
  mask[2] = br.ReadBit();
  mask[7] = br.ReadBit();
  mask[4] = br.ReadBit();
  br.AlignToByteBoundary();

  auto readXor = [&](int idx) {
    if (!mask[idx])
      return;
    uint8_t b = copy.Read<uint8_t>() ^ 1u;
    outGuid |= static_cast<uint64_t>(b) << (idx * 8);
  };

  readXor(5);
  readXor(4);
  readXor(1);
  readXor(0);
  readXor(6);
  outReason = copy.Read<uint8_t>();
  readXor(2);
  readXor(3);
  readXor(7);
}

} // namespace

/// Reads the populated-list header (no extendedCost / no playerCondition per
/// item), then returns the per-item int32 data block in append order.
struct ParsedVendorItem {
  int32_t muId, durability, itemId, type, price, displayId, quantity, stackCount;
};

void ParseVendorInventoryItems(WorldPacket const &pkt, uint64_t &outGuid,
                               uint32_t &outCount, uint8_t &outReason,
                               std::vector<ParsedVendorItem> &outItems) {
  WorldPacket copy = pkt;
  copy.SetReadPos(0);
  outGuid = 0;

  uint8_t mask[8] = {};
  BitReader br(copy);
  mask[1] = br.ReadBit();
  mask[0] = br.ReadBit();
  outCount = br.ReadBits(21);
  mask[3] = br.ReadBit();
  mask[6] = br.ReadBit();
  mask[5] = br.ReadBit();
  mask[2] = br.ReadBit();
  mask[7] = br.ReadBit();
  // Per item: !hasExtendedCost, !hasPlayerCondition.
  for (uint32_t i = 0; i < outCount; ++i) {
    (void)br.ReadBit();
    (void)br.ReadBit();
  }
  mask[4] = br.ReadBit();
  br.AlignToByteBoundary();

  for (uint32_t i = 0; i < outCount; ++i) {
    ParsedVendorItem it{};
    it.muId = copy.Read<int32_t>();
    it.durability = copy.Read<int32_t>();
    it.itemId = copy.Read<int32_t>();
    it.type = copy.Read<int32_t>();
    it.price = copy.Read<int32_t>();
    it.displayId = copy.Read<int32_t>();
    it.quantity = copy.Read<int32_t>();
    it.stackCount = copy.Read<int32_t>();
    outItems.push_back(it);
  }

  auto readXor = [&](int idx) {
    if (!mask[idx])
      return;
    uint8_t b = copy.Read<uint8_t>() ^ 1u;
    outGuid |= static_cast<uint64_t>(b) << (idx * 8);
  };
  readXor(5);
  readXor(4);
  readXor(1);
  readXor(0);
  readXor(6);
  outReason = copy.Read<uint8_t>();
  readXor(2);
  readXor(3);
  readXor(7);
}

TEST(VendorPacketTests, BuildVendorInventory_PopulatedListRoundTrips) {
  uint64_t const vendorGuid = MakeCreatureObjectGuid(54, 0x70000002u);

  vendor_wire::VendorWireItem a;
  a.muId = 1;
  a.itemId = 2488;
  a.type = 1;
  a.price = 100;
  a.itemDisplayInfoId = 12345;
  a.quantity = -1;
  a.stackCount = 1;

  vendor_wire::VendorWireItem b;
  b.muId = 2;
  b.itemId = 2489;
  b.type = 1;
  b.price = 250;
  b.itemDisplayInfoId = 6789;
  b.quantity = 5;
  b.stackCount = 1;

  WorldPacket pkt = vendor_wire::BuildVendorInventory(vendorGuid, {a, b});

  uint64_t parsedGuid = 0;
  uint32_t count = 0;
  uint8_t reason = 0;
  std::vector<ParsedVendorItem> items;
  ParseVendorInventoryItems(pkt, parsedGuid, count, reason, items);

  EXPECT_EQ(parsedGuid, vendorGuid);
  EXPECT_EQ(count, 2u);
  EXPECT_EQ(reason, 0u); // non-empty -> reason byte 0
  ASSERT_EQ(items.size(), 2u);
  EXPECT_EQ(items[0].itemId, 2488);
  EXPECT_EQ(items[0].price, 100);
  EXPECT_EQ(items[0].displayId, 12345);
  EXPECT_EQ(items[0].quantity, -1);
  EXPECT_EQ(items[1].itemId, 2489);
  EXPECT_EQ(items[1].price, 250);
  EXPECT_EQ(items[1].quantity, 5);
}

TEST(VendorPacketTests, BuildVendorInventory_EmptyListMatches15595Layout) {
  uint64_t const vendorGuid = MakeCreatureObjectGuid(1234, 0x70000002u);
  WorldPacket pkt = vendor_wire::BuildVendorInventory(vendorGuid, {});

  EXPECT_EQ(pkt.GetOpcode(), static_cast<uint32>(SMSG_VENDOR_INVENTORY));

  uint64_t parsedGuid = 0;
  uint32_t count = 99;
  uint8_t reason = 0;
  ParseVendorInventoryEmpty(pkt, parsedGuid, count, reason);

  EXPECT_EQ(parsedGuid, vendorGuid);
  EXPECT_EQ(count, 0u);
  EXPECT_EQ(reason, 1u);
}

} // namespace Firelands

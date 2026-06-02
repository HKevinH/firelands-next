#include <infrastructure/network/sessions/WorldSession.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionObjectUpdate.h>
#include <domain/repositories/IVendorRepository.h>
#include <shared/Logger.h>
#include <shared/dbc/ItemTemplateStore.h>
#include <shared/game/InventorySlots.h>
#include <shared/network/BitReader.h>
#include <shared/network/UpdateData.h>
#include <algorithm>
#include <array>
#include <ctime>
#include <optional>
#include <vector>

namespace Firelands {

namespace {

bool IsItemDbTableHash(uint32_t tableHash) {
  // Legacy / reference hashes; real clients use the `table_hash` inside each WDB2
  // file — `ItemDbHotfixStore` matches those after loading from disk.
  return tableHash == 0x50238EC2u || tableHash == 0x6A7C6E76u ||
         tableHash == 0x919BE54Eu;
  }

void SendDbReplyUseClientDb2(WorldSession &session, uint32_t tableHash,
                             uint32_t recordId) {
  // `HandleDBQueryBulk`: negative `RecordID` + empty payload → client uses
  // embedded Item.db2 / Item-sparse.db2 (no server hotfix row).
  WorldPacket reply(SMSG_DB_REPLY);
  int32_t const neg = -static_cast<int32_t>(recordId);
  reply.Append<int32_t>(neg);
  reply.Append<uint32_t>(tableHash);
  reply.Append<uint32_t>(static_cast<uint32_t>(std::time(nullptr)));
  reply.Append<uint32_t>(0);
  session.SendPacket(reply);
  }

uint8_t NormalizeBag0ItemSlot(uint8_t bag, uint8_t slot) {
  if (bag != 0 && bag != CLIENT_INVENTORY_SLOT_DEFAULT_BACKPACK)
    return slot;

  // Clients may send backpack slots relative to the 16-slot grid (0..15) for
  // CMSG_AUTO_EQUIP_ITEM; storage uses bag0 absolute inventory slots (23..38).
  if (slot < kPackSlotCount) {
    return static_cast<uint8_t>(INVENTORY_SLOT_ITEM_START + slot);
  }
    return slot;
  }

  /// `WorldPackets::Item::InvUpdate` / `ItemPacketsCommon.cpp` operator>>.
/// Bit prefix then byte pairs (ContainerSlot, Slot) per item.
/// @return -1 if truncated, else Inv.Items.size() (0..3).
int ReadItemInvUpdatePrefix(WorldPacket &packet) {
  if (packet.Size() - packet.GetReadPos() < 1)
    return -1;
  BitReader br(packet);
  uint32_t const itemCount = br.ReadBits(2);
  br.AlignToByteBoundary();
  for (uint32_t i = 0; i < itemCount; ++i) {
    if (packet.Size() - packet.GetReadPos() < 2)
    return -1;
    (void)packet.Read<uint8_t>();
    (void)packet.Read<uint8_t>();
  }
  return static_cast<int>(itemCount);
  }

  /// `WorldPackets::Item::InventoryChangeFailure::Write()` (default branch).
/// Without this, the 4.3.4 client keeps the item greyed "pending" when equip is rejected.
void SendInventoryChangeFailure(WorldSession &session, int32_t bagResult) {
  WorldPacket pkt(SMSG_INVENTORY_CHANGE_FAILURE, 48);
  pkt.Append<int32_t>(bagResult);
  pkt.AppendPackGUID(0);
  pkt.AppendPackGUID(0);
  pkt.Append<uint8_t>(0); // ContainerBSlot
  session.SendPacket(pkt);
  }

  // `InventoryResult` (client inventory error text).
int32_t constexpr kEquipErrItemNotFound = 18;

std::optional<uint8_t> FindBag0SlotByItemLowGuid(Character const &ch,
                                                  uint32_t itemLowGuid) {
  if (itemLowGuid == 0)
    return std::nullopt;
  for (size_t s = 0; s < kEquipmentSlotCount; ++s) {
    if (ch.GetVisibleItemGuidLow(s) == itemLowGuid)
      return static_cast<uint8_t>(s);
  }
  for (size_t p = 0; p < kPackSlotCount; ++p) {
    if (ch.GetPackItemGuidLow(p) == itemLowGuid) {
      return static_cast<uint8_t>(INVENTORY_SLOT_ITEM_START + p);
  }
  }
    return std::nullopt;
  }

// `BuyResult` (SMSG_BUY_FAILED reason). Subset used by the vendor handlers.
uint8_t constexpr kBuyErrCantFindItem = 0;
uint8_t constexpr kBuyErrNotEnoughMoney = 2;
uint8_t constexpr kBuyErrCantCarryMore = 8;

// `SellResult` (SMSG_SELL_ITEM reason).
uint8_t constexpr kSellErrCantFindItem = 1;
uint8_t constexpr kSellErrCantSellItem = 2;

// Buyback UI slots start here (`PLAYER_FIELD_VENDORBUYBACK_SLOT_1` index base).
uint32_t constexpr kBuybackSlotStart = 74;

struct BackpackItemRef {
  uint8_t packIndex = 0;
  uint32_t entry = 0;
  uint32_t stack = 0;
};

/// Finds a main-backpack (bag 0 grid) item by its low GUID. Equipment slots are
/// intentionally excluded — equipped items are not sellable from the bag UI.
std::optional<BackpackItemRef> FindBackpackItemByLowGuid(Character const &ch,
                                                         uint32_t itemLowGuid) {
  if (itemLowGuid == 0)
    return std::nullopt;
  for (size_t p = 0; p < kPackSlotCount; ++p) {
    if (ch.GetPackItemGuidLow(p) == itemLowGuid) {
      BackpackItemRef r;
      r.packIndex = static_cast<uint8_t>(p);
      r.entry = ch.GetPackItemEntry(p);
      r.stack = ch.GetPackItemStackCount(p);
      return r;
    }
  }
  return std::nullopt;
}

void SendSellResult(WorldSession &session, uint64_t vendorGuid,
                    uint64_t itemGuid, uint8_t reason) {
  // `WorldSession::SendSellError` (4.3.4): vendor guid, item guid, reason byte.
  WorldPacket pkt(SMSG_SELL_ITEM, 8 + 8 + 1);
  pkt.Append<uint64_t>(vendorGuid);
  pkt.Append<uint64_t>(itemGuid);
  pkt.Append<uint8_t>(reason);
  session.SendPacket(pkt);
}

} // namespace

void WorldSession::HandleDbQueryBulk(WorldPacket &packet) {
  if (packet.Size() - packet.GetReadPos() < sizeof(uint32_t))
    return;

  uint32_t const tableHash = packet.Read<uint32_t>();
  BitReader br(packet);
  uint32_t const count = br.ReadBits(23);

  std::vector<std::array<bool, 8>> masks(count);
  for (uint32_t i = 0; i < count; ++i) {
    masks[i][0] = br.ReadBit() != 0;
    masks[i][4] = br.ReadBit() != 0;
    masks[i][7] = br.ReadBit() != 0;
    masks[i][2] = br.ReadBit() != 0;
    masks[i][5] = br.ReadBit() != 0;
    masks[i][3] = br.ReadBit() != 0;
    masks[i][6] = br.ReadBit() != 0;
    masks[i][1] = br.ReadBit() != 0;
  }
  br.AlignToByteBoundary();

  auto skipMaskByte = [&](uint32_t q, int idx) {
    if (masks[q][idx])
    (void)packet.Read<uint8_t>();
  };

  std::vector<uint32_t> recordIds;
  recordIds.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    skipMaskByte(i, 5);
    skipMaskByte(i, 6);
    skipMaskByte(i, 7);
    skipMaskByte(i, 0);
    skipMaskByte(i, 1);
    skipMaskByte(i, 3);
    skipMaskByte(i, 4);
    if (packet.Size() - packet.GetReadPos() < sizeof(uint32_t)) {
      LOG_WARN("HandleDbQueryBulk: truncated at record {}/{}", i, count);
    return;
  }
    recordIds.push_back(packet.Read<uint32_t>());
    skipMaskByte(i, 2);
  }

  bool const itemDbQuery =
      IsItemDbTableHash(tableHash) ||
      (_itemDbHotfix && _itemDbHotfix->handlesTableHash(tableHash));
  if (!itemDbQuery) {
    static uint32_t s_lastUnknownHash = 0;
    if (tableHash != s_lastUnknownHash) {
      s_lastUnknownHash = tableHash;
      LOG_DEBUG("HandleDbQueryBulk: unhandled tableHash=0x{:08X} ({} queries) — no "
                "SMSG_DB_REPLY (non-item DB2)",
                tableHash, count);
  }
    return;
  }

  for (uint32_t entry : recordIds) {
    if (_itemDbHotfix) {
      if (auto hotfix = _itemDbHotfix->tryBuildDbReply(tableHash, entry)) {
        SendPacket(*hotfix);
        continue;
  }
  }
    SendDbReplyUseClientDb2(*this, tableHash, entry);
  }
  }

void WorldSession::HandleAutoEquipItem(WorldPacket &packet) {
  if (_playerGuid == 0)
    return;
  // `EQUIP_ERR_CANT_EQUIP_OTHER` — generic failure for minimal SMSG.
  int32_t constexpr kEquipErrCantEquipOther = 15;

  size_t const payloadStart = packet.GetReadPos();
  size_t const rem = packet.Size() - payloadStart;

  uint8_t packSlot = 0;
  uint8_t slot = 0;
  bool parsed = false;
  bool usedInvUpdatePrefix = false;
  int invItems = -1;

  // Some 4.3.4 clients send only two bytes: PackSlot + Slot.
  if (rem == 2) {
    packSlot = packet.Read<uint8_t>();
    slot = packet.Read<uint8_t>();
    parsed = true;
  } else {
    // Format: InvUpdate prefix then PackSlot + Slot.
    packet.SetReadPos(payloadStart);
    invItems = ReadItemInvUpdatePrefix(packet);
    if (invItems >= 0 && (packet.Size() - packet.GetReadPos()) >= 2) {
    packSlot = packet.Read<uint8_t>();
    slot = packet.Read<uint8_t>();
    parsed = true;
      usedInvUpdatePrefix = true;
  }
  }

  if (!parsed) {
    LOG_WARN("HandleAutoEquipItem: unsupported payload size={} (could not parse)",
             rem);
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  uint8_t const normalizedSlot = NormalizeBag0ItemSlot(packSlot, slot);

  LOG_DEBUG(
      "HandleAutoEquipItem: account={} guid={} packSlot={} slot={} normalizedSlot={} "
      "consumed={} usedInvPrefix={} invItems={}",
      _accountId, _playerGuid, packSlot, slot, normalizedSlot,
      packet.GetReadPos() - payloadStart, usedInvUpdatePrefix, invItems);

  if (!_charService->AutoEquipFromBag0(_accountId, static_cast<uint32_t>(_playerGuid),
                                        packSlot, normalizedSlot)) {
    LOG_DEBUG("HandleAutoEquipItem: equip rejected (account={} guid={})", _accountId,
              _playerGuid);
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  auto refreshed = _charService->GetCharacterByGuid(_playerGuid);
  if (!refreshed) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  UpdateData update(_mapId);
  update.AddValuesUpdate(
      _playerGuid,
      WorldSessionObjectUpdate::BuildPlayerBag0InventoryValues(*refreshed));
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  SendPacket(pkt);
  }

void WorldSession::HandleAutoEquipItemSlot(WorldPacket &packet) {
  if (_playerGuid == 0)
    return;
  int32_t constexpr kEquipErrCantEquipOther = 15;

  auto ch = _charService->GetCharacterByGuid(_playerGuid);
  if (!ch) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  struct ParsedAutoEquipItemSlotPayload {
    bool ok = false;
    bool usedInvPrefix = false;
  int invItems = -1;
    uint64_t itemGuid = 0;
    uint8_t dstSlot = 0;
    size_t consumed = 0;
  };

  auto tryParse = [&](size_t startPos, bool withInvPrefix) {
    ParsedAutoEquipItemSlotPayload out;
    packet.SetReadPos(startPos);
    if (withInvPrefix) {
      out.invItems = ReadItemInvUpdatePrefix(packet);
      if (out.invItems < 0)
      return out;
      out.usedInvPrefix = true;
  }
    if (packet.Size() - packet.GetReadPos() < 2)
      return out;
    out.itemGuid = packet.ReadPackedGuid();
  if (packet.Size() - packet.GetReadPos() < 1)
      return out;
    out.dstSlot = packet.Read<uint8_t>();
    out.consumed = packet.GetReadPos() - startPos;
    out.ok = true;
      return out;
  };

  size_t const payloadStart = packet.GetReadPos();
  ParsedAutoEquipItemSlotPayload parsedWithPrefix =
      tryParse(payloadStart, true);
  ParsedAutoEquipItemSlotPayload parsedNoPrefix =
      tryParse(payloadStart, false);

  auto payloadMatchesBag0 = [&](ParsedAutoEquipItemSlotPayload const &p) {
    if (!p.ok)
      return false;
    uint32_t const low = static_cast<uint32_t>(p.itemGuid & 0xFFFFFFFFu);
    return FindBag0SlotByItemLowGuid(*ch, low).has_value();
  };

  ParsedAutoEquipItemSlotPayload parsed;
  if (payloadMatchesBag0(parsedWithPrefix)) {
    parsed = parsedWithPrefix;
  } else if (payloadMatchesBag0(parsedNoPrefix)) {
    parsed = parsedNoPrefix;
  } else if (parsedNoPrefix.ok) {
    parsed = parsedNoPrefix;
  } else if (parsedWithPrefix.ok) {
    parsed = parsedWithPrefix;
  } else {
    LOG_WARN("HandleAutoEquipItemSlot: unsupported payload size={}",
             packet.Size() - payloadStart);
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  uint64_t const itemGuid = parsed.itemGuid;
  uint8_t const dstSlot = parsed.dstSlot;
  LOG_DEBUG(
      "HandleAutoEquipItemSlot: account={} guid={} itemLow={} dstSlot={} "
      "usedInvPrefix={} invItems={} consumed={}",
      _accountId, _playerGuid,
      static_cast<uint32_t>(itemGuid & 0xFFFFFFFFu), dstSlot,
      parsed.usedInvPrefix, parsed.invItems, parsed.consumed);

  if (dstSlot >= EQUIPMENT_SLOT_END) {
    LOG_DEBUG("HandleAutoEquipItemSlot: invalid dstSlot={}", dstSlot);
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  uint32_t const itemLowGuid = static_cast<uint32_t>(itemGuid & 0xFFFFFFFFu);
  auto srcSlotOpt = FindBag0SlotByItemLowGuid(*ch, itemLowGuid);
  if (!srcSlotOpt) {
    LOG_DEBUG("HandleAutoEquipItemSlot: itemLowGuid={} not found in bag0",
              itemLowGuid);
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  if (!_charService->SwapBag0Slots(_playerGuid, *srcSlotOpt, dstSlot)) {
    LOG_DEBUG("HandleAutoEquipItemSlot: swap rejected src={} dst={} guid={}",
              *srcSlotOpt, dstSlot, _playerGuid);
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  auto refreshed = _charService->GetCharacterByGuid(_playerGuid);
  if (!refreshed) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  UpdateData update(_mapId);
  update.AddValuesUpdate(
      _playerGuid,
      WorldSessionObjectUpdate::BuildPlayerBag0InventoryValues(*refreshed));
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  SendPacket(pkt);
  }

void WorldSession::HandleDestroyItem(WorldPacket &packet) {
  if (_playerGuid == 0)
    return;
  int32_t constexpr kEquipErrCantEquipOther = 15;
  // `WorldPackets::Item::DestroyItem::Read()` — ContainerId, SlotNum, Count.
  if (packet.Size() - packet.GetReadPos() <
      sizeof(uint8_t) * 2 + sizeof(uint32_t)) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  uint8_t const containerId = packet.Read<uint8_t>();
  uint8_t const slotNum = packet.Read<uint8_t>();
  uint32_t const count = packet.Read<uint32_t>();
  uint8_t const normalizedSlot = NormalizeBag0ItemSlot(containerId, slotNum);

  LOG_DEBUG(
      "HandleDestroyItem: account={} guid={} container={} slot={} normalized={} count={}",
      _accountId, _playerGuid, containerId, slotNum, normalizedSlot, count);

  if (containerId != 0 && containerId != CLIENT_INVENTORY_SLOT_DEFAULT_BACKPACK) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  if (normalizedSlot < INVENTORY_SLOT_ITEM_START ||
      normalizedSlot >= INVENTORY_SLOT_ITEM_END) {
    // Equipment / bags not supported yet (no unequip rules).
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  if (!_charService->DestroyBag0BackpackItem(
          _accountId, static_cast<uint32_t>(_playerGuid), containerId, slotNum,
          count)) {
    SendInventoryChangeFailure(*this, kEquipErrItemNotFound);
    return;
  }

  auto refreshed = _charService->GetCharacterByGuid(_playerGuid);
  if (!refreshed) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  UpdateData update(_mapId);
  update.AddValuesUpdate(
      _playerGuid,
      WorldSessionObjectUpdate::BuildPlayerBag0InventoryValues(*refreshed));
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  SendPacket(pkt);
  }

void WorldSession::HandleUseItem(WorldPacket &packet) {
  if (_playerGuid == 0)
    return;
  int32_t constexpr kEquipErrCantEquipOther = 15;
  // WorldPackets::Spells::UseItem::Read() — first fields used for inventory use.
  if (packet.Size() - packet.GetReadPos() < sizeof(uint8_t) * 3 + sizeof(int32_t)) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  uint8_t const packSlot = packet.Read<uint8_t>();
  uint8_t const slot = packet.Read<uint8_t>();
  uint8_t const normalizedSlot = NormalizeBag0ItemSlot(packSlot, slot);
  (void)packet.Read<uint8_t>(); // Cast.CastID
  (void)packet.Read<int32_t>(); // Cast.SpellID (non-zero for many equippables; uses spellmgr)
  packet.SetReadPos(packet.Size());
  LOG_DEBUG("HandleUseItem: account={} guid={} packSlot={} slot={} normalized={}",
            _accountId, _playerGuid, packSlot, slot, normalizedSlot);

  if (!_charService->AutoEquipFromBag0(_accountId, static_cast<uint32_t>(_playerGuid),
                                        packSlot, normalizedSlot)) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  auto refreshed = _charService->GetCharacterByGuid(_playerGuid);
  if (!refreshed) {
    SendInventoryChangeFailure(*this, kEquipErrCantEquipOther);
    return;
  }

  UpdateData update(_mapId);
  update.AddValuesUpdate(
      _playerGuid,
      WorldSessionObjectUpdate::BuildPlayerBag0InventoryValues(*refreshed));
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  SendPacket(pkt);
  }

void WorldSession::SendBuyFailed(uint64_t vendorGuid, uint32_t itemEntry,
                                 uint8_t reason) {
  // `WorldSession::SendBuyError` (4.3.4, param == 0 form).
  WorldPacket pkt(SMSG_BUY_FAILED, 8 + 4 + 1);
  pkt.Append<uint64_t>(vendorGuid);
  pkt.Append<uint32_t>(itemEntry);
  pkt.Append<uint8_t>(reason);
  SendPacket(pkt);
}

void WorldSession::PublishBag0AndCoinageAfterTransaction() {
  auto refreshed = _charService->GetCharacterByGuid(_playerGuid);
  if (!refreshed)
    return;
  _moneyCopper = refreshed->GetMoney();

  UpdateData update(_mapId);
  update.AddValuesUpdate(
      _playerGuid,
      WorldSessionObjectUpdate::BuildPlayerBag0InventoryValues(*refreshed));
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  SendPacket(pkt);

  PublishSelfCoinageUpdate();
}

void WorldSession::HandleSellItem(WorldPacket &packet) {
  if (_playerGuid == 0)
    return;
  // CMSG_SELL_ITEM (4.3.4): uint64 vendorGuid, uint64 itemGuid, uint32 count.
  if (packet.Size() - packet.GetReadPos() < sizeof(uint64) * 2 + sizeof(uint32))
    return;
  uint64 const vendorGuid = packet.Read<uint64>();
  uint64 const itemGuid = packet.Read<uint64>();
  uint32 const count = packet.Read<uint32>();

  auto ch = _charService->GetCharacterByGuid(_playerGuid);
  if (!ch)
    return;

  uint32 const itemLowGuid = static_cast<uint32>(itemGuid & 0xFFFFFFFFu);
  auto ref = FindBackpackItemByLowGuid(*ch, itemLowGuid);
  if (!ref || ref->entry == 0) {
    SendSellResult(*this, vendorGuid, itemGuid, kSellErrCantFindItem);
    return;
  }

  // count == 0 means "sell the whole stack"; clamp oversized requests.
  uint32 const sellCount =
      (count == 0 || count > ref->stack) ? ref->stack : count;

  auto tpl = _itemTemplateStore ? _itemTemplateStore->lookup(ref->entry)
                                : std::nullopt;
  if (!tpl || tpl->sellPrice == 0) {
    // SellPrice 0 -> item has no merchant value and cannot be sold.
    SendSellResult(*this, vendorGuid, itemGuid, kSellErrCantSellItem);
    return;
  }

  uint64 const refund = static_cast<uint64>(tpl->sellPrice) * sellCount;

  LOG_DEBUG("HandleSellItem: guid={} vendor={:#x} itemLow={} entry={} count={} "
            "refund={}",
            _playerGuid, vendorGuid, itemLowGuid, ref->entry, sellCount, refund);

  if (!_charService->DestroyBag0BackpackItem(_accountId,
                                             static_cast<uint32_t>(_playerGuid),
                                             0, ref->packIndex, sellCount)) {
    SendSellResult(*this, vendorGuid, itemGuid, kSellErrCantFindItem);
    return;
  }

  _charService->AddCharacterMoneyDelta(_accountId,
                                       static_cast<uint32_t>(_playerGuid),
                                       static_cast<int64>(refund));

  // Track for buyback (front = most recent, capped at kMaxBuybackSlots).
  BuybackEntry entry;
  entry.itemEntry = ref->entry;
  entry.count = sellCount;
  entry.totalRefund =
      static_cast<uint32_t>(std::min<uint64>(refund, 0xFFFFFFFFu));
  _buybackItems.push_front(entry);
  while (_buybackItems.size() > kMaxBuybackSlots)
    _buybackItems.pop_back();

  PublishBag0AndCoinageAfterTransaction();
}

void WorldSession::HandleBuyItem(WorldPacket &packet) {
  if (_playerGuid == 0)
    return;
  // CMSG_BUY_ITEM (4.3.4): uint64 vendorGuid, uint8 itemType, uint32 item,
  //   uint32 vendorSlot, uint32 count, uint64 bagGuid, uint8 bagSlot.
  if (packet.Size() - packet.GetReadPos() <
      sizeof(uint64) + sizeof(uint8) + sizeof(uint32) * 3 + sizeof(uint64) +
          sizeof(uint8))
    return;
  uint64 const vendorGuid = packet.Read<uint64>();
  uint8 const itemType = packet.Read<uint8>();
  uint32 const item = packet.Read<uint32>();
  uint32 const vendorSlot = packet.Read<uint32>();
  uint32 count = packet.Read<uint32>();
  (void)packet.Read<uint64>(); // bagGuid (auto-store into backpack)
  (void)packet.Read<uint8>();  // bagSlot
  (void)vendorSlot;

  if (itemType != 1) {
    // 2 = currency; not supported.
    SendBuyFailed(vendorGuid, item, kBuyErrCantFindItem);
    return;
  }
  if (count == 0)
    count = 1;

  // The vendor must actually offer this item.
  bool offered = false;
  if (_vendorRepo) {
    if (auto vendorEntry = TryResolveCreatureTemplateEntry(vendorGuid)) {
      for (VendorItemEntry const &row : _vendorRepo->GetVendorItems(*vendorEntry)) {
        if (row.type == 1 && row.item == item) {
          offered = true;
          break;
        }
      }
    }
  }
  if (!offered) {
    SendBuyFailed(vendorGuid, item, kBuyErrCantFindItem);
    return;
  }

  auto tpl = _itemTemplateStore ? _itemTemplateStore->lookup(item)
                                : std::nullopt;
  if (!tpl) {
    SendBuyFailed(vendorGuid, item, kBuyErrCantFindItem);
    return;
  }

  // Cost is per requested unit. Items with BuyCount > 1 (e.g. stacked ammo)
  // would need the stack divided into price; vendors here are BuyCount 1.
  uint64 const totalCost = static_cast<uint64>(tpl->buyPrice) * count;

  auto ch = _charService->GetCharacterByGuid(_playerGuid);
  if (!ch)
    return;
  if (static_cast<uint64>(ch->GetMoney()) < totalCost) {
    SendBuyFailed(vendorGuid, item, kBuyErrNotEnoughMoney);
    return;
  }

  LOG_DEBUG("HandleBuyItem: guid={} vendor={:#x} item={} count={} cost={}",
            _playerGuid, vendorGuid, item, count, totalCost);

  if (!_charService->GrantItemToBag0(static_cast<uint32_t>(_playerGuid), item,
                                     count)) {
    SendBuyFailed(vendorGuid, item, kBuyErrCantCarryMore);
    return;
  }

  _charService->AddCharacterMoneyDelta(_accountId,
                                       static_cast<uint32_t>(_playerGuid),
                                       -static_cast<int64>(totalCost));
  PublishBag0AndCoinageAfterTransaction();
}

void WorldSession::HandleBuybackItem(WorldPacket &packet) {
  if (_playerGuid == 0)
    return;
  // CMSG_BUY_BACK_ITEM (4.3.4): uint64 vendorGuid, uint32 buybackSlot.
  if (packet.Size() - packet.GetReadPos() < sizeof(uint64) + sizeof(uint32))
    return;
  uint64 const vendorGuid = packet.Read<uint64>();
  uint32 const slot = packet.Read<uint32>();

  size_t const index = slot >= kBuybackSlotStart
                           ? static_cast<size_t>(slot - kBuybackSlotStart)
                           : static_cast<size_t>(slot);
  if (index >= _buybackItems.size()) {
    SendBuyFailed(vendorGuid, 0, kBuyErrCantFindItem);
    return;
  }

  BuybackEntry const entry = _buybackItems[index];

  auto ch = _charService->GetCharacterByGuid(_playerGuid);
  if (!ch)
    return;
  if (static_cast<uint64>(ch->GetMoney()) < entry.totalRefund) {
    SendBuyFailed(vendorGuid, entry.itemEntry, kBuyErrNotEnoughMoney);
    return;
  }

  if (!_charService->GrantItemToBag0(static_cast<uint32_t>(_playerGuid),
                                     entry.itemEntry, entry.count)) {
    SendBuyFailed(vendorGuid, entry.itemEntry, kBuyErrCantCarryMore);
    return;
  }

  _charService->AddCharacterMoneyDelta(
      _accountId, static_cast<uint32_t>(_playerGuid),
      -static_cast<int64>(entry.totalRefund));
  _buybackItems.erase(_buybackItems.begin() + static_cast<long>(index));

  PublishBag0AndCoinageAfterTransaction();
}

} // namespace Firelands

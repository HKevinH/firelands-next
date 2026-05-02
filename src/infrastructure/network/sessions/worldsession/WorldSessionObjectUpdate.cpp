#include <infrastructure/network/sessions/worldsession/WorldSessionObjectUpdate.h>
#include <shared/network/UpdateData.h>
#include <shared/network/UpdateFields.h>
#include <shared/game/ChatLanguages.h>
#include <shared/game/EquipmentCache.h>
#include <shared/game/InventorySlots.h>
#include <shared/game/WowGuid.h>

#include <cstring>

namespace Firelands {
namespace WorldSessionObjectUpdate {

namespace {

void SetPackedShort(std::map<uint16, uint32> &fields, uint16 field, uint8 slot,
                    uint16 value) {
  uint32 &packed = fields[static_cast<uint16>(field + (slot / 2))];
  if ((slot % 2) == 0)
    packed = (packed & 0xFFFF0000u) | value;
  else
    packed = (packed & 0x0000FFFFu) | (static_cast<uint32>(value) << 16);
}

void AddLanguageSkillFields(std::map<uint16, uint32> &fields, uint8 race) {
  std::vector<uint32> skills;
  AppendRacialLanguageSkills(race, skills);
  constexpr uint16 kRank = 300;
  for (size_t i = 0; i < skills.size() && i < 64; ++i) {
    uint8 const slot = static_cast<uint8>(i);
    uint16 const skill = static_cast<uint16>(skills[i]);
    SetPackedShort(fields, PLAYER_SKILL_LINEID_0, slot, skill);
    SetPackedShort(fields, PLAYER_SKILL_STEP_0, slot, 0);
    SetPackedShort(fields, PLAYER_SKILL_RANK_0, slot, kRank);
    SetPackedShort(fields, PLAYER_SKILL_MAX_RANK_0, slot, kRank);
    SetPackedShort(fields, PLAYER_SKILL_MODIFIER_0, slot, 0);
    SetPackedShort(fields, PLAYER_SKILL_TALENT_0, slot, 0);
  }
}

} // namespace

std::vector<uint32> BuildDefaultKnownSpells(uint8 classId) {
  switch (classId) {
  case 1: // Warrior
    return {2457, 71, 78, 100, 6673, 772, 3127, 34428};
  case 2: // Paladin
    return {465, 635, 20154, 20271, 19740, 498, 633, 82242};
  case 3: // Hunter
    return {75, 13165, 1978, 3044, 56641, 781, 1130, 2973};
  case 4: // Rogue
    return {1784, 2098, 53, 1752, 921, 1766, 1776, 82245};
  case 5: // Priest
    return {585, 589, 2061, 17, 139, 2050, 8092};
  case 6: // Death Knight
    return {48263, 45524, 49998, 47528, 48721, 45529, 48792};
  case 7: // Shaman
    return {331, 8042, 8017, 8050, 324, 51730, 5185, 52127};
  case 8: // Mage
    return {116, 133, 2136, 1459, 130, 1953, 118};
  case 9: // Warlock
    return {172, 348, 687, 1454, 5782, 980, 603};
  case 11: // Druid
    return {8921, 5185, 774, 768, 1126, 339, 467};
  default:
    return {6673, 78, 2457, 3127};
  }
}

std::map<uint16, uint32> BuildItemCreateFields(uint64 itemObjectGuid,
                                               uint64 ownerGuid, uint32 itemEntry,
                                               uint32 stackCount) {
  std::map<uint16, uint32> fields;
  for (uint16_t i = 0; i < ITEM_END; ++i)
    fields[i] = 0;

  uint32 igLo = 0;
  uint32 igHi = 0;
  uint32 owLo = 0;
  uint32 owHi = 0;
  WriteGuidToTwoUint32(itemObjectGuid, igLo, igHi);
  WriteGuidToTwoUint32(ownerGuid, owLo, owHi);

  fields[OBJECT_FIELD_GUID] = igLo;
  fields[OBJECT_FIELD_GUID + 1] = igHi;
  fields[OBJECT_FIELD_DATA] = 0;
  fields[OBJECT_FIELD_DATA + 1] = 0;
  fields[OBJECT_FIELD_TYPE] = kTypeMaskItem;
  fields[OBJECT_FIELD_ENTRY] = itemEntry;
  fields[OBJECT_FIELD_SCALE_X] = 0x3F800000;
  fields[OBJECT_FIELD_PADDING] = 0;

  fields[ITEM_FIELD_OWNER] = owLo;
  fields[ITEM_FIELD_OWNER + 1] = owHi;
  fields[ITEM_FIELD_CONTAINED] = owLo;
  fields[ITEM_FIELD_CONTAINED + 1] = owHi;
  fields[ITEM_FIELD_STACK_COUNT] = stackCount;
  return fields;
}

std::map<uint16, uint32> BuildPlayerBag0InventoryValues(Character const &character) {
  std::map<uint16, uint32> fields;
  for (size_t slot = 0; slot < kEquipmentSlotCount; ++slot) {
    uint32 const entry = character.GetVisibleItemEntry(slot);
    uint32 const itemGuidLow = character.GetVisibleItemGuidLow(slot);
    uint16 const base = static_cast<uint16>(
        PLAYER_VISIBLE_ITEM_1_ENTRYID + static_cast<uint16>(slot * 2));
    fields[base] = entry;
    fields[static_cast<uint16>(base + 1)] = 0;

    uint64 const itemOg = MakeItemObjectGuid(itemGuidLow);
    uint32 ilo = 0;
    uint32 ihi = 0;
    WriteGuidToTwoUint32(itemOg, ilo, ihi);
    uint16 const invBase = static_cast<uint16>(
        PLAYER_FIELD_INV_SLOT_HEAD + static_cast<uint16>(slot * 2));
    fields[invBase] = ilo;
    fields[static_cast<uint16>(invBase + 1)] = ihi;
  }
  for (size_t packIndex = 0; packIndex < kPackSlotCount; ++packIndex) {
    uint32 const itemGuidLow = character.GetPackItemGuidLow(packIndex);
    uint64 const itemOg = MakeItemObjectGuid(itemGuidLow);
    uint32 ilo = 0;
    uint32 ihi = 0;
    WriteGuidToTwoUint32(itemOg, ilo, ihi);
    uint16 const packBase = static_cast<uint16>(
        PLAYER_FIELD_PACK_SLOT_1 + static_cast<uint16>(packIndex * 2));
    fields[packBase] = ilo;
    fields[static_cast<uint16>(packBase + 1)] = ihi;
  }
  return fields;
}

std::map<uint16, uint32> BuildPlayerUpdateFields(uint64 guid,
                                                 Character const &character) {
  std::map<uint16, uint32> fields;
  fields[OBJECT_FIELD_GUID] = (uint32)(guid & 0xFFFFFFFF);
  fields[OBJECT_FIELD_GUID + 1] = (uint32)(guid >> 32);
  fields[OBJECT_FIELD_TYPE] =
      (1 << TYPEID_OBJECT) | (1 << TYPEID_UNIT) | (1 << TYPEID_PLAYER);
  fields[OBJECT_FIELD_SCALE_X] = 0x3F800000;

  uint8 bytes0[4] = {character.GetRace(), character.GetClass(),
                     character.GetGender(), 0};
  std::memcpy(&fields[UNIT_FIELD_BYTES_0], bytes0, 4);

  fields[UNIT_FIELD_HEALTH] = character.GetHealth();
  fields[UNIT_FIELD_MAXHEALTH] = character.GetMaxHealth();
  fields[UNIT_FIELD_POWER1] = 0;
  fields[UNIT_FIELD_MAXPOWER1] = 0;
  fields[UNIT_FIELD_LEVEL] = character.GetLevel();
  fields[UNIT_FIELD_FACTIONTEMPLATE] = character.GetFactionTemplate();
  fields[UNIT_FIELD_DISPLAYID] = character.GetDisplayId();
  fields[UNIT_FIELD_NATIVEDISPLAYID] = character.GetDisplayId();
  fields[UNIT_FIELD_BYTES_2] = 0;

  AddLanguageSkillFields(fields, character.GetRace());

  {
    uint64 const coin = character.GetMoney();
    fields[PLAYER_FIELD_COINAGE] = static_cast<uint32>(coin & 0xFFFFFFFFu);
    fields[static_cast<uint16>(PLAYER_FIELD_COINAGE + 1)] =
        static_cast<uint32>((coin >> 32) & 0xFFFFFFFFu);
  }

  for (size_t slot = 0; slot < kEquipmentSlotCount; ++slot) {
    uint32 const entry = character.GetVisibleItemEntry(slot);
    uint32 const itemGuidLow = character.GetVisibleItemGuidLow(slot);
    uint16 const base = static_cast<uint16>(
        PLAYER_VISIBLE_ITEM_1_ENTRYID + static_cast<uint16>(slot * 2));
    fields[base] = entry;
    fields[static_cast<uint16>(base + 1)] = 0;

    uint64 const itemOg = MakeItemObjectGuid(itemGuidLow);
    uint32 ilo = 0;
    uint32 ihi = 0;
    WriteGuidToTwoUint32(itemOg, ilo, ihi);
    uint16 const invBase = static_cast<uint16>(
        PLAYER_FIELD_INV_SLOT_HEAD + static_cast<uint16>(slot * 2));
    fields[invBase] = ilo;
    fields[static_cast<uint16>(invBase + 1)] = ihi;
  }
  for (size_t packIndex = 0; packIndex < kPackSlotCount; ++packIndex) {
    uint32 const itemGuidLow = character.GetPackItemGuidLow(packIndex);
    uint64 const itemOg = MakeItemObjectGuid(itemGuidLow);
    uint32 ilo = 0;
    uint32 ihi = 0;
    WriteGuidToTwoUint32(itemOg, ilo, ihi);
    uint16 const packBase = static_cast<uint16>(
        PLAYER_FIELD_PACK_SLOT_1 + static_cast<uint16>(packIndex * 2));
    fields[packBase] = ilo;
    fields[static_cast<uint16>(packBase + 1)] = ihi;
  }

  return fields;
}

void AppendPlayerGuidLookupData(WorldPacket &dst, Character const &ch,
                                std::string const &realmName) {
  dst.WriteString(ch.GetName());
  dst.WriteString(realmName);
  dst.Append<uint8>(ch.GetRace());
  dst.Append<uint8>(ch.GetGender());
  dst.Append<uint8>(ch.GetClass());
  dst.Append<uint8>(0); // DeclinedNames.has_value() == false
}

uint64 ReadClientTargetGuid(WorldPacket &packet) {
  const size_t rem = packet.Size() - packet.GetReadPos();
  if (rem >= sizeof(uint64)) {
    return packet.Read<uint64>();
  }
  if (rem > 0) {
    return packet.ReadPackedGuid();
  }
  return 0;
}

void SendPlayerCreateToNotifier(std::shared_ptr<IMapNotifier> target, uint32 mapId,
                                uint64 objectGuid, Character const &character,
                                MovementInfo const &move,
                                PlayerGmAppearanceForUpdates const &gmAppearance) {
  if (!target)
    return;
  auto fields = BuildPlayerUpdateFields(objectGuid, character);
  MergeGmAppearanceIntoPlayerFields(fields, gmAppearance);
  UpdateData update(mapId);
  update.AddCreateObject(objectGuid, TYPEID_PLAYER, move, fields);
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  target->SendPacket(pkt);
}

} // namespace WorldSessionObjectUpdate
} // namespace Firelands

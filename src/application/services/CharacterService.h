#pragma once

#include <domain/repositories/ICharacterRepository.h>
#include <memory>
#include <shared/Common.h>
#include <vector>

namespace Firelands {

class CharacterService {
public:
  explicit CharacterService(std::shared_ptr<ICharacterRepository> repository)
      : m_repository(std::move(repository)) {}

  std::vector<std::shared_ptr<Character>>
  GetCharactersForAccount(uint32 accountId) {
    return m_repository->GetCharactersByAccount(accountId);
  }

  bool CreateCharacter(uint32 accountId, std::string name, uint8 race,
                       uint8 klass, uint8 gender, uint8 skin, uint8 face,
                       uint8 hairStyle, uint8 hairColor, uint8 facialHair) {

    if (!m_repository->IsNameAvailable(name)) {
      return false;
    }

    // Default spawn location.
    // Reference (FirelandsCore) uses `playercreateinfo` (race/class → map/zone/x/y/z/o).
    // We keep a minimal hardcoded mapping for now.
    uint16 mapId = 0;
    uint16 zoneId = 12;
    float x = -8949.95f, y = -132.49f, z = 83.53f, o = 0.0f; // Northshire Abbey

    switch (race) {
    case 1: // Human
      mapId = 0; zoneId = 12; x = -8949.95f; y = -132.49f; z = 83.53f; o = 0.0f;
      break;
    case 2: // Orc (Valley of Trials)
      mapId = 1; zoneId = 14; x = -618.518f; y = -4251.67f; z = 38.718f; o = 0.0f;
      break;
    case 3: // Dwarf (Coldridge Valley)
      mapId = 0; zoneId = 1; x = -6240.32f; y = 331.033f; z = 382.758f; o = 0.0f;
      break;
    case 4: // Night Elf (Shadowglen)
      mapId = 1; zoneId = 141; x = 10311.3f; y = 832.463f; z = 1326.41f; o = 0.0f;
      break;
    case 5: // Undead (Deathknell)
      mapId = 0; zoneId = 154; x = 1676.71f; y = 1678.31f; z = 121.67f; o = 0.0f;
      break;
    case 6: // Tauren (Camp Narache)
      mapId = 1; zoneId = 215; x = -2917.58f; y = -257.98f; z = 52.9968f; o = 0.0f;
      break;
    case 7: // Gnome (New Tinkertown)
      mapId = 0; zoneId = 1; x = -6240.32f; y = 331.033f; z = 382.758f; o = 0.0f; // shares dwarf start area
      break;
    case 8: // Troll (Echo Isles / Durotar)
      mapId = 1; zoneId = 14; x = -618.518f; y = -4251.67f; z = 38.718f; o = 0.0f; // shares orc start area
      break;
    case 10: // Blood Elf (Sunstrider Isle)
      mapId = 530; zoneId = 3430; x = 10349.6f; y = -6357.29f; z = 33.4026f; o = 0.0f;
      break;
    case 11: // Draenei (Ammen Vale)
      mapId = 530; zoneId = 3526; x = -3961.64f; y = -13931.2f; z = 100.615f; o = 0.0f;
      break;
    case 9: // Goblin (Kezan) - fallback to Orgrimmar start for now
    case 22: // Worgen (Gilneas) - fallback to Human start for now
    default:
      break;
    }

    Character newChar(0, accountId, name, race, klass, gender, skin, face,
                      hairStyle, hairColor, facialHair,
                      1, zoneId, mapId, x, y, z, o,
                      0, 0, 0, true);

    return m_repository->CreateCharacter(newChar);
  }

  bool DeleteCharacter(uint32_t guid, uint32_t accountId) {
    return m_repository->DeleteCharacter(guid, accountId);
  }

  std::optional<Character> GetCharacterByGuid(uint64_t guid) {
    return m_repository->GetCharacterByGuid(guid);
  }

private:
  std::shared_ptr<ICharacterRepository> m_repository;
};

} // namespace Firelands

#pragma once

#include <cstdint>

namespace Firelands {

/// Blizzard `ChrClasses.dbc` ids (build 15595). Index 10 is unused in Cataclysm.
enum class PlayerClass : uint8_t {
  None = 0,
  Warrior = 1,
  Paladin = 2,
  Hunter = 3,
  Rogue = 4,
  Priest = 5,
  DeathKnight = 6,
  Shaman = 7,
  Mage = 8,
  Warlock = 9,
  Unused = 10,
  Druid = 11,
};

inline constexpr uint8_t ToClassId(PlayerClass klass) noexcept {
  return static_cast<uint8_t>(klass);
}

inline PlayerClass ToPlayerClass(uint8_t classId) noexcept {
  return static_cast<PlayerClass>(classId);
}

inline bool IsValidPlayerClass(uint8_t classId) noexcept {
  switch (ToPlayerClass(classId)) {
  case PlayerClass::Warrior:
  case PlayerClass::Paladin:
  case PlayerClass::Hunter:
  case PlayerClass::Rogue:
  case PlayerClass::Priest:
  case PlayerClass::DeathKnight:
  case PlayerClass::Shaman:
  case PlayerClass::Mage:
  case PlayerClass::Warlock:
  case PlayerClass::Druid:
    return true;
  default:
    return false;
  }
}

inline bool IsValidPlayerClass(PlayerClass klass) noexcept {
  return IsValidPlayerClass(ToClassId(klass));
}

} // namespace Firelands

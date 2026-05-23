#pragma once

#include <shared/game/PlayerClass.h>
#include <shared/game/PlayerPowerType.h>

#include <cstdint>

namespace Firelands {

/// `UnitBytes0` power type (client 4.3.4).
enum class PlayerPowerType : uint8_t {
  Mana = 0,
  Rage = 1,
  Focus = 2,
  Energy = 3,
  RunicPower = 6,
};

inline PlayerPowerType GetDefaultPlayerPowerType(PlayerClass klass) {
  switch (klass) {
  case PlayerClass::Warrior:
    return PlayerPowerType::Rage;
  case PlayerClass::Rogue:
    return PlayerPowerType::Energy;
  case PlayerClass::Hunter:
    return PlayerPowerType::Focus;
  case PlayerClass::DeathKnight:
    return PlayerPowerType::RunicPower;
  default:
    return PlayerPowerType::Mana;
  }
}

inline PlayerPowerType GetDefaultPlayerPowerType(uint8_t klass) {
  return GetDefaultPlayerPowerType(ToPlayerClass(klass));
}

inline uint32_t DefaultMaxPower1(PlayerPowerType pt) {
  switch (pt) {
  case PlayerPowerType::Rage:
  case PlayerPowerType::RunicPower:
    return 1000u;
  case PlayerPowerType::Energy:
    return 100u;
  case PlayerPowerType::Focus:
    return 100u;
  case PlayerPowerType::Mana:
  default:
    return 0u;
  }
}

} // namespace Firelands

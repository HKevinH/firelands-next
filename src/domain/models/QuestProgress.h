#pragma once

#include <cstdint>

namespace Firelands {

/// Mirrors TrinityCore `QuestStatus` values stored in `character_queststatus.status`.
enum class QuestStatus : uint8_t {
  None = 0,
  Complete = 1,
  Incomplete = 3,
  Failed = 5,
};

} // namespace Firelands

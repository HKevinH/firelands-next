#pragma once

#include <shared/Common.h>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Firelands {

/// Condition types used by `conditions` rows for `CONDITION_SOURCE_TYPE_PHASE` (26).
enum class PhaseConditionType : uint8_t {
  None = 0,
  Aura = 1,
  QuestRewarded = 8,
  QuestTaken = 9,
  QuestComplete = 28,
};

/// One row from world `conditions` attached to a `phase_area` entry.
struct PhaseCondition {
  uint32 elseGroup = 0;
  PhaseConditionType type = PhaseConditionType::None;
  uint32 value1 = 0;
  uint32 value2 = 0;
  uint32 value3 = 0;
  bool negative = false;
};

using PhaseConditionList = std::vector<PhaseCondition>;

/// Key from `conditions` source group/entry for type 26 (`PhaseId`, `AreaId`).
struct PhaseConditionKey {
  uint16 phaseId = 0;
  uint32 areaId = 0;

  bool operator==(PhaseConditionKey const &other) const {
    return phaseId == other.phaseId && areaId == other.areaId;
  }
};

struct PhaseConditionKeyHash {
  size_t operator()(PhaseConditionKey const &key) const noexcept {
    return (static_cast<size_t>(key.phaseId) << 32) ^ static_cast<size_t>(key.areaId);
  }
};

template <typename Value>
using PhaseConditionMap = std::unordered_map<PhaseConditionKey, Value, PhaseConditionKeyHash>;

} // namespace Firelands

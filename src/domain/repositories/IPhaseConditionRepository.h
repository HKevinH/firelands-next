#pragma once

#include <domain/models/PhaseCondition.h>

#include <cstdint>

namespace Firelands {

/// Loads `conditions` rows for `CONDITION_SOURCE_TYPE_PHASE` (26).
class IPhaseConditionRepository {
public:
  virtual ~IPhaseConditionRepository() = default;

  virtual PhaseConditionMap<PhaseConditionList> LoadPhaseConditions() const = 0;
};

} // namespace Firelands

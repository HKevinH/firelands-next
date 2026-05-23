#pragma once

#include <domain/models/QuestProgress.h>
#include <shared/Common.h>

#include <cstdint>

namespace Firelands {

/// Player quest/aura state for phase condition evaluation (TrinityCore `ConditionSourceInfo`).
class IPlayerQuestProgress {
public:
  virtual ~IPlayerQuestProgress() = default;

  virtual QuestStatus GetQuestStatus(uint32 questId) const = 0;
  virtual bool IsQuestRewarded(uint32 questId) const = 0;
  virtual bool HasAuraSpell(uint32 spellId) const = 0;
};

} // namespace Firelands

#pragma once

#include <domain/models/QuestGossip.h>
#include <cstdint>
#include <vector>

namespace Firelands {

/// Quests a creature can offer in gossip (`creature_queststarter` + `quest_template`).
class IQuestGossipRepository {
public:
  virtual ~IQuestGossipRepository() = default;

  virtual std::vector<QuestGossipSummary>
  GetStarterQuestsForCreature(uint32_t creatureEntry) const = 0;
};

} // namespace Firelands

#include "MySqlQuestGossipRepository.h"
#include <shared/Logger.h>

namespace Firelands {

MySqlQuestGossipRepository::MySqlQuestGossipRepository(
    std::shared_ptr<sql::Connection> connection)
    : _connection(std::move(connection)) {}

std::vector<QuestGossipSummary>
MySqlQuestGossipRepository::GetStarterQuestsForCreature(
    uint32_t creatureEntry) const {
  std::vector<QuestGossipSummary> result;
  if (creatureEntry == 0)
    return result;

  try {
    std::unique_ptr<sql::PreparedStatement> stmt(_connection->prepareStatement(
        "SELECT q.`ID`, q.`LogTitle`, q.`QuestLevel`, q.`Flags` "
        "FROM `creature_queststarter` cqs "
        "INNER JOIN `quest_template` q ON q.`ID` = cqs.`quest` "
        "WHERE cqs.`id` = ? "
        "ORDER BY q.`ID`"));
    stmt->setUInt(1, creatureEntry);
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
    while (rs->next()) {
      QuestGossipSummary summary;
      summary.questId = rs->getUInt("ID");
      summary.title = rs->getString("LogTitle");
      summary.questLevel = rs->getInt("QuestLevel");
      summary.flags = rs->getUInt("Flags");
      summary.isAutoComplete = QuestHasAutoCompleteFlag(summary.flags);
      result.push_back(std::move(summary));
    }
  } catch (sql::SQLException const &e) {
    LOG_ERROR(
        "MySqlQuestGossipRepository::GetStarterQuestsForCreature entry={}: {}",
        creatureEntry, e.what());
  }
  return result;
}

} // namespace Firelands

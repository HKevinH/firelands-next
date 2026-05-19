#include "MySqlGossipRepository.h"
#include <shared/Logger.h>

namespace Firelands {

namespace {

std::string SafeSqlString(std::optional<sql::SQLString> const &val) {
  if (!val.has_value())
    return "";
  return std::string(val->c_str());
}

} // namespace

MySqlGossipRepository::MySqlGossipRepository(std::shared_ptr<sql::Connection> connection)
    : _connection(std::move(connection)) {}

std::optional<uint32_t> MySqlGossipRepository::GetMenuTextId(uint32_t menuId) const {
  try {
    std::unique_ptr<sql::PreparedStatement> stmt(
        _connection->prepareStatement(
            "SELECT `TextID` FROM `gossip_menu` WHERE `MenuID` = ? LIMIT 1"));
    stmt->setUInt(1, menuId);
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
    if (rs->next())
      return rs->getUInt("TextID");
    return std::nullopt;
  } catch (sql::SQLException const &e) {
    LOG_ERROR("MySqlGossipRepository::GetMenuTextId failed for menuId={}: {}", menuId, e.what());
    return std::nullopt;
  }
}

std::vector<GossipMenuItem> MySqlGossipRepository::GetMenuOptions(uint32_t menuId) const {
  std::vector<GossipMenuItem> result;
  try {
    std::unique_ptr<sql::PreparedStatement> stmt(
        _connection->prepareStatement(
            "SELECT `MenuId`, `OptionIndex`, `OptionIcon`, `OptionText`, "
            "`OptionBroadcastTextId`, `OptionType`, `OptionNpcflag`, `VerifiedBuild` "
            "FROM `gossip_menu_option` WHERE `MenuId` = ? ORDER BY `OptionIndex`"));
    stmt->setUInt(1, menuId);
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
    while (rs->next()) {
      GossipMenuItem item;
      item.menuId = rs->getUInt("MenuId");
      item.optionIndex = rs->getUInt("OptionIndex");
      item.icon = static_cast<GossipOptionIcon>(rs->getUInt("OptionIcon"));
      item.optionText = SafeSqlString(rs->getString("OptionText"));
      item.optionBroadcastTextId = rs->getUInt("OptionBroadcastTextId");
      item.optionType = static_cast<GossipOptionType>(rs->getUInt("OptionType"));
      item.optionNpcflag = rs->getUInt64("OptionNpcflag");
      item.verifiedBuild = static_cast<uint16_t>(rs->getShort("VerifiedBuild"));

      // Load box data if present
      std::unique_ptr<sql::PreparedStatement> boxStmt(
          _connection->prepareStatement(
              "SELECT `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextId` "
              "FROM `gossip_menu_option_box` "
              "WHERE `MenuId` = ? AND `OptionIndex` = ? LIMIT 1"));
      boxStmt->setUInt(1, item.menuId);
      boxStmt->setUInt(2, item.optionIndex);
      std::unique_ptr<sql::ResultSet> boxRs(boxStmt->executeQuery());
      if (boxRs->next()) {
        item.isCoded = boxRs->getUInt("BoxCoded") != 0;
        item.boxMoney = boxRs->getUInt("BoxMoney");
        item.boxMessage = SafeSqlString(boxRs->getString("BoxText"));
        item.boxBroadcastTextId = boxRs->getUInt("BoxBroadcastTextId");
      }

      // Load action data if present
      std::unique_ptr<sql::PreparedStatement> actStmt(
          _connection->prepareStatement(
              "SELECT `ActionMenuId`, `ActionPoiId` FROM `gossip_menu_option_action` "
              "WHERE `MenuId` = ? AND `OptionIndex` = ? LIMIT 1"));
      actStmt->setUInt(1, item.menuId);
      actStmt->setUInt(2, item.optionIndex);
      std::unique_ptr<sql::ResultSet> actRs(actStmt->executeQuery());
      if (actRs->next()) {
        item.actionMenuId = actRs->getUInt("ActionMenuId");
        item.actionPoi = actRs->getUInt("ActionPoiId");
      }

      result.push_back(std::move(item));
    }
  } catch (sql::SQLException const &e) {
    LOG_ERROR("MySqlGossipRepository::GetMenuOptions failed for menuId={}: {}", menuId, e.what());
  }
  return result;
}

} // namespace Firelands

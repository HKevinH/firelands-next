#include "MySqlVendorRepository.h"
#include <shared/Logger.h>

namespace Firelands {

MySqlVendorRepository::MySqlVendorRepository(
    std::shared_ptr<sql::Connection> connection)
    : _connection(std::move(connection)) {}

std::vector<VendorItemEntry>
MySqlVendorRepository::GetVendorItems(uint32_t vendorEntry) const {
  std::vector<VendorItemEntry> out;
  if (vendorEntry == 0)
    return out;

  try {
    std::unique_ptr<sql::PreparedStatement> stmt(_connection->prepareStatement(
        "SELECT `item`, `maxcount`, `incrtime`, `extendedCost`, `type` "
        "FROM `npc_vendor` WHERE `entry` = ? ORDER BY `slot`"));
    stmt->setUInt(1, vendorEntry);
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
    while (rs->next()) {
      VendorItemEntry e;
      e.item = rs->getUInt("item");
      e.maxCount = static_cast<int32_t>(rs->getInt("maxcount"));
      e.incrTime = rs->getUInt("incrtime");
      e.extendedCost = rs->getUInt("extendedCost");
      e.type = static_cast<uint8_t>(rs->getUInt("type"));
      if (e.item != 0)
        out.push_back(e);
    }
  } catch (sql::SQLException const &ex) {
    LOG_ERROR("MySqlVendorRepository::GetVendorItems failed for entry={}: {}",
              vendorEntry, ex.what());
    out.clear();
  }
  return out;
}

} // namespace Firelands

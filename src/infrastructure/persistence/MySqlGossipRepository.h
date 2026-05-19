#pragma once

#include <domain/repositories/IGossipRepository.h>
#include <conncpp.hpp>
#include <memory>

namespace Firelands {

class MySqlGossipRepository final : public IGossipRepository {
public:
  explicit MySqlGossipRepository(std::shared_ptr<sql::Connection> connection);

  std::optional<uint32_t> GetMenuTextId(uint32_t menuId) const override;
  std::vector<GossipMenuItem> GetMenuOptions(uint32_t menuId) const override;

private:
  std::shared_ptr<sql::Connection> _connection;
};

} // namespace Firelands

#pragma once

#include <conncpp.hpp>
#include <domain/repositories/IRealmRepository.h>
#include <memory>

namespace Firelands {

class MySqlRealmRepository : public IRealmRepository {
public:
  explicit MySqlRealmRepository(std::shared_ptr<sql::Connection> connection);

  bool FindById(uint32_t id) override;
  void DeleteById(uint32_t id) override;
  void Create(const Realm &realm) override;
  std::vector<Realm> GetRealms() override;

private:
  std::shared_ptr<sql::Connection> _connection;
};

} // namespace Firelands

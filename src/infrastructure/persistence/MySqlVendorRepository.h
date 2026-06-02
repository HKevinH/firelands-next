#pragma once

#include <domain/repositories/IVendorRepository.h>
#include <conncpp.hpp>
#include <memory>

namespace Firelands {

class MySqlVendorRepository final : public IVendorRepository {
public:
  explicit MySqlVendorRepository(std::shared_ptr<sql::Connection> connection);

  std::vector<VendorItemEntry>
  GetVendorItems(uint32_t vendorEntry) const override;

private:
  std::shared_ptr<sql::Connection> _connection;
};

} // namespace Firelands

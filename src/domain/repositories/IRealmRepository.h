#pragma once

#include <domain/models/Realm.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace Firelands {

class IRealmRepository {
public:
  virtual ~IRealmRepository() = default;

  virtual bool FindById(uint32_t id) = 0;
  virtual void DeleteById(uint32_t id) = 0;
  virtual void Create(const Realm &realm) = 0;
  virtual std::vector<Realm> GetRealms() = 0;

  /// Minimum `account.access_level` required to join this realm (same scale as
  /// `AccessLevel`). Empty if the realm id is not present in `realmlist`.
  virtual std::optional<uint8_t> GetAllowedSecurityLevelForRealm(uint32_t id) = 0;
};

} // namespace Firelands

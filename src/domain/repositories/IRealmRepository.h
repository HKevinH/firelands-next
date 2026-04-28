#pragma once

#include <domain/models/Realm.h>
#include <vector>

namespace Firelands {

class IRealmRepository {
public:
  virtual ~IRealmRepository() = default;

  virtual bool FindById(uint32_t id) = 0;
  virtual void DeleteById(uint32_t id) = 0;
  virtual void Create(const Realm &realm) = 0;
  virtual std::vector<Realm> GetRealms() = 0;
};

} // namespace Firelands

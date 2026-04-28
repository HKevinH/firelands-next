#pragma once

#include <domain/models/Realm.h>
#include <domain/repositories/IRealmRepository.h>
#include <memory>
#include <vector>

namespace Firelands {

class RealmListService {
public:
  explicit RealmListService(std::shared_ptr<IRealmRepository> repository);

  std::vector<Realm> GetRealmList();

private:
  std::shared_ptr<IRealmRepository> m_repository;
};

} // namespace Firelands

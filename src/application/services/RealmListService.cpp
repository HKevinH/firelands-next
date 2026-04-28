#include "RealmListService.h"

namespace Firelands {

RealmListService::RealmListService(std::shared_ptr<IRealmRepository> repository)
    : m_repository(std::move(repository)) {}

std::vector<Realm> RealmListService::GetRealmList() {
  if (!m_repository) {
    return {};
  }
  return m_repository->GetRealms();
}

} // namespace Firelands

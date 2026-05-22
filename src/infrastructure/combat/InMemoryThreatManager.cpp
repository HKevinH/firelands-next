#include "InMemoryThreatManager.h"

#include <algorithm>

namespace infrastructure {

void InMemoryThreatManager::AddThreat(uint64_t ownerGuid, uint64_t threatSourceGuid,
                                      float amount) {
  if (ownerGuid == 0 || threatSourceGuid == 0 || amount <= 0.f)
    return;
  _tables[ownerGuid][threatSourceGuid] += amount;
}

void InMemoryThreatManager::RemoveThreat(uint64_t ownerGuid, uint64_t threatSourceGuid) {
  auto ownerIt = _tables.find(ownerGuid);
  if (ownerIt == _tables.end())
    return;
  ownerIt->second.erase(threatSourceGuid);
  if (ownerIt->second.empty())
    _tables.erase(ownerIt);
}

uint64_t InMemoryThreatManager::GetTopThreat(uint64_t ownerGuid) const {
  auto ownerIt = _tables.find(ownerGuid);
  if (ownerIt == _tables.end() || ownerIt->second.empty())
    return 0;

  uint64_t topGuid = 0;
  float topAmount = 0.f;
  for (auto const &[guid, amount] : ownerIt->second) {
    if (guid == 0)
      continue;
    if (topGuid == 0 || amount > topAmount) {
      topGuid = guid;
      topAmount = amount;
    }
  }
  return topGuid;
}

} // namespace infrastructure

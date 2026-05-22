#include "MySqlThreatManager.h"

namespace infrastructure {
    void MySqlThreatManager::AddThreat(uint64_t ownerGuid, uint64_t threatSourceGuid,
                                       float amount) {
        (void)ownerGuid;
        (void)threatSourceGuid;
        (void)amount;
    }
    void MySqlThreatManager::RemoveThreat(uint64_t ownerGuid, uint64_t threatSourceGuid) {
        (void)ownerGuid;
        (void)threatSourceGuid;
    }
    uint64_t MySqlThreatManager::GetTopThreat(uint64_t ownerGuid) const {
        (void)ownerGuid;
        return 0;
    }
}

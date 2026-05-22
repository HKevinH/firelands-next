#pragma once

#include <domain/combat/repositories/IThreatManager.h>
#include <unordered_map>

namespace infrastructure {
    class InMemoryThreatManager : public combat::IThreatManager {
    public:
        void AddThreat(uint64_t ownerGuid, uint64_t threatSourceGuid, float amount) override;
        void RemoveThreat(uint64_t ownerGuid, uint64_t threatSourceGuid) override;
        uint64_t GetTopThreat(uint64_t ownerGuid) const override;

    private:
        std::unordered_map<uint64_t, std::unordered_map<uint64_t, float>> _tables;
    };
}

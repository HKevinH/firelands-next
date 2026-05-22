#pragma once
#include <cstdint>

namespace combat {
    /// Per-hostile-unit threat table (creature guid owns the table).
    class IThreatManager {
    public:
        virtual ~IThreatManager() = default;
        virtual void AddThreat(uint64_t ownerGuid, uint64_t threatSourceGuid, float amount) = 0;
        virtual void RemoveThreat(uint64_t ownerGuid, uint64_t threatSourceGuid) = 0;
        virtual uint64_t GetTopThreat(uint64_t ownerGuid) const = 0;
    };
}

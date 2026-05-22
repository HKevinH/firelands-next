#pragma once

#include <shared/Common.h>

namespace Firelands {

    class WorldPacket;

    class IMapNotifier {
    public:
        virtual ~IMapNotifier() = default;
        virtual void SendPacket(WorldPacket& packet) = 0;
        virtual uint64 GetGuid() const = 0;
        /// Called when a player reduces a creature to 0 HP (melee, spells, DoTs).
        virtual void OnCreatureKilledByPlayer(uint64 creatureGuid, uint32 hpBefore) {}
    };

} // namespace Firelands

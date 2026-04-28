#pragma once

#include <shared/Common.h>

namespace Firelands {

    class WorldPacket;

    class IMapNotifier {
    public:
        virtual ~IMapNotifier() = default;
        virtual void SendPacket(WorldPacket& packet) = 0;
        virtual uint64 GetGuid() const = 0;
    };

} // namespace Firelands

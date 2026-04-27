#pragma once

#include <shared/network/ServerPacket.h>

namespace Firelands {
namespace WorldPackets {
namespace Item {

class SetProficiency : public ServerPacket {
public:
    SetProficiency(uint8 itemClass, uint32 itemMask) 
        : ServerPacket(SMSG_SET_PROFICIENCY, 5), 
          _class(itemClass), _mask(itemMask) {}

    WorldPacket const* Write() override {
        _worldPacket.Append<uint8>(_class);
        _worldPacket.Append<uint32>(_mask);
        return &_worldPacket;
    }

private:
    uint8 _class;
    uint32 _mask;
};

} // namespace Item
} // namespace WorldPackets
} // namespace Firelands

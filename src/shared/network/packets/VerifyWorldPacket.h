#pragma once

#include <shared/network/ServerPacket.h>

namespace Firelands {
namespace WorldPackets {
namespace Login {

class VerifyWorld : public ServerPacket {
public:
    VerifyWorld(int32 mapId, float x, float y, float z, float o) 
        : ServerPacket(SMSG_LOGIN_VERIFY_WORLD, 20), 
          _mapId(mapId), _x(x), _y(y), _z(z), _o(o) {}

    WorldPacket const* Write() override {
        _worldPacket.Append<int32>(_mapId);
        _worldPacket.Append<float>(_x);
        _worldPacket.Append<float>(_y);
        _worldPacket.Append<float>(_z);
        _worldPacket.Append<float>(_o);
        return &_worldPacket;
    }

private:
    int32 _mapId;
    float _x, _y, _z, _o;
};

} // namespace Login
} // namespace WorldPackets
} // namespace Firelands

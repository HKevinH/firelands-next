#pragma once

#include <shared/network/WorldPacket.h>

namespace Firelands {

class ServerPacket {
public:
    ServerPacket(uint32 opcode, size_t size) : _opcode(opcode), _worldPacket(opcode, size) {}
    virtual ~ServerPacket() = default;

    virtual WorldPacket const* Write() = 0;

protected:
    uint32 _opcode;
    WorldPacket _worldPacket;
};

} // namespace Firelands

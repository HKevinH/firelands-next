#pragma once

#include <shared/network/ServerPacket.h>
#include <string>
#include <vector>

namespace Firelands {
namespace WorldPackets {
namespace Misc {

class Motd : public ServerPacket {
public:
    Motd(std::vector<std::string> lines) : ServerPacket(SMSG_MOTD, 4 + lines.size() * 32), _lines(lines) {}

    WorldPacket const* Write() override {
        _worldPacket.Append<uint32>(static_cast<uint32>(_lines.size()));
        for (auto const &line : _lines) {
            _worldPacket.WriteString(line);
        }
        return &_worldPacket;
    }

private:
    std::vector<std::string> _lines;
};

} // namespace Misc
} // namespace WorldPackets
} // namespace Firelands

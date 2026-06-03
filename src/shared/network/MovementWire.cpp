#include <shared/network/MovementWire.h>
#include <shared/network/movement/ClientMovementMse.h>

namespace Firelands {

bool TryReadClientMovement(WorldPacket &packet, WorldOpcode opcode,
                           MovementInfo &move, uint64 *outMoverGuid) {
  return TryReadClientMovementMse(packet, static_cast<uint32>(opcode), move,
                                  outMoverGuid);
}

bool BuildServerMovement(WorldPacket &out, WorldOpcode opcode,
                         MovementInfo const &move, uint64 moverGuid) {
  return BuildServerMovementMse(out, static_cast<uint32>(opcode), move,
                                moverGuid);
}

} // namespace Firelands

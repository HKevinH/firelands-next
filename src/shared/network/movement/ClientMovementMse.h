#pragma once

#include <shared/network/MovementInfo.h>
#include <shared/network/WorldPacket.h>

namespace Firelands {

/// Decode client `MSG_MOVE_*` payloads using the opcode-specific Movement Status
/// Element (MSE) sequence used on the wire for this project’s client build.
/// `outMoverGuid` (optional) receives the mover ObjectGuid carried in the packet
/// (diagnostic: lets the caller verify it matches the session's player).
bool TryReadClientMovementMse(WorldPacket &packet, uint32 opcode,
                              MovementInfo &move, uint64 *outMoverGuid = nullptr);

} // namespace Firelands

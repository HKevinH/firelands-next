#ifndef FIRELANDS_INFRASTRUCTURE_NETWORK_SESSIONS_WORLDSESSION_MOVEMENT_CHECKS_H
#define FIRELANDS_INFRASTRUCTURE_NETWORK_SESSIONS_WORLDSESSION_MOVEMENT_CHECKS_H

#include <shared/network/MovementInfo.h>
#include <shared/network/WorldOpcodes.h>

#include <cmath>

namespace Firelands {

/// Match reference `Firelands::IsValidMapCoord` bounds + sane orientation for
/// movement wire parsing (shared by login position validation and movement echo).
inline constexpr float kWsMapCoordLimit =
    (533.3333f * 64.0f) * 0.5f - 0.5f;

inline constexpr float kWsMaxOrientation = 7.0f; // slightly above 2*pi (~6.283)

inline bool WsIsSaneWorldPosition(MovementInfo const &m) {
  if (!std::isfinite(m.x) || !std::isfinite(m.y) || !std::isfinite(m.z) ||
      !std::isfinite(m.orientation))
    return false;
  if (std::fabs(m.orientation) > kWsMaxOrientation)
    return false;
  return std::fabs(m.x) <= kWsMapCoordLimit && std::fabs(m.y) <= kWsMapCoordLimit &&
         std::fabs(m.z) <= kWsMapCoordLimit;
}

/// Opcodes whose MSE layout reliably includes X/Y/Z (see `MovementStatusSequences.inc`).
/// Other `MSG_MOVE_*` opcodes still only merge flags/time — never trust their coords.
inline bool WsIsTrustedPositionOpcode(WorldOpcode opcode) {
  switch (opcode) {
  case MSG_MOVE_HEARTBEAT:
  case MSG_MOVE_STOP:
  case MSG_MOVE_SET_FACING:
  case MSG_MOVE_FALL_LAND:
    return true;
  default:
    return false;
  }
}

/// Adopt X/Y/Z from a parsed non-heartbeat `MSG_MOVE_*` when the delta is plausible
/// (keeps combat range in sync while the client walks between heartbeats).
inline bool WsTryAdoptParsedMovementPosition(MovementInfo &current,
                                             MovementInfo const &parsed,
                                             float maxSpeedYardsPerSec) {
  if (!WsIsSaneWorldPosition(parsed))
    return false;

  uint32_t dtMs = 500u;
  if (parsed.time >= current.time)
    dtMs = parsed.time - current.time;
  dtMs = std::min(dtMs, 2000u);
  if (dtMs < 50u)
    dtMs = 50u;

  float const dx = parsed.x - current.x;
  float const dy = parsed.y - current.y;
  float const dz = parsed.z - current.z;
  float const dist = std::sqrt(dx * dx + dy * dy + dz * dz);
  float const speed = std::max(0.01f, maxSpeedYardsPerSec);
  float const maxStep = speed * (static_cast<float>(dtMs) / 1000.0f) + 1.5f;
  if (dist > maxStep)
    return false;

  current.x = parsed.x;
  current.y = parsed.y;
  current.z = parsed.z;
  if (std::isfinite(parsed.orientation))
    current.orientation = parsed.orientation;
  current.time = parsed.time;
  return true;
}

inline bool WsIsClientMovementOpcode(WorldOpcode opcode) {
  switch (opcode) {
  case MSG_MOVE_HEARTBEAT:
  case MSG_MOVE_START_FORWARD:
  case MSG_MOVE_START_BACKWARD:
  case MSG_MOVE_START_STRAFE_LEFT:
  case MSG_MOVE_START_STRAFE_RIGHT:
  case MSG_MOVE_STOP:
  case MSG_MOVE_STOP_STRAFE:
  case MSG_MOVE_START_ASCEND:
  case MSG_MOVE_START_DESCEND:
  case MSG_MOVE_STOP_ASCEND:
  case MSG_MOVE_START_TURN_LEFT:
  case MSG_MOVE_START_TURN_RIGHT:
  case MSG_MOVE_STOP_TURN:
  case MSG_MOVE_START_PITCH_UP:
  case MSG_MOVE_START_PITCH_DOWN:
  case MSG_MOVE_STOP_PITCH:
  case MSG_MOVE_SET_RUN_MODE:
  case MSG_MOVE_SET_WALK_MODE:
  case MSG_MOVE_START_SWIM:
  case MSG_MOVE_STOP_SWIM:
  case MSG_MOVE_JUMP:
  case MSG_MOVE_SET_FACING:
  case MSG_MOVE_FALL_LAND:
  case CMSG_MOVE_SET_CAN_FLY:
  case CMSG_MOVE_SET_CAN_FLY_ACK:
    return true;
  default:
    return false;
  }
}

} // namespace Firelands

#endif

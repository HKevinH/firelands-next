#include <shared/network/movement/ClientMovementMse.h>
#include <shared/network/movement/MovementStatusElements.h>
#include <shared/network/BitReader.h>
#include <shared/network/BitWriter.h>
#include <shared/network/MovementFlags.h>
#include <shared/network/WorldOpcodes.h>

#include <cmath>
#include <cstdint>

namespace Firelands {

#include <shared/network/movement/MovementStatusSequences.inc>

namespace {

MovementStatusElements const *GetClientMovementStatusSequence(uint32 opcode) {
  switch (opcode) {
  case SMSG_MOVE_UPDATE:
    return MovementUpdate;
  case MSG_MOVE_FALL_LAND:
    return MovementFallLand;
  case MSG_MOVE_HEARTBEAT:
    return MovementHeartBeat;
  case MSG_MOVE_JUMP:
    return MovementJump;
  case MSG_MOVE_SET_FACING:
    return MovementSetFacing;
  case MSG_MOVE_START_ASCEND:
    return MovementStartAscend;
  case MSG_MOVE_START_BACKWARD:
    return MovementStartBackward;
  case MSG_MOVE_START_DESCEND:
    return MovementStartDescend;
  case MSG_MOVE_START_FORWARD:
    return MovementStartForward;
  case MSG_MOVE_START_PITCH_DOWN:
    return MovementStartPitchDown;
  case MSG_MOVE_START_PITCH_UP:
    return MovementStartPitchUp;
  case MSG_MOVE_START_STRAFE_LEFT:
    return MovementStartStrafeLeft;
  case MSG_MOVE_START_STRAFE_RIGHT:
    return MovementStartStrafeRight;
  case MSG_MOVE_START_SWIM:
    return MovementStartSwim;
  case MSG_MOVE_START_TURN_LEFT:
    return MovementStartTurnLeft;
  case MSG_MOVE_START_TURN_RIGHT:
    return MovementStartTurnRight;
  case MSG_MOVE_STOP:
    return MovementStop;
  case MSG_MOVE_STOP_ASCEND:
    return MovementStopAscend;
  case MSG_MOVE_STOP_PITCH:
    return MovementStopPitch;
  case MSG_MOVE_STOP_STRAFE:
    return MovementStopStrafe;
  case MSG_MOVE_STOP_SWIM:
    return MovementStopSwim;
  case MSG_MOVE_STOP_TURN:
    return MovementStopTurn;
  case MSG_MOVE_SET_RUN_MODE:
    return MovementSetRunMode;
  case MSG_MOVE_SET_WALK_MODE:
    return MovementSetWalkMode;
  case CMSG_MOVE_FORCE_RUN_SPEED_CHANGE_ACK:
    return MovementForceRunSpeedChangeAck;
  case CMSG_MOVE_FORCE_FLIGHT_SPEED_CHANGE_ACK:
    return MovementForceFlightSpeedChangeAck;
  default:
    return nullptr;
  }
}

template <typename T> bool ReadChecked(ByteBuffer &bb, T &out) {
  if (bb.GetReadPos() + sizeof(T) > bb.Size())
    return false;
  out = bb.Read<T>();
  return true;
}

inline bool ConsumeByteSeq(ByteBuffer &bb, bool present,
                           uint8_t *outByte = nullptr) {
  if (!present)
    return true;
  uint8_t b = 0;
  if (!ReadChecked(bb, b))
    return false;
  // Packed-guid bytes are stored XOR 1 on the wire (same convention used by the
  // bit-packed SMSG builders in this project).
  if (outByte)
    *outByte = static_cast<uint8_t>(b ^ 1u);
  return true;
}

} // namespace

bool TryReadClientMovementMse(WorldPacket &packet, uint32 opcode,
                              MovementInfo &move, uint64 *outMoverGuid) {
  MovementStatusElements const *sequence = GetClientMovementStatusSequence(opcode);
  if (!sequence)
    return false;

  move = MovementInfo{};
  uint8_t guidBytes[8] = {};

  bool hasMovementFlags = false;
  bool hasMovementFlags2 = false;
  bool hasTimestamp = false;
  bool hasOrientation = false;
  bool hasTransportData = false;
  bool hasTransportTime2 = false;
  bool hasTransportVehicleId = false;
  bool hasPitch = false;
  bool hasFallData = false;
  bool hasFallDirection = false;
  bool hasSplineElevation = false;

  bool guidSend[8] = {};
  bool tguidSend[8] = {};

  BitReader br(packet);

  for (MovementStatusElements const *it = sequence; *it != MSEEnd; ++it) {
    MovementStatusElements const el = *it;
    switch (el) {
    case MSEHasGuidByte0:
    case MSEHasGuidByte1:
    case MSEHasGuidByte2:
    case MSEHasGuidByte3:
    case MSEHasGuidByte4:
    case MSEHasGuidByte5:
    case MSEHasGuidByte6:
    case MSEHasGuidByte7:
      guidSend[static_cast<size_t>(el - MSEHasGuidByte0)] = br.ReadBit();
      break;
    case MSEHasTransportGuidByte0:
    case MSEHasTransportGuidByte1:
    case MSEHasTransportGuidByte2:
    case MSEHasTransportGuidByte3:
    case MSEHasTransportGuidByte4:
    case MSEHasTransportGuidByte5:
    case MSEHasTransportGuidByte6:
    case MSEHasTransportGuidByte7:
      if (hasTransportData)
        tguidSend[static_cast<size_t>(el - MSEHasTransportGuidByte0)] =
            br.ReadBit();
      break;
    case MSEGuidByte0:
    case MSEGuidByte1:
    case MSEGuidByte2:
    case MSEGuidByte3:
    case MSEGuidByte4:
    case MSEGuidByte5:
    case MSEGuidByte6:
    case MSEGuidByte7:
      br.AlignToByteBoundary();
      {
        size_t const gi = static_cast<size_t>(el - MSEGuidByte0);
        if (!ConsumeByteSeq(packet, guidSend[gi], &guidBytes[gi]))
          return false;
      }
      br.ResyncAfterExternalByteReads();
      break;
    case MSETransportGuidByte0:
    case MSETransportGuidByte1:
    case MSETransportGuidByte2:
    case MSETransportGuidByte3:
    case MSETransportGuidByte4:
    case MSETransportGuidByte5:
    case MSETransportGuidByte6:
    case MSETransportGuidByte7:
      if (hasTransportData) {
        br.AlignToByteBoundary();
        if (!ConsumeByteSeq(
                packet, tguidSend[static_cast<size_t>(el - MSETransportGuidByte0)]))
          return false;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSEHasMovementFlags:
      hasMovementFlags = !br.ReadBit();
      break;
    case MSEHasMovementFlags2:
      hasMovementFlags2 = !br.ReadBit();
      break;
    case MSEHasTimestamp:
      hasTimestamp = !br.ReadBit();
      break;
    case MSEHasOrientation:
      hasOrientation = !br.ReadBit();
      break;
    case MSEHasTransportData:
      hasTransportData = br.ReadBit();
      break;
    case MSEHasTransportTime2:
      if (hasTransportData)
        hasTransportTime2 = br.ReadBit();
      break;
    case MSEHasVehicleId:
      if (hasTransportData)
        hasTransportVehicleId = br.ReadBit();
      break;
    case MSEHasPitch:
      hasPitch = !br.ReadBit();
      break;
    case MSEHasFallData:
      hasFallData = br.ReadBit();
      move.hasFallData = hasFallData;
      break;
    case MSEHasFallDirection:
      if (hasFallData) {
        hasFallDirection = br.ReadBit();
        move.hasFallDirection = hasFallDirection;
      }
      break;
    case MSEHasSplineElevation:
      hasSplineElevation = !br.ReadBit();
      break;
    case MSEHasSpline:
      (void)br.ReadBit();
      break;
    case MSEHasHeightChangeFailed:
      (void)br.ReadBit();
      break;
    case MSEZeroBit:
    case MSEOneBit:
      (void)br.ReadBit();
      break;
    case MSEFlushBits:
      br.AlignToByteBoundary();
      break;
    case MSEMovementFlags:
      if (hasMovementFlags)
        move.flags = br.ReadBits(30) & kMovementFlagsWireMask;
      break;
    case MSEMovementFlags2:
      if (hasMovementFlags2)
        move.flags2 =
            static_cast<uint16>(br.ReadBits(12) & kMovementFlags2WireMask);
      break;
    case MSETimestamp:
      if (hasTimestamp) {
        br.AlignToByteBoundary();
        if (!ReadChecked(packet, move.time))
          return false;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSEPositionX:
      br.AlignToByteBoundary();
      if (!ReadChecked(packet, move.x))
        return false;
      br.ResyncAfterExternalByteReads();
      break;
    case MSEPositionY:
      br.AlignToByteBoundary();
      if (!ReadChecked(packet, move.y))
        return false;
      br.ResyncAfterExternalByteReads();
      break;
    case MSEPositionZ:
      br.AlignToByteBoundary();
      if (!ReadChecked(packet, move.z))
        return false;
      br.ResyncAfterExternalByteReads();
      break;
    case MSEOrientation:
      if (hasOrientation) {
        br.AlignToByteBoundary();
        if (!ReadChecked(packet, move.orientation))
          return false;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSETransportPositionX:
      if (hasTransportData) {
        br.AlignToByteBoundary();
        float v = 0.0f;
        if (!ReadChecked(packet, v))
          return false;
        (void)v;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSETransportPositionY:
      if (hasTransportData) {
        br.AlignToByteBoundary();
        float v = 0.0f;
        if (!ReadChecked(packet, v))
          return false;
        (void)v;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSETransportPositionZ:
      if (hasTransportData) {
        br.AlignToByteBoundary();
        float v = 0.0f;
        if (!ReadChecked(packet, v))
          return false;
        (void)v;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSETransportOrientation:
      if (hasTransportData) {
        br.AlignToByteBoundary();
        float v = 0.0f;
        if (!ReadChecked(packet, v))
          return false;
        (void)v;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSETransportSeat:
      if (hasTransportData) {
        br.AlignToByteBoundary();
        int8_t seat = 0;
        if (!ReadChecked(packet, seat))
          return false;
        (void)seat;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSETransportTime:
      if (hasTransportData) {
        br.AlignToByteBoundary();
        uint32_t t = 0;
        if (!ReadChecked(packet, t))
          return false;
        (void)t;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSETransportTime2:
      if (hasTransportData && hasTransportTime2) {
        br.AlignToByteBoundary();
        uint32_t t = 0;
        if (!ReadChecked(packet, t))
          return false;
        (void)t;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSETransportVehicleId:
      if (hasTransportData && hasTransportVehicleId) {
        br.AlignToByteBoundary();
        uint32_t vid = 0;
        if (!ReadChecked(packet, vid))
          return false;
        (void)vid;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSEPitch:
      if (hasPitch) {
        br.AlignToByteBoundary();
        float p = 0.0f;
        if (!ReadChecked(packet, p))
          return false;
        (void)p;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSEFallTime:
      if (hasFallData) {
        br.AlignToByteBoundary();
        if (!ReadChecked(packet, move.fallTime))
          return false;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSEFallVerticalSpeed:
      if (hasFallData) {
        br.AlignToByteBoundary();
        if (!ReadChecked(packet, move.jumpVerticalSpeed))
          return false;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSEFallCosAngle:
      if (hasFallData && hasFallDirection) {
        br.AlignToByteBoundary();
        if (!ReadChecked(packet, move.jumpCosAngle))
          return false;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSEFallSinAngle:
      if (hasFallData && hasFallDirection) {
        br.AlignToByteBoundary();
        if (!ReadChecked(packet, move.jumpSinAngle))
          return false;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSEFallHorizontalSpeed:
      if (hasFallData && hasFallDirection) {
        br.AlignToByteBoundary();
        if (!ReadChecked(packet, move.jumpHorizontalSpeed))
          return false;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSESplineElevation:
      if (hasSplineElevation) {
        br.AlignToByteBoundary();
        float v = 0.0f;
        if (!ReadChecked(packet, v))
          return false;
        (void)v;
        br.ResyncAfterExternalByteReads();
      }
      break;
    case MSECounter: {
      br.AlignToByteBoundary();
      uint32_t c = 0;
      if (!ReadChecked(packet, c))
        return false;
      (void)c;
      br.ResyncAfterExternalByteReads();
      break;
    }
    case MSEExtraElement:
    case MSEExtraFloat: {
      br.AlignToByteBoundary();
      float extra = 0.f;
      if (!ReadChecked(packet, extra))
        return false;
      (void)extra;
      br.ResyncAfterExternalByteReads();
      break;
    }
    case MSEExtraInt8:
    case MSEExtraTwoBits:
      return false;
    default:
      return false;
    }
  }

  if (!std::isfinite(move.x) || !std::isfinite(move.y) || !std::isfinite(move.z) ||
      !std::isfinite(move.orientation))
    return false;

  if (outMoverGuid) {
    uint64 g = 0;
    for (size_t i = 0; i < 8; ++i)
      g |= static_cast<uint64>(guidBytes[i]) << (i * 8);
    *outMoverGuid = g;
  }

  return true;
}

bool BuildServerMovementMse(WorldPacket &out, uint32 opcode,
                            MovementInfo const &move, uint64 moverGuid) {
  MovementStatusElements const *sequence = GetClientMovementStatusSequence(opcode);
  if (!sequence)
    return false;

  out.SetOpcode(opcode);

  uint8_t guidBytes[8] = {};
  for (size_t i = 0; i < 8; ++i)
    guidBytes[i] = static_cast<uint8_t>((moverGuid >> (i * 8)) & 0xFFu);

  // What this relay sends: position, movement flags, timestamp and orientation.
  // Transport / pitch / fall / spline are not relayed (the reader discards them
  // too), so their presence bits are written as "absent".
  uint32 const movementFlags0 = move.flags & kMovementFlagsWireMask;
  uint16 const movementFlags1 =
      static_cast<uint16>(move.flags2 & kMovementFlags2WireMask);
  bool const hasMovementFlags = movementFlags0 != 0;
  bool const hasMovementFlags2 = movementFlags1 != 0;
  bool const hasTimestamp = true;
  bool const hasOrientation = true;
  bool const hasTransportData = false;
  bool const hasPitch = false;
  bool const hasFallData = move.hasFallData;
  bool const hasFallDirection = move.hasFallData && move.hasFallDirection;
  bool const hasSplineElevation = false;

  BitWriter bw(out);

  for (MovementStatusElements const *it = sequence; *it != MSEEnd; ++it) {
    switch (*it) {
    case MSEHasGuidByte0:
    case MSEHasGuidByte1:
    case MSEHasGuidByte2:
    case MSEHasGuidByte3:
    case MSEHasGuidByte4:
    case MSEHasGuidByte5:
    case MSEHasGuidByte6:
    case MSEHasGuidByte7:
      bw.WriteBit(guidBytes[static_cast<size_t>(*it - MSEHasGuidByte0)] != 0);
      break;
    case MSEHasTransportGuidByte0:
    case MSEHasTransportGuidByte1:
    case MSEHasTransportGuidByte2:
    case MSEHasTransportGuidByte3:
    case MSEHasTransportGuidByte4:
    case MSEHasTransportGuidByte5:
    case MSEHasTransportGuidByte6:
    case MSEHasTransportGuidByte7:
      if (hasTransportData)
        bw.WriteBit(false);
      break;
    case MSEGuidByte0:
    case MSEGuidByte1:
    case MSEGuidByte2:
    case MSEGuidByte3:
    case MSEGuidByte4:
    case MSEGuidByte5:
    case MSEGuidByte6:
    case MSEGuidByte7: {
      bw.Flush();
      uint8_t const b = guidBytes[static_cast<size_t>(*it - MSEGuidByte0)];
      if (b != 0)
        out.Append<uint8_t>(static_cast<uint8_t>(b ^ 1u));
      break;
    }
    case MSETransportGuidByte0:
    case MSETransportGuidByte1:
    case MSETransportGuidByte2:
    case MSETransportGuidByte3:
    case MSETransportGuidByte4:
    case MSETransportGuidByte5:
    case MSETransportGuidByte6:
    case MSETransportGuidByte7:
      // transport not relayed
      break;
    case MSEHasMovementFlags:
      bw.WriteBit(!hasMovementFlags);
      break;
    case MSEHasMovementFlags2:
      bw.WriteBit(!hasMovementFlags2);
      break;
    case MSEHasTimestamp:
      bw.WriteBit(!hasTimestamp);
      break;
    case MSEHasOrientation:
      bw.WriteBit(!hasOrientation);
      break;
    case MSEHasTransportData:
      bw.WriteBit(hasTransportData);
      break;
    case MSEHasTransportTime2:
    case MSEHasVehicleId:
      if (hasTransportData)
        bw.WriteBit(false);
      break;
    case MSEHasPitch:
      bw.WriteBit(!hasPitch);
      break;
    case MSEHasFallData:
      bw.WriteBit(hasFallData);
      break;
    case MSEHasFallDirection:
      if (hasFallData)
        bw.WriteBit(hasFallDirection);
      break;
    case MSEHasSplineElevation:
      bw.WriteBit(!hasSplineElevation);
      break;
    case MSEHasSpline:
    case MSEHasHeightChangeFailed:
    case MSEZeroBit:
      bw.WriteBit(false);
      break;
    case MSEOneBit:
      bw.WriteBit(true);
      break;
    case MSEFlushBits:
      bw.Flush();
      break;
    case MSEMovementFlags:
      if (hasMovementFlags)
        bw.WriteBits(movementFlags0, 30);
      break;
    case MSEMovementFlags2:
      if (hasMovementFlags2)
        bw.WriteBits(movementFlags1, 12);
      break;
    case MSETimestamp:
      if (hasTimestamp) {
        bw.Flush();
        out.Append<uint32>(static_cast<uint32>(move.time));
      }
      break;
    case MSEPositionX:
      bw.Flush();
      out.Append<float>(move.x);
      break;
    case MSEPositionY:
      bw.Flush();
      out.Append<float>(move.y);
      break;
    case MSEPositionZ:
      bw.Flush();
      out.Append<float>(move.z);
      break;
    case MSEOrientation:
      if (hasOrientation) {
        bw.Flush();
        out.Append<float>(move.orientation);
      }
      break;
    case MSEFallTime:
      if (hasFallData) {
        bw.Flush();
        out.Append<uint32>(move.fallTime);
      }
      break;
    case MSEFallVerticalSpeed:
      if (hasFallData) {
        bw.Flush();
        out.Append<float>(move.jumpVerticalSpeed);
      }
      break;
    case MSEFallCosAngle:
      if (hasFallData && hasFallDirection) {
        bw.Flush();
        out.Append<float>(move.jumpCosAngle);
      }
      break;
    case MSEFallSinAngle:
      if (hasFallData && hasFallDirection) {
        bw.Flush();
        out.Append<float>(move.jumpSinAngle);
      }
      break;
    case MSEFallHorizontalSpeed:
      if (hasFallData && hasFallDirection) {
        bw.Flush();
        out.Append<float>(move.jumpHorizontalSpeed);
      }
      break;
    case MSETransportPositionX:
    case MSETransportPositionY:
    case MSETransportPositionZ:
    case MSETransportOrientation:
    case MSETransportSeat:
    case MSETransportTime:
    case MSETransportTime2:
    case MSETransportVehicleId:
    case MSEPitch:
    case MSESplineElevation:
      // not relayed (corresponding "has" bit written false above)
      break;
    case MSECounter:
      bw.Flush();
      out.Append<uint32>(0);
      break;
    case MSEExtraElement:
    case MSEExtraFloat:
      bw.Flush();
      out.Append<float>(0.0f);
      break;
    case MSEExtraInt8:
    case MSEExtraTwoBits:
      return false;
    default:
      return false;
    }
  }

  bw.Flush();
  return true;
}

} // namespace Firelands

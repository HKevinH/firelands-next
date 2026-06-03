#pragma once

#include <shared/network/BitWriter.h>
#include <shared/network/MovementFlags.h>
#include <shared/network/MovementInfo.h>
#include <shared/network/MovementSetPackets.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>

namespace Firelands::move_update_wire {

/// Builds `SMSG_MOVE_UPDATE` (4.3.4.15595) — the packet broadcasts so
/// observers render another unit's movement natively (smooth interpolation,
/// rotation, jumps). The body is the same bit-packed `MovementInfo` wire layout
/// as the working `SMSG_MOVE_UPDATE_TELEPORT` builder (position floats first,
/// then the bit block, then the guid byte sequence), with the real movement
/// flags so the client animates walking/running/jumping.
///
/// `move.time` is expected to already be in SERVER time (caller adjusts it via
/// the client's time-sync clock delta). Transport / fall / pitch / spline are
/// not relayed here (the common ground-movement case).
inline WorldPacket BuildMoveUpdate(uint64 moverGuid, MovementInfo const &move) {
  using movement_set_packets_detail::GuidByteLe;
  auto G = [&](unsigned i) { return GuidByteLe(moverGuid, i); };

  uint32 const movementFlags = move.flags & kMovementFlagsWireMask;
  uint16 const movementFlags2 =
      static_cast<uint16>(move.flags2 & kMovementFlags2WireMask);
  bool const hasOrientation = true;
  bool const hasSpline = false;
  bool const hasMovementFlags = movementFlags != 0;
  bool const hasFallData = false;
  bool const hasTransportData = false;
  bool const hasHeightChangeFailed = false;
  bool const hasPitch = false;
  bool const hasExtraMovementFlags = movementFlags2 != 0;
  bool const hasTime = move.time != 0;
  bool const hasSplineElevation = false;

  WorldPacket pkt(static_cast<uint32>(SMSG_MOVE_UPDATE), 96);
  pkt.Append<float>(move.z);
  pkt.Append<float>(move.y);
  pkt.Append<float>(move.x);

  BitWriter bw(pkt);
  bw.WriteBit(!hasOrientation);
  bw.WriteBit(hasSpline);
  bw.WriteBit(!hasMovementFlags);
  bw.WriteBitMask(G(2));
  bw.WriteBitMask(G(4));
  bw.WriteBitMask(G(6));
  bw.WriteBit(hasFallData);
  bw.WriteBitMask(G(0));
  bw.WriteBit(hasTransportData);
  bw.WriteBitMask(G(5));

  bw.WriteBit(hasHeightChangeFailed);
  bw.WriteBitMask(G(7));
  bw.WriteBitMask(G(3));

  bw.WriteBit(!hasPitch);
  bw.WriteBit(!hasExtraMovementFlags);
  bw.WriteBit(!hasTime);

  if (hasFallData)
    bw.WriteBit(false); // HasFallDirection

  if (hasExtraMovementFlags)
    bw.WriteBits(movementFlags2, 12);

  bw.WriteBit(!hasSplineElevation);

  if (hasMovementFlags)
    bw.WriteBits(movementFlags, 30);

  bw.WriteBitMask(G(1));
  bw.Flush();

  pkt.WriteByteSeq(G(7));
  pkt.WriteByteSeq(G(6));

  if (hasPitch)
    pkt.Append<float>(0.f);

  if (hasSplineElevation)
    pkt.Append<float>(0.f);

  if (hasOrientation)
    pkt.Append<float>(move.orientation);

  pkt.WriteByteSeq(G(2));
  pkt.WriteByteSeq(G(3));
  pkt.WriteByteSeq(G(1));

  pkt.WriteByteSeq(G(5));
  pkt.WriteByteSeq(G(4));

  if (hasTime)
    pkt.Append<uint32>(static_cast<uint32>(move.time));

  pkt.WriteByteSeq(G(0));
  return pkt;
}

} // namespace Firelands::move_update_wire

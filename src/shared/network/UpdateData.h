#pragma once

#include <map>
#include <shared/network/BitWriter.h>
#include <shared/network/ByteBuffer.h>
#include <shared/network/MovementInfo.h>
#include <shared/network/UpdateFields.h>
#include <shared/network/WorldPacket.h>
#include <vector>

namespace Firelands {

/**
 * @brief Accumulates object updates to be sent in a single SMSG_UPDATE_OBJECT
 * packet. Specific to Cataclysm 4.3.4 format.
 */
class UpdateData {
public:
  UpdateData() : _count(0), _mapId(0) {}
  explicit UpdateData(uint16 mapId) : _count(0), _mapId(mapId) {}

  /**
   * @brief Adds a CreateObject block to the update data.
   * @param guid The GUID of the object being created.
   * @param typeId The TypeID of the object.
   * @param move Movement information.
   * @param fields Update fields.
   */
  void AddCreateObject(uint64 guid, TypeID typeId, MovementInfo const &move,
                       std::map<uint16, uint32> const &fields) {
    _count++;
    _data.Append<uint8>(UPDATETYPE_CREATE_OBJECT2);
    _data.WritePackedGuid(guid);
    _data.Append<uint8>(static_cast<uint8>(typeId));

    // Cataclysm 4.3.4 Build 15595 Movement Flags Bit Order
    uint32 flags = 0x20; // UPDATEFLAG_SELF (0x20)
    if (typeId == TYPEID_PLAYER || typeId == TYPEID_UNIT) {
      flags |= 0x80; // UPDATEFLAG_LIVING (0x80)
    }

    BitWriter bw(_data);
    bw.WriteBit(false);        // UPDATEFLAG_PLAY_HOVER_ANIM
    bw.WriteBit(false);        // UPDATEFLAG_SUPPRESSED_GREETINGS
    bw.WriteBit(false);        // UPDATEFLAG_ROTATION
    bw.WriteBit(false);        // UPDATEFLAG_ANIMKITS
    bw.WriteBit(false);        // UPDATEFLAG_HAS_TARGET
    bw.WriteBit(flags & 0x20); // UPDATEFLAG_SELF
    bw.WriteBit(false);        // UPDATEFLAG_VEHICLE
    bw.WriteBit(flags & 0x80); // UPDATEFLAG_LIVING
    bw.WriteBits(0, 24);       // PauseTimes size
    bw.WriteBit(false);        // UPDATEFLAG_NO_BIRTH_ANIM
    bw.WriteBit(false);        // UPDATEFLAG_GO_TRANSPORT_POSITION
    bw.WriteBit(false);        // UPDATEFLAG_STATIONARY_POSITION
    bw.WriteBit(false);        // UPDATEFLAG_AREATRIGGER
    bw.WriteBit(false);        // UPDATEFLAG_ENABLE_PORTALS
    bw.WriteBit(false);        // UPDATEFLAG_TRANSPORT

    if (flags & 0x80) { // LIVING bits (exact 15595 sequence)
      uint8 guidBytes[8];
      for (int i = 0; i < 8; ++i)
        guidBytes[i] = (guid >> (i * 8)) & 0xFF;
      bool isZeroOrientation = (move.orientation == 0.0f);

      bw.WriteBit(true); // !Has MoveFlags0 (using 0 for now)
      bw.WriteBit(isZeroOrientation);
      bw.WriteBit(guidBytes[7] != 0);
      bw.WriteBit(guidBytes[3] != 0);
      bw.WriteBit(guidBytes[2] != 0);

      bw.WriteBit(true);  // !Has player spline data
      bw.WriteBit(true);  // !Has pitch
      bw.WriteBit(false); // Has spline data
      bw.WriteBit(false); // Has fall data
      bw.WriteBit(true);  // !Has spline elevation
      bw.WriteBit(guidBytes[5] != 0);
      bw.WriteBit(false); // Has transport data
      bw.WriteBit(false); // !HasTime (we write 0, so HasTime is true)

      bw.WriteBit(guidBytes[4] != 0);
      bw.WriteBit(guidBytes[6] != 0);
      bw.WriteBit(guidBytes[0] != 0);
      bw.WriteBit(guidBytes[1] != 0);
      bw.WriteBit(false); // HeightChangeFailed
      bw.WriteBit(true);  // !Has MoveFlags1
    }

    bw.Flush();

    if (flags & 0x80) { // LIVING bytes
      uint8 guidBytes[8];
      for (int i = 0; i < 8; ++i)
        guidBytes[i] = (guid >> (i * 8)) & 0xFF;

      _data.WriteByteSeq(guidBytes[4]);
      _data.Append<float>(4.5f); // MOVE_RUN_BACK
      _data.Append<float>(4.5f); // MOVE_SWIM_BACK
      _data.Append<float>(move.z);
      _data.WriteByteSeq(guidBytes[5]);
      _data.Append<float>(move.x);
      _data.Append<float>(3.14159f); // MOVE_PITCH_RATE
      _data.WriteByteSeq(guidBytes[3]);
      _data.WriteByteSeq(guidBytes[0]);
      _data.Append<float>(4.72222f); // MOVE_SWIM
      _data.Append<float>(move.y);
      _data.WriteByteSeq(guidBytes[7]);
      _data.WriteByteSeq(guidBytes[1]);
      _data.WriteByteSeq(guidBytes[2]);
      _data.Append<float>(2.5f);                            // MOVE_WALK
      _data.Append<uint32>(static_cast<uint32>(move.time)); // GameTime
      _data.Append<float>(3.14159f);                        // MOVE_TURN_RATE
      _data.WriteByteSeq(guidBytes[6]);
      _data.Append<float>(7.0f); // MOVE_FLIGHT
      if (move.orientation != 0.0f)
        _data.Append<float>(move.orientation);
      _data.Append<float>(7.0f); // MOVE_RUN
      _data.Append<float>(4.5f); // MOVE_FLIGHT_BACK
    }

    // Update Fields
    uint32 maxField = 0;
    if (!fields.empty()) {
      maxField = fields.rbegin()->first + 1;
    }

    uint32 maskSize = (maxField + 31) / 32;
    _data.Append<uint8>(static_cast<uint8>(maskSize));

    if (maskSize > 0) {
      std::vector<uint32> mask(maskSize, 0);
      for (auto const &[index, value] : fields) {
        mask[index / 32] |= (1 << (index % 32));
      }

      for (uint32 m : mask)
        _data.Append<uint32>(m);

      for (uint32 i = 0; i < maxField; ++i) {
        if (mask[i / 32] & (1 << (i % 32))) {
          _data.Append<uint32>(fields.at(i));
        }
      }
    }
  }

  /**
   * @brief Builds the SMSG_UPDATE_OBJECT packet.
   * @param packet The WorldPacket to append the data to.
   */
  void Build(WorldPacket &packet) {
    packet.SetOpcode(SMSG_UPDATE_OBJECT);
    packet.Append<uint16>(_mapId);
    packet.Append<uint32>(_count);
    packet.Append(_data.GetBuffer(), _data.Size());
  }

  size_t GetBlockCount() const { return _count; }

private:
  uint32 _count;
  uint16 _mapId;
  ByteBuffer _data;
};

} // namespace Firelands

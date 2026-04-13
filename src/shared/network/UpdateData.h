#pragma once

#include <shared/network/ByteBuffer.h>
#include <shared/network/WorldPacket.h>
#include <shared/network/BitWriter.h>
#include <shared/network/MovementInfo.h>
#include <shared/network/UpdateFields.h>
#include <vector>
#include <map>

namespace Firelands {

    /**
     * @brief Accumulates object updates to be sent in a single SMSG_UPDATE_OBJECT packet.
     * Specific to Cataclysm 4.3.4 format.
     */
    class UpdateData {
    public:
        UpdateData() : _count(0) {}

        /**
         * @brief Adds a CreateObject block to the update data.
         * @param guid The GUID of the object being created.
         * @param typeId The TypeID of the object.
         * @param move Movement information.
         * @param fields Update fields.
         */
        void AddCreateObject(uint64 guid, TypeID typeId, MovementInfo const& move, std::map<uint16, uint32> const& fields) {
            _count++;
            _data.Append<uint8>(UPDATETYPE_CREATE_OBJECT);
            _data.WritePackedGuid(guid);
            _data.Append<uint8>(static_cast<uint8>(typeId));
            
            // Movement data bits
            BitWriter bw(_data);
            bw.WriteBit(true);  // Has movement data
            bw.WriteBit(false); // No transport
            bw.WriteBit(true);  // Is Self (always true for initial player update)
            bw.WriteBit(false); // Is Stationary
            bw.WriteBit(false); // Has AnimKits
            bw.Flush();

            // Movement Data Bytes
            _data.Append<uint32>(move.flags);
            _data.Append<uint16>(move.flags2);
            _data.Append<uint32>(move.time);
            _data.Append<float>(move.x);
            _data.Append<float>(move.y);
            _data.Append<float>(move.z);
            _data.Append<float>(move.orientation);
            _data.Append<uint32>(move.fallTime);

            // Bit writer for extra bits (Victim, Vehicle, etc)
            BitWriter extraBw(_data);
            extraBw.WriteBit(false); // Has Victim
            extraBw.WriteBit(false); // Has Vehicle
            extraBw.WriteBit(false); // Has PVP Guid
            extraBw.Flush();

            // Update Fields
            uint32 maxField = 0;
            if (!fields.empty()) {
                maxField = fields.rbegin()->first + 1;
            }
            
            uint32 maskSize = (maxField + 31) / 32;
            _data.Append<uint8>(static_cast<uint8>(maskSize));
            
            if (maskSize > 0) {
                std::vector<uint32> mask(maskSize, 0);
                for (auto const& [index, value] : fields) {
                    mask[index / 32] |= (1 << (index % 32));
                }
                
                for (uint32 m : mask) _data.Append<uint32>(m);

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
        void Build(WorldPacket& packet) {
            packet.SetOpcode(SMSG_UPDATE_OBJECT);
            packet.Append<uint32>(_count);
            packet.Append(_data.GetBuffer(), _data.Size());
        }

        size_t GetBlockCount() const { return _count; }

    private:
        uint32 _count;
        ByteBuffer _data;
    };

} // namespace Firelands

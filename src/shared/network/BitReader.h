#ifndef FIRELANDS_SHARED_NETWORK_BIT_READER_H
#define FIRELANDS_SHARED_NETWORK_BIT_READER_H

#include <shared/Common.h>
#include <shared/network/ByteBuffer.h>

namespace Firelands {

    class BitReader {
    public:
        explicit BitReader(ByteBuffer& buffer) : _buffer(buffer), _bitPos(8) {}

        bool ReadBit() {
            if (_bitPos >= 8) {
                _curByte = _buffer.Read<uint8>();
                _bitPos = 0;
            }
            return (_curByte >> (7 - _bitPos++)) & 1;
        }

        uint32 ReadBits(uint8 count) {
            uint32 res = 0;
            for (uint8 i = 0; i < count; ++i) {
                if (ReadBit()) {
                    res |= (1 << (count - i - 1));
                }
            }
            return res;
        }

        std::string ReadString(uint32 length) {
            std::string res;
            if (length == 0) return res;
            
            // Strings are byte-aligned after the bit-field header usually
            // But we must check if we need to skip the rest of the current byte
            // In WoW Cataclysm, strings in bit-packed packets start at the next byte boundary
            // if we just finished a bitfield.
            
            std::vector<uint8> bytes(length);
            _buffer.Read(bytes.data(), length);
            res.assign(bytes.begin(), bytes.end());
            return res;
        }

    private:
        ByteBuffer& _buffer;
        uint8 _curByte = 0;
        uint8 _bitPos = 8;
    };

} // namespace Firelands

#endif

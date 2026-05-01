#pragma once

// In-memory FOURCC chunk parser.
// WoW ADT, WDT, and WMO files are composed of 8-byte headers followed by a
// variable-length payload.  This reader wraps a raw byte buffer (typically
// from MpqStream) and allows iterating over those chunks by tag.
//
// Chunk header layout (little-endian):
//   uint32  tag      – four-character code, stored reversed in many files
//   uint32  size     – payload size in bytes (not counting the 8-byte header)

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace Firelands::VMap {

// Flip the byte order of a FOURCC tag.  WoW ADT chunks are stored with the
// tag reversed relative to how they appear in documentation, so "REVM" on
// disk == "MVER" logically.  Pass the raw 4-byte tag and compare to the
// logical constant.
inline uint32_t FlipFourcc(uint32_t tag) {
    return ((tag & 0xFF000000u) >> 24u) |
           ((tag & 0x00FF0000u) >>  8u) |
           ((tag & 0x0000FF00u) <<  8u) |
           ((tag & 0x000000FFu) << 24u);
}

// Produce a uint32 tag from a 4-char literal in the "logical" order
// (MVER, MHDR, …).  Usage: MakeTag("MVER")
inline constexpr uint32_t MakeTag(char const (&s)[5]) {
    return static_cast<uint32_t>(
        (static_cast<unsigned char>(s[0]) << 24u) |
        (static_cast<unsigned char>(s[1]) << 16u) |
        (static_cast<unsigned char>(s[2]) <<  8u) |
        (static_cast<unsigned char>(s[3])       ));
}

// ─────────────────────────── ChunkReader ─────────────────────────────────────

class ChunkReader {
public:
    // `data` must outlive this object.
    ChunkReader(const uint8_t* data, uint32_t size)
        : _base(data), _pos(0), _end(size) {}

    struct Chunk {
        uint32_t        tag{};   // logical (possibly flipped) tag
        uint32_t        size{};  // payload byte count
        const uint8_t*  data{};  // pointer into the source buffer
    };

    // Advance to the next chunk.  Returns false when the buffer is exhausted.
    bool Next(Chunk& out) {
        if (_pos + 8 > _end) return false;
        uint32_t raw_tag, size;
        std::memcpy(&raw_tag, _base + _pos,     4);
        std::memcpy(&size,    _base + _pos + 4, 4);
        if (_pos + 8 + size > _end) return false;
        out.tag  = FlipFourcc(raw_tag);
        out.size = size;
        out.data = _base + _pos + 8;
        _pos    += 8 + size;
        return true;
    }

    // Reset to the beginning of the buffer.
    void Reset() { _pos = 0; }

    // Current byte offset.
    uint32_t Pos()  const { return _pos; }
    uint32_t Size() const { return _end; }

private:
    const uint8_t* _base;
    uint32_t       _pos;
    uint32_t       _end;
};

} // namespace Firelands::VMap

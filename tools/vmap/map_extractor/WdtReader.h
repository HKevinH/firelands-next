#pragma once

// WDT (World Data Table) reader.
// Parses the WDT file for a given map and returns the 64×64 bitset of which
// ADT tiles actually exist for that map.
//
// WDT layout:
//   MVER (12 bytes) — version=18
//   MPHD (optional) — map flags
//   MAIN (8 + 64*64*8 bytes) — tile existence grid

#include "MapTypes.h"

#include <bitset>
#include <cstdint>

namespace Firelands::VMap::MapExtractor {

struct WdtTileGrid {
    // tile[y][x] == true if the ADT for that tile exists in the MPQ
    std::bitset<kWdtMapSize * kWdtMapSize> exists;

    bool TileExists(int y, int x) const {
        return exists[static_cast<size_t>(y) * kWdtMapSize + x];
    }
};

class WdtReader {
public:
    // Parse a WDT file from an in-memory buffer.
    // Returns false if the magic/version is wrong.
    static bool Parse(const uint8_t* buf, uint32_t size, WdtTileGrid& out);
};

} // namespace Firelands::VMap::MapExtractor

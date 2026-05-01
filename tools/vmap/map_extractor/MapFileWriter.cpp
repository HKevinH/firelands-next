#include "MapFileWriter.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>

namespace Firelands::VMap::MapExtractor {

// ─── static packing helpers ──────────────────────────────────────────────────

static float SelectUInt8Step(float diff)  { return 255.f  / diff; }
static float SelectUInt16Step(float diff) { return 65535.f / diff; }

// ─── Write ───────────────────────────────────────────────────────────────────

bool MapFileWriter::Write(const AdtGridData& grid,
                          const std::filesystem::path& outPath,
                          uint32_t build,
                          const MapWriteOptions& opts)
{
    // Working copies of height arrays (we may clamp them).
    float V9[kAdtGridSize + 1][kAdtGridSize + 1];
    float V8[kAdtGridSize][kAdtGridSize];
    std::memcpy(V9, grid.V9, sizeof(V9));
    std::memcpy(V8, grid.V8, sizeof(V8));

    // ── area data ────────────────────────────────────────────────────────────
    uint16_t area_ids[kAdtCellsPerGrid][kAdtCellsPerGrid];
    std::memcpy(area_ids, grid.area_ids, sizeof(area_ids));

    bool fullAreaData = false;
    uint16_t firstAreaId = area_ids[0][0];
    for (int y = 0; y < kAdtCellsPerGrid && !fullAreaData; ++y)
        for (int x = 0; x < kAdtCellsPerGrid; ++x)
            if (area_ids[y][x] != firstAreaId) { fullAreaData = true; break; }

    map_areaHeader areaHdr{};
    areaHdr.fourcc = kMapAreaMagic;
    if (fullAreaData)
        areaHdr.gridArea = 0;
    else {
        areaHdr.flags |= kMapAreaNoArea;
        areaHdr.gridArea = firstAreaId;
    }

    // ── height bounds ─────────────────────────────────────────────────────────
    float maxH = -20000.f, minH = 20000.f;
    for (int y = 0; y < kAdtGridSize; ++y)
        for (int x = 0; x < kAdtGridSize; ++x) { maxH = std::max(maxH, V8[y][x]); minH = std::min(minH, V8[y][x]); }
    for (int y = 0; y <= kAdtGridSize; ++y)
        for (int x = 0; x <= kAdtGridSize; ++x) { maxH = std::max(maxH, V9[y][x]); minH = std::min(minH, V9[y][x]); }

    // Clamp very-deep ocean tiles
    if (opts.allowHeightLimit && minH < opts.minHeight) {
        for (int y = 0; y < kAdtGridSize;  ++y) for (int x = 0; x < kAdtGridSize;  ++x) V8[y][x] = std::max(V8[y][x], opts.minHeight);
        for (int y = 0; y <= kAdtGridSize; ++y) for (int x = 0; x <= kAdtGridSize; ++x) V9[y][x] = std::max(V9[y][x], opts.minHeight);
        minH = std::max(minH, opts.minHeight);
        maxH = std::max(maxH, opts.minHeight);
    }

    // ── height header / packing ───────────────────────────────────────────────
    map_heightHeader heightHdr{};
    heightHdr.fourcc        = kMapHeightMagic;
    heightHdr.gridHeight    = minH;
    heightHdr.gridMaxHeight = maxH;

    if (maxH == minH || (opts.allowFloatToInt && (maxH - minH) < opts.flatHeightDelta))
        heightHdr.flags |= kMapHeightNoHeight;

    if (grid.hasFlightBox) {
        heightHdr.flags |= kMapHeightHasFlightBounds;
    }

    // Try int8 / int16 packing
    uint8_t  uint8_V9[kAdtGridSize + 1][kAdtGridSize + 1]{};
    uint8_t  uint8_V8[kAdtGridSize][kAdtGridSize]{};
    uint16_t uint16_V9[kAdtGridSize + 1][kAdtGridSize + 1]{};
    uint16_t uint16_V8[kAdtGridSize][kAdtGridSize]{};
    float    step = 0.f;

    if (!(heightHdr.flags & kMapHeightNoHeight) && opts.allowFloatToInt) {
        float diff = maxH - minH;
        if (diff < opts.floatToInt8Limit) {
            heightHdr.flags |= kMapHeightAsInt8;
            step = SelectUInt8Step(diff);
        } else if (diff < opts.floatToInt16Limit) {
            heightHdr.flags |= kMapHeightAsInt16;
            step = SelectUInt16Step(diff);
        }
    }

    if (heightHdr.flags & kMapHeightAsInt8) {
        for (int y = 0; y < kAdtGridSize;  ++y) for (int x = 0; x < kAdtGridSize;  ++x) uint8_V8[y][x] = static_cast<uint8_t>((V8[y][x] - minH) * step + 0.5f);
        for (int y = 0; y <= kAdtGridSize; ++y) for (int x = 0; x <= kAdtGridSize; ++x) uint8_V9[y][x] = static_cast<uint8_t>((V9[y][x] - minH) * step + 0.5f);
    } else if (heightHdr.flags & kMapHeightAsInt16) {
        for (int y = 0; y < kAdtGridSize;  ++y) for (int x = 0; x < kAdtGridSize;  ++x) uint16_V8[y][x] = static_cast<uint16_t>((V8[y][x] - minH) * step + 0.5f);
        for (int y = 0; y <= kAdtGridSize; ++y) for (int x = 0; x <= kAdtGridSize; ++x) uint16_V9[y][x] = static_cast<uint16_t>((V9[y][x] - minH) * step + 0.5f);
    }

    // ── liquid packing ────────────────────────────────────────────────────────
    float liquid_height[kAdtGridSize + 1][kAdtGridSize + 1];
    std::memcpy(liquid_height, grid.liquid_height, sizeof(liquid_height));

    uint16_t firstLiqType = grid.liquid_entry[0][0];
    uint8_t  firstLiqFlag = grid.liquid_flags[0][0];
    bool     fullType = false;
    for (int y = 0; y < kAdtCellsPerGrid && !fullType; ++y)
        for (int x = 0; x < kAdtCellsPerGrid; ++x)
            if (grid.liquid_entry[y][x] != firstLiqType || grid.liquid_flags[y][x] != firstLiqFlag) { fullType = true; break; }

    bool hasLiquid = !(firstLiqFlag == 0 && !fullType);
    map_liquidHeader liqHdr{};
    int liqMinX = 255, liqMinY = 255, liqMaxX = 0, liqMaxY = 0;
    float liqMinH = 20000.f, liqMaxH = -20000.f;

    if (hasLiquid) {
        for (int y = 0; y < kAdtGridSize; ++y) {
            for (int x = 0; x < kAdtGridSize; ++x) {
                if (grid.liquid_show[y][x]) {
                    liqMinX = std::min(liqMinX, x); liqMaxX = std::max(liqMaxX, x);
                    liqMinY = std::min(liqMinY, y); liqMaxY = std::max(liqMaxY, y);
                    float h = liquid_height[y][x];
                    liqMinH = std::min(liqMinH, h); liqMaxH = std::max(liqMaxH, h);
                } else {
                    liquid_height[y][x] = opts.minHeight;
                    liqMinH = std::min(liqMinH, opts.minHeight);
                }
            }
        }

        liqHdr.fourcc      = kMapLiquidMagic;
        liqHdr.offsetX     = static_cast<uint8_t>(liqMinX);
        liqHdr.offsetY     = static_cast<uint8_t>(liqMinY);
        liqHdr.width       = static_cast<uint8_t>(liqMaxX - liqMinX + 2);
        liqHdr.height      = static_cast<uint8_t>(liqMaxY - liqMinY + 2);
        liqHdr.liquidLevel = liqMinH;

        if (liqMaxH == liqMinH ||
            (opts.allowFloatToInt && (liqMaxH - liqMinH) < opts.flatLiquidDelta))
            liqHdr.flags |= kMapLiquidNoHeight;

        if (!fullType) {
            liqHdr.flags    |= kMapLiquidNoType;
            liqHdr.liquidFlags = firstLiqFlag;
            liqHdr.liquidType  = firstLiqType;
        }
    }

    // ── compute offsets ───────────────────────────────────────────────────────
    map_fileheader hdr{};
    hdr.mapMagic      = kMapMagic;
    hdr.versionMagic  = kMapVersionMagic;
    hdr.buildMagic    = build;

    hdr.areaMapOffset = sizeof(hdr);
    hdr.areaMapSize   = sizeof(areaHdr);
    if (fullAreaData) hdr.areaMapSize += sizeof(area_ids);

    hdr.heightMapOffset = hdr.areaMapOffset + hdr.areaMapSize;
    hdr.heightMapSize   = sizeof(heightHdr);
    if (grid.hasFlightBox)
        hdr.heightMapSize += sizeof(grid.flight_box_max) + sizeof(grid.flight_box_min);
    if (!(heightHdr.flags & kMapHeightNoHeight)) {
        if (heightHdr.flags & kMapHeightAsInt8)
            hdr.heightMapSize += sizeof(uint8_V9)  + sizeof(uint8_V8);
        else if (heightHdr.flags & kMapHeightAsInt16)
            hdr.heightMapSize += sizeof(uint16_V9) + sizeof(uint16_V8);
        else
            hdr.heightMapSize += sizeof(V9) + sizeof(V8);
    }

    if (hasLiquid) {
        hdr.liquidMapOffset = hdr.heightMapOffset + hdr.heightMapSize;
        hdr.liquidMapSize   = sizeof(liqHdr);
        if (!(liqHdr.flags & kMapLiquidNoType))
            hdr.liquidMapSize += sizeof(grid.liquid_entry) + sizeof(grid.liquid_flags);
        if (!(liqHdr.flags & kMapLiquidNoHeight))
            hdr.liquidMapSize += sizeof(float) * liqHdr.width * liqHdr.height;
    }

    if (grid.hasHoles) {
        hdr.holesOffset = hasLiquid ? hdr.liquidMapOffset + hdr.liquidMapSize
                                    : hdr.heightMapOffset + hdr.heightMapSize;
        hdr.holesSize   = sizeof(grid.holes);
    }

    // ── write ─────────────────────────────────────────────────────────────────
    std::ofstream out(outPath, std::ios::binary);
    if (!out) return false;

    out.write(reinterpret_cast<const char*>(&hdr),     sizeof(hdr));
    out.write(reinterpret_cast<const char*>(&areaHdr), sizeof(areaHdr));
    if (fullAreaData)
        out.write(reinterpret_cast<const char*>(area_ids), sizeof(area_ids));

    out.write(reinterpret_cast<const char*>(&heightHdr), sizeof(heightHdr));
    if (!(heightHdr.flags & kMapHeightNoHeight)) {
        if (heightHdr.flags & kMapHeightAsInt16) {
            out.write(reinterpret_cast<const char*>(uint16_V9), sizeof(uint16_V9));
            out.write(reinterpret_cast<const char*>(uint16_V8), sizeof(uint16_V8));
        } else if (heightHdr.flags & kMapHeightAsInt8) {
            out.write(reinterpret_cast<const char*>(uint8_V9), sizeof(uint8_V9));
            out.write(reinterpret_cast<const char*>(uint8_V8), sizeof(uint8_V8));
        } else {
            out.write(reinterpret_cast<const char*>(V9), sizeof(V9));
            out.write(reinterpret_cast<const char*>(V8), sizeof(V8));
        }
    }
    if (grid.hasFlightBox) {
        out.write(reinterpret_cast<const char*>(grid.flight_box_max), sizeof(grid.flight_box_max));
        out.write(reinterpret_cast<const char*>(grid.flight_box_min), sizeof(grid.flight_box_min));
    }

    if (hasLiquid) {
        out.write(reinterpret_cast<const char*>(&liqHdr), sizeof(liqHdr));
        if (!(liqHdr.flags & kMapLiquidNoType)) {
            out.write(reinterpret_cast<const char*>(grid.liquid_entry), sizeof(grid.liquid_entry));
            out.write(reinterpret_cast<const char*>(grid.liquid_flags), sizeof(grid.liquid_flags));
        }
        if (!(liqHdr.flags & kMapLiquidNoHeight)) {
            for (int y = 0; y < liqHdr.height; ++y)
                out.write(reinterpret_cast<const char*>(&liquid_height[y + liqHdr.offsetY][liqHdr.offsetX]),
                          sizeof(float) * liqHdr.width);
        }
    }

    if (grid.hasHoles)
        out.write(reinterpret_cast<const char*>(grid.holes), sizeof(grid.holes));

    return out.good();
}

// ─── WriteTileList ────────────────────────────────────────────────────────────

bool MapFileWriter::WriteTileList(const std::filesystem::path& outPath,
                                  const std::bitset<kWdtMapSize * kWdtMapSize>& existingTiles,
                                  uint32_t build)
{
    FILE* f = std::fopen(outPath.string().c_str(), "wb");
    if (!f) return false;

    // Header: "MAPS" (4) + MAP_VERSION_MAGIC (4) + build (4)
    uint32_t magic   = kMapMagic;
    uint32_t version = kMapVersionMagic;
    std::fwrite(&magic,   sizeof(magic),   1, f);
    std::fwrite(&version, sizeof(version), 1, f);
    std::fwrite(&build,   sizeof(build),   1, f);

    // Tile bitset as ASCII '0'/'1' string (matches reference)
    std::string bits = existingTiles.to_string();
    std::fwrite(bits.c_str(), 1, bits.size(), f);
    std::fclose(f);
    return true;
}

} // namespace Firelands::VMap::MapExtractor

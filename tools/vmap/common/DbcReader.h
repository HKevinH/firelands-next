#pragma once

// WDBC (DBC) reader for the vmap pipeline.
// Loads an entire DBC file into memory and exposes row/field access.
// All fields are fixed 4-byte (float / uint32 / int32); strings are stored in
// a separate string table appended after the record data.
//
// WDBC header layout (all little-endian uint32):
//   [0]  magic        "WDBC"
//   [4]  recordCount
//   [8]  fieldCount
//   [12] recordSize   (must equal fieldCount * 4)
//   [16] stringSize

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "StormLib.h"

namespace Firelands::VMap {

class DbcReader {
public:
    // Load from an MPQ archive.
    explicit DbcReader(HANDLE mpq, const char* filename);
    // Load from a raw file on disk (used by vmap4_assembler's DBC reads).
    explicit DbcReader(const char* filepath);

    ~DbcReader() = default;

    DbcReader(DbcReader const&)            = delete;
    DbcReader& operator=(DbcReader const&) = delete;

    bool IsOpen() const { return _open; }
    const std::string& ErrorMessage() const { return _error; }

    uint32_t RecordCount() const { return _recordCount; }
    uint32_t FieldCount()  const { return _fieldCount;  }

    // Field accessors (0-based column index).
    uint32_t    GetUInt  (uint32_t row, uint32_t col) const;
    int32_t     GetInt   (uint32_t row, uint32_t col) const;
    float       GetFloat (uint32_t row, uint32_t col) const;
    const char* GetString(uint32_t row, uint32_t col) const;

private:
    bool ParseBuffer(const uint8_t* buf, uint32_t size);

    std::unique_ptr<uint8_t[]> _data;
    const uint8_t*             _records{};
    const char*                _strings{};
    uint32_t                   _recordCount{};
    uint32_t                   _fieldCount{};
    uint32_t                   _recordSize{};
    uint32_t                   _stringSize{};
    bool                       _open{false};
    std::string                _error;
};

} // namespace Firelands::VMap

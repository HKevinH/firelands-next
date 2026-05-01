#include "DbcReader.h"

#include <cassert>
#include <cstdio>

namespace Firelands::VMap {

static constexpr uint32_t kWdbcMagic = 0x43424457u; // "WDBC" as little-endian uint32

// ─── helpers ─────────────────────────────────────────────────────────────────

static bool LoadFileToBuffer(const char* filepath,
                             std::unique_ptr<uint8_t[]>& out,
                             uint32_t& size,
                             std::string& error)
{
    FILE* f = std::fopen(filepath, "rb");
    if (!f) {
        error = std::string("Cannot open file '") + filepath + "'";
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    long len = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (len <= 0) {
        error = std::string("File '") + filepath + "' is empty or unreadable";
        std::fclose(f);
        return false;
    }
    size = static_cast<uint32_t>(len);
    out  = std::make_unique<uint8_t[]>(size);
    if (std::fread(out.get(), 1, size, f) != size) {
        error = std::string("Read error for '") + filepath + "'";
        std::fclose(f);
        return false;
    }
    std::fclose(f);
    return true;
}

static bool LoadMpqFileToBuffer(HANDLE mpq,
                                const char* filename,
                                std::unique_ptr<uint8_t[]>& out,
                                uint32_t& size,
                                std::string& error)
{
    HANDLE file = nullptr;
    if (!SFileOpenFileEx(mpq, filename, SFILE_OPEN_FROM_MPQ, &file)) {
        error = std::string("Cannot open '") + filename + "' from MPQ (err=" +
                std::to_string(GetLastError()) + ")";
        return false;
    }
    DWORD hi = 0;
    DWORD lo = SFileGetFileSize(file, &hi);
    if (hi != 0 || lo == 0) {
        error = std::string("Invalid file size for '") + filename + "'";
        SFileCloseFile(file);
        return false;
    }
    size = lo;
    out  = std::make_unique<uint8_t[]>(lo);
    DWORD read = 0;
    if (!SFileReadFile(file, out.get(), lo, &read, nullptr) || read != lo) {
        error = std::string("Read error for '") + filename + "'";
        SFileCloseFile(file);
        return false;
    }
    SFileCloseFile(file);
    return true;
}

// ─── constructors ────────────────────────────────────────────────────────────

DbcReader::DbcReader(HANDLE mpq, const char* filename) {
    uint32_t size{};
    if (!LoadMpqFileToBuffer(mpq, filename, _data, size, _error)) return;
    _open = ParseBuffer(_data.get(), size);
}

DbcReader::DbcReader(const char* filepath) {
    uint32_t size{};
    if (!LoadFileToBuffer(filepath, _data, size, _error)) return;
    _open = ParseBuffer(_data.get(), size);
}

// ─── ParseBuffer ─────────────────────────────────────────────────────────────

bool DbcReader::ParseBuffer(const uint8_t* buf, uint32_t size) {
    if (size < 20) {
        _error = "Buffer too small to be a valid DBC file";
        return false;
    }

    uint32_t magic;
    std::memcpy(&magic, buf, 4);
    if (magic != kWdbcMagic) {
        _error = "File is not a valid WDBC (bad magic)";
        return false;
    }

    std::memcpy(&_recordCount, buf + 4,  4);
    std::memcpy(&_fieldCount,  buf + 8,  4);
    std::memcpy(&_recordSize,  buf + 12, 4);
    std::memcpy(&_stringSize,  buf + 16, 4);

    if (_recordSize != _fieldCount * 4) {
        _error = "DBC recordSize != fieldCount * 4";
        return false;
    }

    uint32_t expectedSize = 20 + _recordCount * _recordSize + _stringSize;
    if (size < expectedSize) {
        _error = "DBC buffer too small for declared records + string table";
        return false;
    }

    _records = buf + 20;
    _strings = reinterpret_cast<const char*>(buf + 20 + _recordCount * _recordSize);
    return true;
}

// ─── accessors ───────────────────────────────────────────────────────────────

uint32_t DbcReader::GetUInt(uint32_t row, uint32_t col) const {
    assert(row < _recordCount && col < _fieldCount);
    uint32_t v;
    std::memcpy(&v, _records + row * _recordSize + col * 4, 4);
    return v;
}

int32_t DbcReader::GetInt(uint32_t row, uint32_t col) const {
    assert(row < _recordCount && col < _fieldCount);
    int32_t v;
    std::memcpy(&v, _records + row * _recordSize + col * 4, 4);
    return v;
}

float DbcReader::GetFloat(uint32_t row, uint32_t col) const {
    assert(row < _recordCount && col < _fieldCount);
    float v;
    std::memcpy(&v, _records + row * _recordSize + col * 4, 4);
    return v;
}

const char* DbcReader::GetString(uint32_t row, uint32_t col) const {
    assert(row < _recordCount && col < _fieldCount);
    uint32_t offset = GetUInt(row, col);
    assert(offset < _stringSize);
    return _strings + offset;
}

} // namespace Firelands::VMap

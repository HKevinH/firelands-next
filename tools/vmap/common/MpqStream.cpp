#include "MpqStream.h"

#include <cstring>

namespace Firelands::VMap {

MpqStream::MpqStream(HANDLE mpq, const char* filename, bool warnIfMissing) {
    HANDLE file = nullptr;
    if (!SFileOpenFileEx(mpq, filename, SFILE_OPEN_FROM_MPQ, &file)) {
        if (warnIfMissing) {
            _error = std::string("Can't open '") + filename + "' (StormLib err=" +
                     std::to_string(GetLastError()) + ")";
        }
        return;
    }

    DWORD hi = 0;
    DWORD lo = SFileGetFileSize(file, &hi);
    if (hi != 0) {
        _error = std::string("File '") + filename + "' is too large (size_hi=" +
                 std::to_string(hi) + ")";
        SFileCloseFile(file);
        return;
    }
    if (lo == 0) {
        _error = std::string("File '") + filename + "' is empty";
        SFileCloseFile(file);
        return;
    }

    _buf  = std::make_unique<uint8_t[]>(lo);
    _size = lo;

    DWORD read = 0;
    if (!SFileReadFile(file, _buf.get(), lo, &read, nullptr) || read != lo) {
        _error = std::string("Failed to read '") + filename + "' (size=" +
                 std::to_string(lo) + " read=" + std::to_string(read) + ")";
        _buf.reset();
        _size = 0;
        SFileCloseFile(file);
        return;
    }

    SFileCloseFile(file);
    _eof = false;
}

uint32_t MpqStream::Read(void* dest, uint32_t bytes) {
    if (_eof || !_buf) return 0;
    uint32_t available = _size - _pos;
    if (bytes > available) {
        bytes = available;
        _eof  = true;
    }
    std::memcpy(dest, _buf.get() + _pos, bytes);
    _pos += bytes;
    return bytes;
}

void MpqStream::Seek(uint32_t offset) {
    _pos = offset;
    _eof = (_pos >= _size);
}

void MpqStream::SeekRelative(int32_t offset) {
    _pos = static_cast<uint32_t>(static_cast<int64_t>(_pos) + offset);
    _eof = (_pos >= _size);
}

} // namespace Firelands::VMap

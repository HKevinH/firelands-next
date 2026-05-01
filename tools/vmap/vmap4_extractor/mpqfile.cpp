#include "mpqfile.h"

#include <cstring>
#include <cstdio>

MPQFile::MPQFile(HANDLE mpq, const char* filename, bool warnNoExist) {
    HANDLE file = nullptr;
    if (!SFileOpenFileEx(mpq, filename, SFILE_OPEN_FROM_MPQ, &file)) {
        if (warnNoExist || GetLastError() != ERROR_FILE_NOT_FOUND)
            std::fprintf(stderr, "Can't open %s, err=%u!\n", filename,
                         static_cast<unsigned>(GetLastError()));
        eof_ = true;
        return;
    }

    DWORD hi = 0;
    DWORD lo = SFileGetFileSize(file, &hi);
    if (hi != 0) {
        std::fprintf(stderr, "Can't open %s, size[hi] = %u!\n", filename, hi);
        SFileCloseFile(file);
        eof_ = true;
        return;
    }
    if (lo <= 1) {
        std::fprintf(stderr, "Can't open %s, size = %u!\n", filename, lo);
        SFileCloseFile(file);
        eof_ = true;
        return;
    }

    buffer_ = new char[lo];
    size_   = lo;
    DWORD read = 0;
    if (!SFileReadFile(file, buffer_, lo, &read, nullptr) || read != lo) {
        std::fprintf(stderr, "Can't read %s, size=%u read=%u!\n", filename, lo, read);
        SFileCloseFile(file);
        delete[] buffer_;
        buffer_ = nullptr;
        eof_    = true;
        return;
    }
    SFileCloseFile(file);
    eof_ = false;
}

size_t MPQFile::read(void* dest, size_t bytes) {
    if (eof_ || !buffer_) return 0;
    size_t rpos = pos_ + bytes;
    if (rpos > size_) {
        bytes = size_ - pos_;
        eof_    = true;
    }
    std::memcpy(dest, buffer_ + pos_, bytes);
    pos_ = rpos;
    return bytes;
}

void MPQFile::seek(int offset) {
    pos_ = static_cast<size_t>(offset);
    eof_ = (pos_ >= size_);
}

void MPQFile::seekRelative(int offset) {
    pos_ = static_cast<size_t>(static_cast<long long>(pos_) + offset);
    eof_ = (pos_ >= size_);
}

void MPQFile::close() {
    delete[] buffer_;
    buffer_ = nullptr;
    eof_    = true;
}

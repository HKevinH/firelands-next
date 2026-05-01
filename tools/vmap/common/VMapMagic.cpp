#include "VMapMagic.h"

#include <cstring>

namespace Firelands::VMap {

bool ReadAndValidateChunk(FILE* rf, const char* expected, uint32_t len) {
    char buf[16] = {};
    if (len > sizeof(buf)) {
        return false;
    }
    if (fread(buf, 1, len, rf) != len) {
        return false;
    }
    return std::memcmp(buf, expected, len) == 0;
}

} // namespace Firelands::VMap

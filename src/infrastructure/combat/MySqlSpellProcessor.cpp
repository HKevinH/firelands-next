#include "MySqlSpellProcessor.h"

namespace infrastructure {
    bool MySqlSpellProcessor::CanCast(uint64_t spellId, uint64_t targetGuid) {
        return true;
    }
    void MySqlSpellProcessor::ExecuteCast(uint64_t spellId, uint64_t targetGuid) {
    }
}

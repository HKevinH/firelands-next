#pragma once
#include <cstdint>

namespace combat {
    class ISpellProcessor {
    public:
        virtual ~ISpellProcessor() = default;
        virtual bool CanCast(uint64_t spellId, uint64_t targetGuid) = 0;
        virtual void ExecuteCast(uint64_t spellId, uint64_t targetGuid) = 0;
    };
}

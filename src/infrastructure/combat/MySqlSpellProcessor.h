#pragma once
#include "../../domain/combat/repositories/ISpellProcessor.h"

namespace infrastructure {
    class MySqlSpellProcessor : public combat::ISpellProcessor {
    public:
        bool CanCast(uint64_t spellId, uint64_t targetGuid) override;
        void ExecuteCast(uint64_t spellId, uint64_t targetGuid) override;
    };
}

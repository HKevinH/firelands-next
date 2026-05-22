#pragma once
#include <cstdint>
#include <string>

namespace combat {
    class ICombatEntity {
    public:
        virtual ~ICombatEntity() = default;
        virtual uint64_t GetGuid() const = 0;
        virtual bool IsAlive() const = 0;
        virtual void TakeDamage(float amount) = 0;
    };
}

#include "DamageCalculator.h"

namespace combat {
    float DamageCalculator::Calculate(float baseDamage, float modifier) {
        return baseDamage * modifier;
    }
}

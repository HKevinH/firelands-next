#pragma once

#include <application/spell/SpellManager.h>
#include <chrono>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace Firelands {

class ISpellCastTables;
class ISpellDefinitionStore;

/// Known spellbook ids that are passive combat auras (excludes language, mount, form).
std::vector<uint32_t> CollectLoginPassiveSpellIds(
    std::unordered_set<uint32_t> const &knownSpellIds,
    ISpellDefinitionStore const *spellDefinitions);

/// Builds `SpellCastOutcome` aura applies for passive spells (one outcome per apply-aura row).
std::vector<SpellCastOutcome> BuildPassiveAuraOutcomes(
    uint64_t unitGuid, uint8_t casterLevel,
    std::vector<uint32_t> const &candidateSpellIds,
    ISpellDefinitionStore const *spellDefinitions,
    ISpellCastTables const *castTables,
    std::chrono::steady_clock::time_point now);

} // namespace Firelands

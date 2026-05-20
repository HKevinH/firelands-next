#pragma once

#include <domain/models/PlayerCreateInfo.h>
#include <domain/repositories/ISpellDefinitionStore.h>
#include <cstdint>
#include <vector>

namespace Firelands {

class PlayerCreateInfoService;

/// Builds known spell lists and starter skills (race/class/level + world DB).
namespace PlayerSpellbook {

std::vector<uint32_t> BuildKnownSpells(
    uint8_t race, uint8_t klass, uint8_t level,
    PlayerCreateInfoService const &createInfo,
    ISpellDefinitionStore const *spellDefinitions,
    std::vector<uint32_t> const &extraSpellIdsFromCharacter);

std::vector<StarterSkillGrant> BuildStarterSkills(
    uint8_t race, uint8_t klass, PlayerCreateInfoService const &createInfo);

} // namespace PlayerSpellbook

} // namespace Firelands

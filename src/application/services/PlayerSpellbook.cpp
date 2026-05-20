#include <application/services/PlayerSpellbook.h>
#include <application/services/PlayerCreateInfoService.h>
#include <shared/game/ChatLanguages.h>
#include <shared/Logger.h>

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace Firelands {
namespace PlayerSpellbook {

namespace {

void PushUnique(std::vector<uint32_t> &out, uint32_t sid) {
  if (sid == 0u)
    return;
  if (std::find(out.begin(), out.end(), sid) == out.end())
    out.push_back(sid);
}

bool SpellAllowedAtLevel(ISpellDefinitionStore const *store, uint32_t spellId,
                         uint8_t level) {
  if (!store)
    return true;
  auto def = store->GetDefinition(spellId);
  if (!def)
    return true;
  if (def->requiredLevel == 0u)
    return true;
  return def->requiredLevel <= level;
}

void StripWrongEraSpellIds(std::vector<uint32_t> &spells) {
  for (auto it = spells.begin(); it != spells.end();) {
    if (*it >= 86450u && *it <= 86550u)
      it = spells.erase(it);
    else
      ++it;
  }
}

void FilterBySpellDbc(ISpellDefinitionStore const *store, uint8_t race,
                      uint8_t klass, std::vector<uint32_t> &spells) {
  if (!store)
    return;
  for (auto it = spells.begin(); it != spells.end();) {
    uint32_t const sid = *it;
    if (!IsLanguagePassiveSpell(sid) && !store->HasSpell(sid)) {
      LOG_WARN(
          "Spell id {} not in Spell.dbc; omitted from known spells (race={} "
          "class={})",
          sid, static_cast<uint32_t>(race), static_cast<uint32_t>(klass));
      it = spells.erase(it);
    } else
      ++it;
  }
}

} // namespace

std::vector<uint32_t> BuildKnownSpells(
    uint8_t race, uint8_t klass, uint8_t level,
    PlayerCreateInfoService const &createInfo,
    ISpellDefinitionStore const *spellDefinitions,
    std::vector<uint32_t> const &extraSpellIdsFromCharacter) {
  std::vector<uint32_t> spells;
  AppendRacialLanguageSpells(race, spells);

  std::vector<uint32_t> const starterSpells =
      createInfo.GetStarterSpells(race, klass);
  if (starterSpells.empty()) {
    LOG_WARN(
        "No starter spells for race={} class={} (ensure "
        "playercreateinfo_spell has data or DBC files are present).",
        static_cast<uint32_t>(race), static_cast<uint32_t>(klass));
  } else {
    spells.reserve(spells.size() + starterSpells.size());
    for (uint32_t sid : starterSpells) {
      if (!SpellAllowedAtLevel(spellDefinitions, sid, level))
        continue;
      PushUnique(spells, sid);
    }
  }

  StripWrongEraSpellIds(spells);
  FilterBySpellDbc(spellDefinitions, race, klass, spells);

  for (uint32_t sid : extraSpellIdsFromCharacter) {
    if (!SpellAllowedAtLevel(spellDefinitions, sid, level))
      continue;
    if (spellDefinitions && !IsLanguagePassiveSpell(sid) &&
        !spellDefinitions->HasSpell(sid))
      continue;
    PushUnique(spells, sid);
  }

  EnsureRacialLanguageSpells(race, spells);
  PrioritizeDefaultLanguageSpell(race, spells);
  return spells;
}

std::vector<StarterSkillGrant> BuildStarterSkills(
    uint8_t race, uint8_t klass,
    PlayerCreateInfoService const &createInfo) {
  std::vector<StarterSkillGrant> out;
  std::unordered_set<uint32_t> seen;
  auto pushGrant = [&](StarterSkillGrant g) {
    if (g.skillId == 0u || !seen.insert(g.skillId).second)
      return;
    if (g.maxRank == 0u)
      g.maxRank = g.rank > 0 ? g.rank : 300;
    if (g.rank == 0u)
      g.rank = 1;
    out.push_back(g);
  };

  std::vector<uint32_t> langSkills;
  AppendRacialLanguageSkills(race, langSkills);
  for (uint32_t skillId : langSkills) {
    StarterSkillGrant g;
    g.skillId = skillId;
    g.rank = 300;
    g.maxRank = 300;
    pushGrant(g);
  }
  for (StarterSkillGrant const &g : createInfo.GetStarterSkills(race, klass))
    pushGrant(g);
  return out;
}

} // namespace PlayerSpellbook
} // namespace Firelands

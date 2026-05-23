#pragma once

#include <shared/Common.h>

#include <unordered_set>
#include <vector>

namespace Firelands {

/// Passive "Language *" spells (build 15595 / 4.3.4) so the client spell book
/// matches retail and chat uses the correct `Language` ids.
/// Reference: ChrRaces trade language + racial language
/// `HandleMessagechatOpcode` language skill checks (`LanguageDesc::skill_id`).
void AppendRacialLanguageSpells(uint8 race, std::vector<uint32> &knownSpells);

/// Returns 0 if there is no default racial spell for that `Language` id.
uint32 LanguageSpellIdForLang(uint32 lang);

/// True if `knownSpells` contains the passive spell that grants `lang`.
bool PlayerKnowsLanguage(std::vector<uint32> const &knownSpells, uint32 lang);
/// Same as the vector overload; preferred when ids are stored in a set (O(1)).
bool PlayerKnowsLanguage(std::unordered_set<uint32> const &knownSpellIds,
                         uint32 lang);
bool PlayerKnowsLanguage(std::unordered_set<uint32> const &knownSpellIds,
                         uint32 lang, uint8 race);

/// Faction trade language (Common for Alliance, Orcish for Horde).
uint32 DefaultLanguageForRace(uint8 race);

/// Primary racial language (`GetLanguageByIndex(1)` on the 4.3.4 client).
uint32 PrimaryLanguageForRace(uint8 race);

/// SkillLine.dbc id for a `Language` id, or 0 when that language has no known
/// skill row in 4.3.4.
uint32 LanguageSkillIdForLang(uint32 lang);

/// Idempotent: appends every language skill that should be visible for `race`.
void AppendRacialLanguageSkills(uint8 race, std::vector<uint32> &skillIds);

/// Passive "Language *" spell ids used by this module (4.3.4). Used to avoid
/// stripping them when validating against `Spell.dbc`.
bool IsLanguagePassiveSpell(uint32 spellId);

/// Idempotent: ensures every racial language passive for `race` is present.
void EnsureRacialLanguageSpells(uint8 race, std::vector<uint32> &spellIds);

/// Moves the default `/say` language passive to the front of the list.
void PrioritizeDefaultLanguageSpell(uint8 race, std::vector<uint32> &spellIds);

/// True when `CHAT_LANG_ADDON` (-1) is valid for this `ChatMsg` type (party, guild, …).
bool IsAddonChatLanguageAllowed(uint32 chatType);

/// Maps client language id (including `-1` / universal) to a speakable language for
/// `SMSG_MESSAGECHAT`. Never returns `LANG_UNIVERSAL` or `CHAT_LANG_ADDON`.
uint32 NormalizePlayerChatLanguage(uint32 requestedLang, uint32 chatType, uint8 race,
                                   std::unordered_set<uint32> const &knownSpellIds);

} // namespace Firelands

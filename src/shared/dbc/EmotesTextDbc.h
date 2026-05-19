#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace Firelands {

/// `EmotesText.dbc` (4.3.4.15595): maps text-emote command id → animation id on wire.
class EmotesTextDbc {
public:
  bool Load(std::string const &path);

  bool IsLoaded() const { return m_loaded; }

  /// Returns `EmoteID` column for a text-emote id, or empty when unknown / DBC missing.
  std::optional<uint32_t> LookupEmoteAnim(uint32_t textEmoteId) const;

private:
  bool m_loaded = false;
  std::unordered_map<uint32_t, uint32_t> m_textEmoteToAnim;
};

} // namespace Firelands

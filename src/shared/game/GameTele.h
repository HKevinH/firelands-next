#pragma once

#include <shared/Common.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>

namespace Firelands {

/// One named teleport destination (mirrors the `game_tele` world DB table).
struct GameTele {
  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  float o = 0.f;
  uint32 mapId = 0;
  std::string name; ///< original (display) name
};

/// In-memory lookup of named teleports, loaded once from `game_tele`. Names are
/// matched case-insensitively, with a prefix fallback so `.tele storm` resolves
/// `Stormwind`.
class GameTeleStore {
public:
  void Add(GameTele tele) {
    std::string key = ToLower(tele.name);
    m_byName.emplace(std::move(key), std::move(tele));
  }

  GameTele const *Find(std::string const &name) const {
    std::string const key = ToLower(name);
    if (auto it = m_byName.find(key); it != m_byName.end())
      return &it->second;
    // Prefix fallback: first entry whose name starts with the query.
    for (auto const &[k, tele] : m_byName)
      if (k.size() >= key.size() && k.compare(0, key.size(), key) == 0)
        return &tele;
    return nullptr;
  }

  size_t Size() const { return m_byName.size(); }
  bool Empty() const { return m_byName.empty(); }

private:
  static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
  }

  std::unordered_map<std::string, GameTele> m_byName;
};

} // namespace Firelands

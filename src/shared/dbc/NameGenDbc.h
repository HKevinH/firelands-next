#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Firelands {

/// `NameGen.dbc` — random character names per race and gender (build 15595).
class NameGenDbc {
public:
  bool Load(std::string const &path);

  bool IsLoaded() const { return m_loaded; }

  /// `gender` matches character creation: 0 = male, 1 = female.
  std::optional<std::string> PickRandomName(uint8_t race, uint8_t gender) const;

private:
  using NameList = std::vector<std::string>;

  static uint64_t Key(uint8_t race, uint8_t gender) {
    return (static_cast<uint64_t>(race) << 8) | gender;
  }

  bool m_loaded = false;
  std::unordered_map<uint64_t, NameList> m_namesByRaceGender;
};

} // namespace Firelands

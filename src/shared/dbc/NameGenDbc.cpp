#include <shared/dbc/NameGenDbc.h>
#include <shared/dbc/DbcReader.h>
#include <shared/Logger.h>

#include <random>
#include <string_view>

namespace Firelands {

namespace {

constexpr std::string_view kNameGenFmt = "nsxx";

bool IsPlayableRace(uint8_t race) {
  return (race >= 1 && race <= 11) || race == 22;
}

} // namespace

bool NameGenDbc::Load(std::string const &path) {
  m_loaded = false;
  m_namesByRaceGender.clear();

  DbcReader reader;
  if (!reader.Load(path))
    return false;

  if (!reader.VerifyFormat(kNameGenFmt) || reader.GetRecordSize() != 16u) {
    LOG_WARN("NameGen.dbc: unexpected layout (fields={}, recordSize={}) in {}",
             reader.GetFieldCount(), reader.GetRecordSize(), path);
    return false;
  }

  std::vector<uint32_t> const offsets = DbcBuildFieldByteOffsets(kNameGenFmt);
  uint32_t const recordCount = reader.GetRecordCount();
  for (uint32_t rec = 0; rec < recordCount; ++rec) {
    uint32_t const stringOffset = reader.ReadUInt32(rec, 1, offsets);
    uint32_t const race = reader.ReadUInt32(rec, 2, offsets);
    uint32_t const sex = reader.ReadUInt32(rec, 3, offsets);
    if (race == 0 || race > 128u || sex > 1u)
      continue;

    std::string name = reader.ReadStringAtOffset(stringOffset);
    if (name.empty())
      continue;

    m_namesByRaceGender[Key(static_cast<uint8_t>(race), static_cast<uint8_t>(sex))]
        .push_back(std::move(name));
  }

  if (m_namesByRaceGender.empty()) {
    LOG_WARN("NameGen.dbc: no usable name rows in {}", path);
    return false;
  }

  m_loaded = true;
  LOG_INFO("NameGen.dbc: loaded {} race/gender name lists from {}", m_namesByRaceGender.size(),
           path);
  return true;
}

std::optional<std::string> NameGenDbc::PickRandomName(uint8_t race,
                                                      uint8_t gender) const {
  if (!m_loaded || !IsPlayableRace(race) || gender > 1u)
    return std::nullopt;

  auto it = m_namesByRaceGender.find(Key(race, gender));
  if (it == m_namesByRaceGender.end() || it->second.empty())
    return std::nullopt;

  NameList const &names = it->second;
  static thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<size_t> dist(0, names.size() - 1);
  return names[dist(rng)];
}

} // namespace Firelands

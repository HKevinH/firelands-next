#include <shared/dbc/GtOctCombatDbc.h>
#include <shared/Logger.h>
#include <fstream>
#include <string>
#include <vector>

namespace Firelands {

namespace {

bool LoadGtTable(DbcReader &out, bool &flag, std::string const &path) {
  if (!out.Load(path)) {
    flag = false;
    return false;
  }
  if (out.GetRecordCount() == 0 || out.GetRecordSize() < 4u) {
    LOG_WARN(
        "GtOctCombatDbc: invalid record layout in {} (records={}, recordSize={})",
        path, out.GetRecordCount(), out.GetRecordSize());
    flag = false;
    return false;
  }
  flag = true;
  return true;
}

bool TryLoadFirstExisting(DbcReader &out, bool &flag,
                          std::vector<std::string> const &paths) {
  flag = false;
  for (std::string const &p : paths) {
    std::ifstream probe(p);
    if (!probe.good())
      continue;
    if (LoadGtTable(out, flag, p))
      return true;
  }
  return false;
}

} // namespace

bool GtOctCombatDbc::Load(std::string const &dbcDirectory) {
  m_baseHpLoaded = m_baseMpLoaded = m_hpPerStaLoaded = m_mpPerIntLoaded = false;
  if (dbcDirectory.empty())
    return false;
  std::string const base =
      dbcDirectory.back() == '/' ? dbcDirectory : (dbcDirectory + "/");
  LoadGtTable(m_baseHp, m_baseHpLoaded, base + "gtOCTBaseHPByClass.dbc");
  LoadGtTable(m_baseMp, m_baseMpLoaded, base + "gtOCTBaseMPByClass.dbc");
  LoadGtTable(m_hpPerSta, m_hpPerStaLoaded, base + "gtOCTHpPerStamina.dbc");
  // Some extracts only ship one spelling; try common variants.
  TryLoadFirstExisting(m_mpPerInt, m_mpPerIntLoaded,
                       {base + "gtOCTMpPerIntellect.dbc",
                        base + "gtOCTMPPerIntellect.dbc"});
  return m_baseHpLoaded || m_baseMpLoaded || m_hpPerStaLoaded ||
         m_mpPerIntLoaded;
}

uint32_t GtOctCombatDbc::ClassLevelIndex(uint8_t klass, uint8_t level) {
  if (klass == 0 || level == 0)
    return 0;
  uint32_t const li = static_cast<uint32_t>(level) - 1u;
  if (li >= kGtMaxLevel)
    return (static_cast<uint32_t>(klass) - 1u) * kGtMaxLevel + (kGtMaxLevel - 1u);
  return (static_cast<uint32_t>(klass) - 1u) * kGtMaxLevel + li;
}

uint32_t GtOctCombatDbc::LevelIndex(uint8_t level) {
  if (level == 0)
    return 0;
  uint32_t const li = static_cast<uint32_t>(level) - 1u;
  return li >= kGtMaxLevel ? kGtMaxLevel - 1u : li;
}

float GtOctCombatDbc::BaseHpByClassLevel(uint8_t klass, uint8_t level) const {
  if (!m_baseHpLoaded)
    return 0.f;
  uint32_t const idx = ClassLevelIndex(klass, level);
  if (idx >= m_baseHp.GetRecordCount())
    return 0.f;
  return m_baseHp.ReadFirstFloatInRecord(idx);
}

float GtOctCombatDbc::BaseMpByClassLevel(uint8_t klass, uint8_t level) const {
  if (!m_baseMpLoaded)
    return 0.f;
  uint32_t const idx = ClassLevelIndex(klass, level);
  if (idx >= m_baseMp.GetRecordCount())
    return 0.f;
  return m_baseMp.ReadFirstFloatInRecord(idx);
}

float GtOctCombatDbc::HpPerStaminaAtPlayerLevel(uint8_t level) const {
  if (!m_hpPerStaLoaded)
    return 0.f;
  uint32_t const idx = LevelIndex(level);
  if (idx >= m_hpPerSta.GetRecordCount())
    return 0.f;
  return m_hpPerSta.ReadFirstFloatInRecord(idx);
}

float GtOctCombatDbc::MpPerIntellectAtPlayerLevel(uint8_t level) const {
  if (!m_mpPerIntLoaded)
    return 0.f;
  uint32_t const idx = LevelIndex(level);
  if (idx >= m_mpPerInt.GetRecordCount())
    return 0.f;
  return m_mpPerInt.ReadFirstFloatInRecord(idx);
}

} // namespace Firelands

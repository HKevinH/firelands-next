#include "AchievementDbcStore.h"

#include <shared/Logger.h>
#include <shared/dbc/DbcReader.h>

#include <string_view>

namespace Firelands {

namespace {
// `Achievement.dbc` (build 15595): ID(0), Faction, MapID, Supercedes, Title(s),
// Description(x), Category, Points, Ui_order(x), Flags, IconID(x), Reward(x),
// MinimumCriteria(12), SharesCriteria.
constexpr std::string_view kAchievementFmt = "niixsxiixixxii";
constexpr uint32_t kAchievementFieldId = 0;
constexpr uint32_t kAchievementFieldMinCriteria = 12;

// `Achievement_Criteria.dbc`: ID(0), AchievementID(1), Type(2), Asset(3),
// Quantity(4 = int64 'l'), then more. Quantity holds the required level for a
// REACH_LEVEL criterion.
constexpr std::string_view kCriteriaFmt = "niiiliiiisiiiiiiiiiiiii";
constexpr uint32_t kCriteriaFieldAchievementId = 1;
constexpr uint32_t kCriteriaFieldType = 2;
constexpr uint32_t kCriteriaFieldQuantity = 4;
} // namespace

bool AchievementDbcStore::Load(std::string const &achievementPath,
                               std::string const &criteriaPath) {
  m_levelAchievements.clear();

  // Pass 1: criteria → per-achievement total count + REACH_LEVEL required levels.
  struct Acc {
    uint32 total = 0;
    std::vector<uint32> reachLevels;
  };
  std::unordered_map<uint32, Acc> byAchievement;

  DbcReader critReader;
  if (!critReader.Load(criteriaPath)) {
    LOG_WARN("Achievement_Criteria.dbc not readable: {}", criteriaPath);
    return false;
  }
  std::vector<uint32_t> const critOffsets = DbcBuildFieldByteOffsets(kCriteriaFmt);
  if (!critReader.VerifyFormat(kCriteriaFmt)) {
    LOG_WARN("Achievement_Criteria.dbc: field count mismatch (got {}, expected "
             "{}) path={}",
             critReader.GetFieldCount(), kCriteriaFmt.size(), criteriaPath);
    return false;
  }
  uint32_t const critN = critReader.GetRecordCount();
  for (uint32_t rec = 0; rec < critN; ++rec) {
    uint32_t const achId =
        critReader.ReadUInt32(rec, kCriteriaFieldAchievementId, critOffsets);
    if (achId == 0u)
      continue;
    Acc &acc = byAchievement[achId];
    ++acc.total;
    uint32_t const type =
        critReader.ReadUInt32(rec, kCriteriaFieldType, critOffsets);
    if (type == kAchievementCriteriaReachLevel) {
      // Quantity is int64; the required level fits in the low 32 bits.
      uint32_t const reqLevel =
          critReader.ReadUInt32(rec, kCriteriaFieldQuantity, critOffsets);
      if (reqLevel != 0u)
        acc.reachLevels.push_back(reqLevel);
    }
  }

  // Pass 2: achievement rows → MinimumCriteria; keep only those with at least
  // one REACH_LEVEL criterion.
  DbcReader achReader;
  if (!achReader.Load(achievementPath)) {
    LOG_WARN("Achievement.dbc not readable: {}", achievementPath);
    return false;
  }
  std::vector<uint32_t> const achOffsets = DbcBuildFieldByteOffsets(kAchievementFmt);
  if (!achReader.VerifyFormat(kAchievementFmt)) {
    LOG_WARN("Achievement.dbc: field count mismatch (got {}, expected {}) path={}",
             achReader.GetFieldCount(), kAchievementFmt.size(), achievementPath);
    return false;
  }
  uint32_t const achN = achReader.GetRecordCount();
  for (uint32_t rec = 0; rec < achN; ++rec) {
    uint32_t const id = achReader.ReadUInt32(rec, kAchievementFieldId, achOffsets);
    if (id == 0u)
      continue;
    auto it = byAchievement.find(id);
    if (it == byAchievement.end() || it->second.reachLevels.empty())
      continue; // no REACH_LEVEL criterion → not a level achievement
    LevelAchievement la;
    la.achievementId = id;
    la.minCriteria =
        achReader.ReadUInt32(rec, kAchievementFieldMinCriteria, achOffsets);
    la.totalCriteria = it->second.total;
    la.reachLevels = it->second.reachLevels;
    m_levelAchievements.push_back(std::move(la));
  }

  LOG_DEBUG("Achievement DBCs: {} level achievements from {} / {}.",
            m_levelAchievements.size(), achievementPath, criteriaPath);
  return !m_levelAchievements.empty();
}

} // namespace Firelands

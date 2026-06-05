#pragma once

#include <shared/Common.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Firelands {

/// `Achievement_Criteria.dbc` type for "reach character level N" (build 15595).
constexpr uint32 kAchievementCriteriaReachLevel = 5;

/// A level-gated achievement: earned once enough of its REACH_LEVEL criteria are
/// satisfied. v1 only tracks REACH_LEVEL, so an achievement with non-level
/// criteria simply needs all of THOSE to be level criteria to ever complete.
struct LevelAchievement {
  uint32 achievementId = 0;
  uint32 minCriteria = 0;   ///< from Achievement.dbc; 0 = all criteria required
  uint32 totalCriteria = 0; ///< total criteria the achievement has
  /// Required level of each REACH_LEVEL criterion of this achievement.
  std::vector<uint32> reachLevels;
};

/// Loads the achievement DBCs and exposes the subset needed for the REACH_LEVEL
/// vertical slice. Other criteria types are intentionally not indexed yet.
class AchievementDbcStore {
public:
  bool Load(std::string const &achievementPath, std::string const &criteriaPath);

  bool IsLoaded() const { return !m_levelAchievements.empty(); }
  size_t LevelAchievementCount() const { return m_levelAchievements.size(); }

  /// Achievements that have at least one REACH_LEVEL criterion.
  std::vector<LevelAchievement> const &LevelAchievements() const {
    return m_levelAchievements;
  }

private:
  std::vector<LevelAchievement> m_levelAchievements;
};

} // namespace Firelands

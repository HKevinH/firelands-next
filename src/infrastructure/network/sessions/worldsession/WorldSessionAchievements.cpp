#include <application/services/CharacterService.h>
#include <application/services/WorldService.h>
#include <infrastructure/dbc/AchievementDbcStore.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <shared/Logger.h>
#include <shared/network/BitWriter.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>

#include <ctime>

namespace Firelands {

void WorldSession::LoadAchievementsForCharacter(uint32 characterGuid) {
  _earnedAchievements.clear();
  if (_charService && characterGuid != 0u) {
    for (CharacterAchievementRow const &r :
         _charService->GetCharacterAchievements(characterGuid)) {
      _earnedAchievements.emplace(r.achievementId, r.earnedDate);
    }
  }
}

void WorldSession::AwardAchievement(uint32 achievementId, bool announce) {
  if (achievementId == 0 || _earnedAchievements.count(achievementId) != 0u)
    return;
  uint32 const now = static_cast<uint32>(std::time(nullptr));
  _earnedAchievements.emplace(achievementId, now);
  if (_charService)
    _charService->AddCharacterAchievement(static_cast<uint32>(_playerGuid),
                                          achievementId, now);
  if (announce)
    SendAchievementEarned(achievementId, now);
  LOG_DEBUG("[ACHV] earned: achievement {} (level {} announce={})", achievementId,
            _playerLevel, announce);
}

void WorldSession::CheckLevelAchievements(bool announce) {
  if (_playerGuid == 0)
    return;
  auto store = WorldService::Instance().GetAchievementStore();
  if (!store)
    return;

  for (LevelAchievement const &la : store->LevelAchievements()) {
    if (_earnedAchievements.count(la.achievementId) != 0u)
      continue;
    // Count REACH_LEVEL criteria the character already satisfies.
    uint32 satisfied = 0;
    for (uint32 reqLevel : la.reachLevels)
      if (_playerLevel >= reqLevel)
        ++satisfied;
    // MinimumCriteria 0 means every criterion is required. Non-level criteria
    // can never be satisfied in this slice, so only achievements whose required
    // count is met purely by level criteria are awarded.
    uint32 const need =
        (la.minCriteria > 0) ? la.minCriteria : la.totalCriteria;
    if (need != 0 && satisfied >= need)
      AwardAchievement(la.achievementId, announce);
  }
}

void WorldSession::SendAchievementEarned(uint32 achievementId,
                                         uint32 earnedDate) {
  WorldPacket data(SMSG_ACHIEVEMENT_EARNED);
  data.AppendPackGUID(_playerGuid);
  data.Append<uint32>(achievementId);
  data.AppendPackedTime(static_cast<time_t>(earnedDate));
  data.Append<uint32>(0); // 0 = also shown to others; non-zero = self only
  SendPacket(data);
}

void WorldSession::SendAllAchievementData() {
  // v1: no in-progress criteria are sent (numCriteria = 0), only the list of
  // earned achievements, so they show up on login. The criteria section uses
  // heavy Cata bit-packing and is deferred.
  WorldPacket data(SMSG_ALL_ACHIEVEMENT_DATA);
  BitWriter bw(data);
  bw.WriteBits(0, 21); // numCriteria
  bw.WriteBits(static_cast<uint32>(_earnedAchievements.size()), 23);
  bw.Flush();
  for (auto const &[achievementId, earnedDate] : _earnedAchievements) {
    data.Append<uint32>(achievementId);
    data.AppendPackedTime(static_cast<time_t>(earnedDate));
  }
  SendPacket(data);
}

} // namespace Firelands

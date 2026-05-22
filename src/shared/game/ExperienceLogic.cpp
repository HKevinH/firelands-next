#include <shared/game/ExperienceLogic.h>

#include <algorithm>
#include <cmath>

namespace Firelands::ExperienceLogic {

uint8_t GrayLevelForPlayer(uint8_t playerLevel) {
  if (playerLevel <= 5)
    return 0;
  if (playerLevel <= 39)
    return static_cast<uint8_t>(playerLevel - 5 - (playerLevel % 5));
  if (playerLevel <= 59)
    return static_cast<uint8_t>(playerLevel - 10 - (playerLevel % 5));
  return static_cast<uint8_t>(playerLevel - 15 - (playerLevel % 5));
}

uint32_t BaseKillExperienceForCreatureLevel(uint8_t creatureLevel) {
  uint8_t const lv = std::max<uint8_t>(1, creatureLevel);
  return static_cast<uint32_t>(lv) * 45u + 5u;
}

uint32_t CalculateKillExperience(uint8_t playerLevel, uint8_t creatureLevel,
                                 float creatureExperienceModifier,
                                 ExperienceRates const &rates) {
  if (playerLevel == 0 || playerLevel >= kMaxPlayerLevelCata)
    return 0;
  uint8_t const gray = GrayLevelForPlayer(playerLevel);
  if (creatureLevel <= gray)
    return 0;

  float factor = 1.0f;
  int const diff = static_cast<int>(creatureLevel) - static_cast<int>(playerLevel);
  if (diff < 0) {
    if (diff >= -5)
      factor = 1.0f + static_cast<float>(diff) * 0.1f;
    else
      factor = 0.1f;
  } else if (diff > 0) {
    factor = 1.0f + static_cast<float>(diff) * 0.05f;
    if (factor > 1.2f)
      factor = 1.2f;
  }

  float const expMod =
      creatureExperienceModifier > 0.0f ? creatureExperienceModifier : 1.0f;
  float const killRate = rates.kill > 0.0f ? rates.kill : 1.0f;
  double const xp = static_cast<double>(BaseKillExperienceForCreatureLevel(creatureLevel)) *
                    static_cast<double>(factor) * static_cast<double>(expMod) *
                    static_cast<double>(killRate);
  if (xp < 1.0)
    return 0;
  return static_cast<uint32_t>(std::floor(xp));
}

ExperienceGainResult ApplyExperienceGain(
    uint8_t level, uint32_t xp, uint32_t amount, uint8_t maxLevel,
    std::function<uint32_t(uint8_t)> const &xpToNext) {
  ExperienceGainResult result;
  result.level = std::max<uint8_t>(1, level);
  result.xp = xp;

  if (result.level >= maxLevel || amount == 0)
    return result;

  uint64_t total = static_cast<uint64_t>(result.xp) + static_cast<uint64_t>(amount);

  while (result.level < maxLevel) {
    uint32_t need = xpToNext(result.level);
    if (need == 0)
      need = 400u;
    if (total < need)
      break;
    total -= need;
    ++result.level;
    ++result.levelsGained;
  }

  if (result.level >= maxLevel) {
    result.level = maxLevel;
    result.xp = 0;
  } else {
    uint32_t need = xpToNext(result.level);
    if (need == 0)
      need = 400u;
    if (total > need)
      total = need;
    result.xp = static_cast<uint32_t>(total);
  }

  return result;
}

} // namespace Firelands::ExperienceLogic

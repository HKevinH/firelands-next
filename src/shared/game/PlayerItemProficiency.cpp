#include <shared/game/PlayerItemProficiency.h>

namespace Firelands {
namespace {

// `ItemSubclassWeapon` bit masks (4.3.4 client).
constexpr uint32_t kMaskAxe = 1u << 0u;
constexpr uint32_t kMaskAxe2 = 1u << 1u;
constexpr uint32_t kMaskBow = 1u << 2u;
constexpr uint32_t kMaskGun = 1u << 3u;
constexpr uint32_t kMaskMace = 1u << 4u;
constexpr uint32_t kMaskMace2 = 1u << 5u;
constexpr uint32_t kMaskPolearm = 1u << 6u;
constexpr uint32_t kMaskSword = 1u << 7u;
constexpr uint32_t kMaskSword2 = 1u << 8u;
constexpr uint32_t kMaskStaff = 1u << 9u;
constexpr uint32_t kMaskFist = 1u << 10u;
constexpr uint32_t kMaskDagger = 1u << 11u;
constexpr uint32_t kMaskThrown = 1u << 12u;
constexpr uint32_t kMaskCrossbow = 1u << 14u;
constexpr uint32_t kMaskWand = 1u << 15u;

// `ItemSubclassArmor` bit masks.
constexpr uint32_t kMaskArmorMisc = 1u << 0u;
constexpr uint32_t kMaskArmorCloth = 1u << 1u;
constexpr uint32_t kMaskArmorLeather = 1u << 2u;
constexpr uint32_t kMaskArmorMail = 1u << 3u;
constexpr uint32_t kMaskArmorPlate = 1u << 4u;

void ApplyWeaponSkillMask(uint32_t skillId, uint32_t &mask) {
  switch (skillId) {
  case 44u:
    mask |= kMaskAxe;
    break;
  case 172u:
    mask |= kMaskAxe2;
    break;
  case 45u:
    mask |= kMaskBow;
    break;
  case 46u:
    mask |= kMaskGun;
    break;
  case 54u:
    mask |= kMaskMace;
    break;
  case 160u:
    mask |= kMaskMace2;
    break;
  case 229u:
    mask |= kMaskPolearm;
    break;
  case 43u:
    mask |= kMaskSword;
    break;
  case 55u:
    mask |= kMaskSword2;
    break;
  case 136u:
    mask |= kMaskStaff;
    break;
  case 473u:
    mask |= kMaskFist;
    break;
  case 173u:
    mask |= kMaskDagger;
    break;
  case 176u:
    mask |= kMaskThrown;
    break;
  case 226u:
    mask |= kMaskCrossbow;
    break;
  case 228u:
    mask |= kMaskWand;
    break;
  default:
    break;
  }
}

void ApplyArmorSkillMask(uint32_t skillId, uint32_t &mask) {
  switch (skillId) {
  case 415u:
    mask |= kMaskArmorCloth;
    break;
  case 414u:
    mask |= kMaskArmorLeather;
    break;
  case 413u:
    mask |= kMaskArmorMail;
    break;
  case 293u:
    mask |= kMaskArmorPlate;
    break;
  default:
    break;
  }
}

} // namespace

uint32_t ComputeWeaponProficiencyMask(
    std::vector<StarterSkillGrant> const &skills) {
  uint32_t mask = 0;
  for (StarterSkillGrant const &g : skills)
    ApplyWeaponSkillMask(g.skillId, mask);
  return mask;
}

uint32_t ComputeArmorProficiencyMask(
    std::vector<StarterSkillGrant> const &skills) {
  uint32_t mask = kMaskArmorMisc;
  for (StarterSkillGrant const &g : skills)
    ApplyArmorSkillMask(g.skillId, mask);
  return mask;
}

} // namespace Firelands

#include <gtest/gtest.h>
#include <application/spell/PlayerAuraStatEffects.h>
#include <domain/models/SpellDefinition.h>
#include <domain/repositories/ISpellDefinitionStore.h>
#include <domain/world/Aura.h>
#include <shared/game/SpellAuraTypes.h>
#include <shared/game/SpellAttributes.h>
#include <unordered_map>

using namespace Firelands;

namespace {

class StatAuraStore final : public ISpellDefinitionStore {
public:
  void Add(SpellDefinition def) { m_defs[def.id] = std::move(def); }

  bool HasSpell(uint32 spellId) const override {
    return m_defs.find(spellId) != m_defs.end();
  }
  std::optional<SpellDefinition> GetDefinition(uint32 spellId) const override {
    auto it = m_defs.find(spellId);
    if (it == m_defs.end())
      return std::nullopt;
    return it->second;
  }

  std::unordered_map<uint32, SpellDefinition> m_defs;
};

Aura MakePassiveAura(uint32 spellId) {
  auto const farFuture = std::chrono::steady_clock::time_point::max();
  return Aura(spellId, kSpellAuraModStat, 3, 0, 0x100ULL, farFuture, 0, 0, 0,
              farFuture, {});
}

} // namespace

TEST(PlayerAuraStatEffectsTests, ModStatAddsPosStatFromAuraEffectsRow) {
  SpellDefinition def{};
  def.id = 100u;
  def.attributes = SpellAttr0::kPassive;
  SpellAuraEffectRow row{};
  row.auraType = kSpellAuraModStat;
  row.basePoints = 2;
  row.miscValue = 1;
  def.auraEffects.push_back(row);

  StatAuraStore store;
  store.Add(def);

  std::vector<Aura> auras{MakePassiveAura(100u)};
  PlayerAuraStatBonus const bonus =
      ComputePlayerAuraStatBonus(auras, &store, 10);

  EXPECT_EQ(bonus.posStat[1], 3);
  EXPECT_EQ(bonus.posStat[0], 0);
}

TEST(PlayerAuraStatEffectsTests, ModAttackPowerAddsPosMod) {
  SpellDefinition def{};
  def.id = 20572u;
  SpellAuraEffectRow row{};
  row.auraType = kSpellAuraModAttackPower;
  row.basePoints = 6;
  def.auraEffects.push_back(row);

  StatAuraStore store;
  store.Add(def);

  std::vector<Aura> auras{MakePassiveAura(20572u)};
  PlayerAuraStatBonus const bonus =
      ComputePlayerAuraStatBonus(auras, &store, 10);

  EXPECT_EQ(bonus.attackPowerModPos, 7);
  EXPECT_EQ(bonus.attackPowerModNeg, 0);
  EXPECT_FLOAT_EQ(bonus.attackPowerMultiplier, 0.f);
}

TEST(PlayerAuraStatEffectsTests, ModAttackPowerPctAddsMultiplier) {
  SpellDefinition def{};
  def.id = 20572u;
  SpellAuraEffectRow row{};
  row.auraType = kSpellAuraModAttackPowerPct;
  row.basePoints = 6;
  def.auraEffects.push_back(row);

  StatAuraStore store;
  store.Add(def);

  std::vector<Aura> auras{MakePassiveAura(20572u)};
  PlayerAuraStatBonus const bonus =
      ComputePlayerAuraStatBonus(auras, &store, 10);

  EXPECT_EQ(bonus.attackPowerModPos, 0);
  EXPECT_FLOAT_EQ(bonus.attackPowerMultiplier, 0.07f);
}

TEST(PlayerAuraStatEffectsTests, BloodFuryStacksFlatAndPctRows) {
  SpellDefinition def{};
  def.id = 20572u;
  SpellAuraEffectRow flat{};
  flat.auraType = kSpellAuraModAttackPower;
  flat.basePoints = 6;
  SpellAuraEffectRow pct{};
  pct.auraType = kSpellAuraModAttackPowerPct;
  pct.basePoints = 6;
  def.auraEffects.push_back(flat);
  def.auraEffects.push_back(pct);

  StatAuraStore store;
  store.Add(def);

  std::vector<Aura> auras{MakePassiveAura(20572u)};
  PlayerAuraStatBonus const bonus =
      ComputePlayerAuraStatBonus(auras, &store, 85);

  EXPECT_EQ(bonus.attackPowerModPos, 7);
  EXPECT_FLOAT_EQ(bonus.attackPowerMultiplier, 0.07f);
}

TEST(PlayerAuraStatEffectsTests, ModRatingAddsCombatRating) {
  SpellDefinition def{};
  def.id = 200u;
  SpellAuraEffectRow row{};
  row.auraType = kSpellAuraModRating;
  row.basePoints = 7;
  row.miscValue = 5;
  def.auraEffects.push_back(row);

  StatAuraStore store;
  store.Add(def);

  std::vector<Aura> auras{MakePassiveAura(200u)};
  PlayerAuraStatBonus const bonus =
      ComputePlayerAuraStatBonus(auras, &store, 20);

  EXPECT_EQ(bonus.combatRating[5], 8);
}

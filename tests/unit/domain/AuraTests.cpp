#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <domain/world/Aura.h>
#include <domain/world/Creature.h>
#include <domain/world/Player.h>

using namespace Firelands;

namespace {

Aura MakeAura(uint32 spellId, uint32 auraEffectType, int32 basePoints, int32 dieSides,
              uint64 casterGuid, std::chrono::steady_clock::time_point expireTime,
              uint8 visualSlot = 0, uint32 periodicPeriodMs = 0,
              int32 periodicHealthDeltaPerTick = 0) {
  auto const nextTick = std::chrono::steady_clock::now();
  return Aura(spellId, auraEffectType, basePoints, dieSides, casterGuid, expireTime,
              visualSlot, periodicPeriodMs, periodicHealthDeltaPerTick, nextTick);
}

} // namespace

TEST(AuraTests, ConstructorInitializesFieldsCorrectly) {
  auto const now = std::chrono::steady_clock::now();
  auto const expireTime = now + std::chrono::seconds(30);
  Aura const aura = MakeAura(123, 4, 10, 6, 0x1000ULL, expireTime, 2);

  EXPECT_EQ(aura.GetSpellId(), 123u);
  EXPECT_EQ(aura.GetAuraEffectType(), 4u);
  EXPECT_EQ(aura.GetBasePoints(), 10);
  EXPECT_EQ(aura.GetDieSides(), 6);
  EXPECT_EQ(aura.GetCasterGuid(), 0x1000ULL);
  EXPECT_EQ(aura.GetExpireTime(), expireTime);
  EXPECT_EQ(aura.GetVisualSlot(), 2u);
}

TEST(AuraTests, IsExpiredReturnsFalseWhenNotExpired) {
  auto const now = std::chrono::steady_clock::now();
  Aura const aura = MakeAura(123, 4, 10, 6, 0x1000ULL, now + std::chrono::seconds(30));
  EXPECT_FALSE(aura.IsExpired(now));
}

TEST(AuraTests, IsExpiredReturnsTrueWhenExpired) {
  auto const now = std::chrono::steady_clock::now();
  Aura const aura = MakeAura(123, 4, 10, 6, 0x1000ULL, now - std::chrono::seconds(30));
  EXPECT_TRUE(aura.IsExpired(now));
}

TEST(AuraTests, GetMagnitudeReturnsBasePointsWhenDieSidesZero) {
  auto const now = std::chrono::steady_clock::now();
  Aura const aura = MakeAura(123, 4, 15, 0, 0x1000ULL, now + std::chrono::seconds(30));
  EXPECT_EQ(aura.GetMagnitude(), 15);
}

TEST(AuraTests, GetMagnitudeIncludesHalfDieSidesWhenPositive) {
  auto const now = std::chrono::steady_clock::now();
  Aura const aura = MakeAura(123, 4, 10, 6, 0x1000ULL, now + std::chrono::seconds(30));
  EXPECT_EQ(aura.GetMagnitude(), 13);
}

TEST(AuraTests, PeriodicTickDueAndAdvance) {
  auto const now = std::chrono::steady_clock::now();
  Aura aura = MakeAura(1, 8, 5, 0, 0, now + std::chrono::minutes(1), 0, 1000, 10);
  EXPECT_FALSE(aura.IsPeriodicDue(now));
  aura.AdvancePeriodicTick(now);
  EXPECT_TRUE(aura.IsPeriodicDue(now + std::chrono::milliseconds(1001)));
}

TEST(PlayerAuraTests, PlayerUpdatesAurasCorrectly) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  auto const now = std::chrono::steady_clock::now();
  player->AddAura(MakeAura(123, 4, 10, 6, 0x2000ULL, now - std::chrono::seconds(30)));
  player->AddAura(MakeAura(456, 5, 15, 8, 0x3000ULL, now + std::chrono::seconds(30)));

  EXPECT_FALSE(player->HasAura(123));
  EXPECT_TRUE(player->HasAura(456));

  auto const removed = player->UpdateAuras(now);
  ASSERT_EQ(removed.size(), 1u);
  EXPECT_EQ(removed[0].spellId, 123u);
  EXPECT_FALSE(player->HasAura(123));
  EXPECT_TRUE(player->HasAura(456));
}

TEST(PlayerAuraTests, PlayerCanRemoveAura) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  auto const now = std::chrono::steady_clock::now();
  player->AddAura(MakeAura(123, 4, 10, 6, 0x2000ULL, now + std::chrono::seconds(30)));
  EXPECT_TRUE(player->HasAura(123));
  player->RemoveAura(123);
  EXPECT_FALSE(player->HasAura(123));
}

TEST(PlayerAuraTests, TryRemoveAuraReturnsVisualSlot) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  auto const now = std::chrono::steady_clock::now();
  player->AddAura(MakeAura(774, 8, 10, 0, 0, now + std::chrono::seconds(30), 5));
  auto const removal = player->TryRemoveAura(774);
  ASSERT_TRUE(removal.has_value());
  EXPECT_EQ(removal->spellId, 774u);
  EXPECT_EQ(removal->visualSlot, 5u);
  EXPECT_FALSE(player->HasAura(774));
  EXPECT_FALSE(player->TryRemoveAura(774).has_value());
}

TEST(PlayerAuraTests, AllocateAuraVisualSlotReusesSlotForSameSpell) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  auto const now = std::chrono::steady_clock::now();
  player->AddAura(MakeAura(99, 4, 1, 0, 0, now + std::chrono::seconds(10), 3));
  EXPECT_EQ(player->AllocateAuraVisualSlot(99), 3u);
  EXPECT_EQ(player->AllocateAuraVisualSlot(100), 0u);
}

TEST(PlayerAuraTests, PeriodicTickReturnsHealthDelta) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  auto const now = std::chrono::steady_clock::now();
  Aura aura = MakeAura(77, 8, 0, 0, 0, now + std::chrono::minutes(1), 1, 500, 25);
  aura.AdvancePeriodicTick(now - std::chrono::seconds(1));
  player->AddAura(aura);

  auto ticks = player->TickPeriodicAuras(now);
  ASSERT_EQ(ticks.size(), 1u);
  EXPECT_EQ(ticks[0].spellId, 77u);
  EXPECT_EQ(ticks[0].healthDelta, 25);
}

TEST(CreatureAuraTests, AddAuraAndRemoveBySpellId) {
  Creature creature(0x2000ULL, 1, 100, 500);
  auto const now = std::chrono::steady_clock::now();
  creature.AddAura(MakeAura(589, 3, 5, 0, 0x100ULL, now + std::chrono::seconds(20), 2));
  EXPECT_TRUE(creature.HasAura(589));
  auto const removal = creature.TryRemoveAura(589);
  ASSERT_TRUE(removal.has_value());
  EXPECT_EQ(removal->visualSlot, 2u);
  EXPECT_FALSE(creature.HasAura(589));
}

TEST(AuraTests, GetRemainingMsZeroAtAndAfterExpiry) {
  auto const now = std::chrono::steady_clock::now();
  Aura const aura = MakeAura(1, 8, 0, 0, 0, now);
  EXPECT_EQ(aura.GetRemainingMs(now).count(), 0);
  EXPECT_TRUE(aura.IsExpired(now));
  EXPECT_EQ(aura.GetRemainingMs(now + std::chrono::seconds(1)).count(), 0);
}

TEST(PlayerAuraTests, TickAurasRemovesExpiredBeforePeriodic) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  auto const now = std::chrono::steady_clock::now();
  Aura aura = MakeAura(589, 3, 0, 0, 0, now, 0, 1000, -5);
  aura.AdvancePeriodicTick(now - std::chrono::milliseconds(1));
  player->AddAura(aura);

  UnitAuraTickResult const result = player->TickAuras(now);
  ASSERT_EQ(result.removals.size(), 1u);
  EXPECT_EQ(result.removals[0].spellId, 589u);
  EXPECT_TRUE(result.periodicTicks.empty());
  EXPECT_FALSE(player->HasAura(589));
}

TEST(PlayerAuraTests, TickPeriodicAurasSkipsExpiredWithoutPriorUpdate) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  auto const now = std::chrono::steady_clock::now();
  Aura aura = MakeAura(77, 8, 0, 0, 0, now, 1, 500, 25);
  aura.AdvancePeriodicTick(now - std::chrono::seconds(1));
  player->AddAura(aura);

  auto ticks = player->TickPeriodicAuras(now);
  EXPECT_TRUE(ticks.empty());
}

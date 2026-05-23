#include <application/gm/GmPlayerRevive.h>
#include <domain/ports/IMapNotifier.h>
#include <domain/world/Player.h>
#include <gtest/gtest.h>
#include <shared/network/WorldPacket.h>

namespace Firelands {
namespace {

class NullNotifier final : public IMapNotifier {
public:
  void SendPacket(WorldPacket &) override {}
  uint64 GetGuid() const override { return 1; }
};

TEST(GmPlayerReviveTests, RestoresHealthAndPowerFromZero) {
  Player player(1, std::make_shared<NullNotifier>());
  player.InitCombatResources(0, 100, 50, 100);

  GmReviveOutcome const outcome = ApplyGmReviveToPlayer(player);
  EXPECT_EQ(outcome.result, GmReviveResult::RevivedFromDeath);
  EXPECT_TRUE(outcome.healthChanged);
  EXPECT_TRUE(outcome.powerChanged);
  EXPECT_EQ(player.GetLiveHealth(), 100u);
  EXPECT_EQ(player.GetLivePower1(), 100u);
}

TEST(GmPlayerReviveTests, AlreadyFullReturnsWithoutChanges) {
  Player player(1, std::make_shared<NullNotifier>());
  player.InitCombatResources(100, 100, 50, 50);

  GmReviveOutcome const outcome = ApplyGmReviveToPlayer(player);
  EXPECT_EQ(outcome.result, GmReviveResult::AlreadyFull);
  EXPECT_FALSE(outcome.healthChanged);
  EXPECT_FALSE(outcome.powerChanged);
}

} // namespace
} // namespace Firelands

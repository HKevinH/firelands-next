#include <gtest/gtest.h>
#include <application/combat/CombatEntityAdapters.h>
#include <application/combat/CombatService.h>
#include <domain/ports/IMapNotifier.h>
#include <domain/combat/CombatEngine.h>
#include <domain/world/Creature.h>
#include <domain/world/Player.h>
#include <infrastructure/combat/InMemoryThreatManager.h>
#include <domain/combat/repositories/ISpellProcessor.h>

using namespace Firelands;

namespace {

class NullNotifier final : public IMapNotifier {
public:
  void SendPacket(WorldPacket & /*packet*/) override {}
  uint64 GetGuid() const override { return 0; }
};

class NullSpellProcessor : public combat::ISpellProcessor {
public:
  bool CanCast(uint64_t, uint64_t) override { return true; }
  void ExecuteCast(uint64_t, uint64_t) override {}
};

} // namespace

TEST(CombatIntegrationTest, MeleeSwingAddsThreatAndDamage) {
  auto threatMgr = std::make_shared<infrastructure::InMemoryThreatManager>();
  auto spellProc = std::make_shared<NullSpellProcessor>();
  auto engine = std::make_shared<combat::CombatEngine>(threatMgr, spellProc);
  application::CombatService service(engine);

  auto notifier = std::make_shared<NullNotifier>();
  auto player = std::make_shared<Player>(100ull, notifier);
  player->InitCombatResources(100u, 100u, 0u, 100u);
  auto creature = std::make_shared<Creature>(200ull, 1u, 1u, 50u);

  application::adapters::PlayerCombatEntity attacker(player);
  application::adapters::CreatureCombatEntity victim(creature);

  auto const result = service.BeginMeleeSwing(attacker, victim);
  EXPECT_EQ(result, application::MeleeSwingResult::Success);
  EXPECT_EQ(threatMgr->GetTopThreat(creature->GetGuid()), player->GetGuid());
  EXPECT_LT(creature->GetLiveHealth(), 50u);
}

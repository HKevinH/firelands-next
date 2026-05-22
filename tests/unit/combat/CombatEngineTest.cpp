#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <domain/combat/CombatEngine.h>

using namespace combat;
using namespace testing;

class MockThreatManager : public IThreatManager {
public:
  MOCK_METHOD(void, AddThreat, (uint64_t, uint64_t, float), (override));
  MOCK_METHOD(void, RemoveThreat, (uint64_t, uint64_t), (override));
  MOCK_METHOD(uint64_t, GetTopThreat, (uint64_t), (const, override));
};

class MockSpellProcessor : public ISpellProcessor {
public:
  MOCK_METHOD(bool, CanCast, (uint64_t, uint64_t), (override));
  MOCK_METHOD(void, ExecuteCast, (uint64_t, uint64_t), (override));
};

class MockEntity : public ICombatEntity {
public:
  MOCK_METHOD(uint64_t, GetGuid, (), (const, override));
  MOCK_METHOD(bool, IsAlive, (), (const, override));
  MOCK_METHOD(void, TakeDamage, (float), (override));
};

TEST(CombatEngineTest, EngageAddsThreatForAttackerOnVictimTable) {
  auto threatMgr = std::make_shared<MockThreatManager>();
  auto spellProc = std::make_shared<MockSpellProcessor>();
  CombatEngine engine(threatMgr, spellProc);

  MockEntity attacker;
  MockEntity victim;
  EXPECT_CALL(attacker, GetGuid()).WillRepeatedly(Return(42));
  EXPECT_CALL(attacker, IsAlive()).WillRepeatedly(Return(true));
  EXPECT_CALL(victim, IsAlive()).WillRepeatedly(Return(true));
  EXPECT_CALL(victim, GetGuid()).WillRepeatedly(Return(123));

  EXPECT_CALL(*threatMgr, AddThreat(123, 42, 1.0f)).Times(1);

  engine.Engage(attacker, victim);
}

TEST(CombatEngineTest, UpdateAppliesDamageWhenAlive) {
  auto threatMgr = std::make_shared<MockThreatManager>();
  auto spellProc = std::make_shared<MockSpellProcessor>();
  CombatEngine engine(threatMgr, spellProc);

  MockEntity attacker;
  MockEntity victim;
  EXPECT_CALL(attacker, IsAlive()).WillRepeatedly(Return(true));
  EXPECT_CALL(victim, IsAlive()).WillRepeatedly(Return(true));
  EXPECT_CALL(victim, TakeDamage(10.0f)).Times(1);

  engine.Update(attacker, victim);
}

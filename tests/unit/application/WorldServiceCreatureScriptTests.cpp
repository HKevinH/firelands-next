#include <application/services/WorldService.h>
#include <domain/world/Creature.h>
#include <gtest/gtest.h>
#include <infrastructure/scripting/LuaGameScriptHost.h>
#include <shared/Logger.h>
#include <shared/network/MovementInfo.h>

using namespace Firelands;

TEST(WorldServiceCreatureSpawn, LuaReceivesCreatureSpawnEvent) {
  if (!Logger::IsInitialized()) {
    Logger::Init(LoggerBuilder().WithConsole(false).Build());
  }
  auto lua = std::make_shared<LuaGameScriptHost>();
  ASSERT_TRUE(lua->Init(""));
  std::string err;
  ASSERT_TRUE(lua->RunChunk(R"(
    _last = ""
    function OnScriptEvent(name, guid)
      if name == "creature_spawn" then
        _last = tostring(guid)
      end
    end
  )",
                            &err))
      << err;

  WorldService::Instance().SetScriptHost(lua);

  auto creature = std::make_shared<Creature>(555u, 1u, 1u);
  MovementInfo pos{};
  pos.x = 5.f;
  pos.y = 5.f;
  creature->SetPosition(pos);
  WorldService::Instance().AddCreatureToMap(9999u, creature);

  std::string last;
  ASSERT_TRUE(lua->TryGetGlobalString("_last", &last));
  EXPECT_EQ(last, "555");

  WorldService::Instance().RemovePlayerFromMap(9999u, 555u);
  WorldService::Instance().SetScriptHost(nullptr);
}

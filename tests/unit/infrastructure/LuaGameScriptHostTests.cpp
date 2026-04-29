#include <gtest/gtest.h>
#include <infrastructure/scripting/LuaGameScriptHost.h>

using namespace Firelands;

TEST(LuaGameScriptHost, RunChunkSuccess) {
  LuaGameScriptHost host;
  ASSERT_TRUE(host.Init(""));
  std::string err;
  EXPECT_TRUE(host.RunChunk("local x = 1 + 1", &err));
  EXPECT_TRUE(err.empty());
}

TEST(LuaGameScriptHost, RunChunkSyntaxError) {
  LuaGameScriptHost host;
  ASSERT_TRUE(host.Init(""));
  std::string err;
  EXPECT_FALSE(host.RunChunk("this is not valid lua +++", &err));
  EXPECT_FALSE(err.empty());
}

TEST(LuaGameScriptHost, FireGossipSelectSetsContextGlobals) {
  LuaGameScriptHost host;
  ASSERT_TRUE(host.Init(""));
  std::string err;
  ASSERT_TRUE(host.RunChunk(R"(
    function OnScriptEvent(name, guid)
      if name == "gossip_select" then
        _capture = tostring(guid) .. ":" .. tostring(_gossipMenuId) .. ":" .. tostring(_gossipListId)
      end
    end
  )",
                            &err))
      << err;

  host.FireGossipSelect(77, 100, 200);

  std::string cap;
  ASSERT_TRUE(host.TryGetGlobalString("_capture", &cap));
  EXPECT_EQ(cap, "77:100:200");
}

TEST(LuaGameScriptHost, FireEventInvokesGlobal) {
  LuaGameScriptHost host;
  ASSERT_TRUE(host.Init(""));
  std::string err;
  ASSERT_TRUE(host.RunChunk(R"(
    _capture = ""
    function OnScriptEvent(name, guid)
      _capture = name .. ":" .. tostring(guid)
    end
  )",
                            &err))
      << err;

  host.FireEvent("spawn", 99);

  std::string cap;
  ASSERT_TRUE(host.TryGetGlobalString("_capture", &cap));
  EXPECT_EQ(cap, "spawn:99");
}

#include <gtest/gtest.h>
#include <shared/Config.h>

using namespace Firelands;

TEST(ConfigTest, LogLevelConversion) {
    YAML::Node node = YAML::Load("trace");
    EXPECT_EQ(node.as<LogLevel>(), LogLevel::Trace);

    node = YAML::Load("DEBUG");
    EXPECT_EQ(node.as<LogLevel>(), LogLevel::Debug);

    node = YAML::Load("Info");
    EXPECT_EQ(node.as<LogLevel>(), LogLevel::Info);

    node = YAML::Load("WARN");
    EXPECT_EQ(node.as<LogLevel>(), LogLevel::Warn);

    node = YAML::Load("error");
    EXPECT_EQ(node.as<LogLevel>(), LogLevel::Error);

    node = YAML::Load("CRITICAL");
    EXPECT_EQ(node.as<LogLevel>(), LogLevel::Critical);

    node = YAML::Load("off");
    EXPECT_EQ(node.as<LogLevel>(), LogLevel::Off);
}

TEST(ConfigTest, NestedAccess) {
    YAML::Node root = YAML::Load("Logging:\n  Console: debug\n  File: error");
    
    // Test basic yaml-cpp access
    EXPECT_EQ(root["Logging"]["Console"].as<LogLevel>(), LogLevel::Debug);
    EXPECT_EQ(root["Logging"]["File"].as<LogLevel>(), LogLevel::Error);
}

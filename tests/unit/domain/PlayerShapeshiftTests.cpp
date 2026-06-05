#include <gtest/gtest.h>
#include <domain/world/Player.h>
#include <shared/game/ShapeshiftForms.h>

using namespace Firelands;

TEST(PlayerShapeshiftTests, DefaultsToFormNone) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  EXPECT_EQ(player->GetShapeshiftForm(), FORM_NONE);
}

TEST(PlayerShapeshiftTests, SetAndGetForm) {
  auto player = std::make_unique<Player>(0x1000ULL, nullptr);
  player->SetShapeshiftForm(FORM_DEFENSIVESTANCE);
  EXPECT_EQ(player->GetShapeshiftForm(), FORM_DEFENSIVESTANCE);
  player->SetShapeshiftForm(FORM_BERSERKERSTANCE);
  EXPECT_EQ(player->GetShapeshiftForm(), FORM_BERSERKERSTANCE);
  player->SetShapeshiftForm(FORM_NONE);
  EXPECT_EQ(player->GetShapeshiftForm(), FORM_NONE);
}

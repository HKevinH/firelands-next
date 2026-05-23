-- Level-1 spells granted by ref Player::LearnSkillRewardedSpells when class-tab
-- skills are learned at create (SkillLineAbility acquire method 2).
-- INSERT IGNORE: safe with per-class starter migrations and DBC merge at runtime.
-- Warlock demon summons stay quest-gated (migrations 50/51). Druid flight (33943,
-- 40120) stay removed (migration 46). Druid 86470/86530 were removed by mistake in 46.
USE `firelands_world`;

INSERT IGNORE INTO `playercreateinfo_spell` (`raceMask`, `classMask`, `spellId`) VALUES
-- Warrior
(0, 1, 2457),
(0, 1, 49410),
(0, 1, 88161),
(0, 1, 88163),
-- Paladin
(0, 2, 35395),
(0, 2, 49410),
-- Hunter
(0, 4, 75),
(0, 4, 883),
(0, 4, 982),
(0, 4, 3044),
(0, 4, 77442),
(0, 4, 87816),
-- Rogue
(0, 8, 1752),
-- Priest
(0, 16, 585),
(0, 16, 84733),
(0, 16, 88685),
-- Death Knight
(0, 32, 49410),
(0, 32, 59879),
-- Shaman
(0, 64, 403),
-- Mage
(0, 128, 133),
-- Warlock (not 688+ — quest-gated)
(0, 256, 686),
(0, 256, 58284),
(0, 256, 86213),
-- Druid (restore passives removed in 46; no flight forms)
(0, 1024, 5176),
(0, 1024, 79577),
(0, 1024, 84736),
(0, 1024, 86470),
(0, 1024, 86530);

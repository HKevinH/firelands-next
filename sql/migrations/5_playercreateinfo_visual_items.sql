CREATE DATABASE IF NOT EXISTS `firelands_world`;
USE `firelands_world`;

-- Visual items shown on the character select screen (SMSG_CHAR_ENUM).
-- This is the "starter clothing" / initial look, driven by DB.
--
-- We store visual parameters directly (invType/displayId/enchant) so we don't
-- need item_template + ItemDisplayInfo DBC plumbing yet.
--
-- Wildcards:
-- - race=0 => any race
-- - class=0 => any class
-- - gender=2 => any gender
-- - outfitId=0 => default outfit
CREATE TABLE IF NOT EXISTS `playercreateinfo_visual_items` (
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `gender` tinyint(3) unsigned NOT NULL DEFAULT '2',
  `outfitId` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `slot` tinyint(3) unsigned NOT NULL,
  `invType` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `displayId` int(10) unsigned NOT NULL DEFAULT '0',
  `displayEnchantId` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `gender`, `outfitId`, `slot`),
  KEY `idx_class` (`class`),
  KEY `idx_race_class` (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;


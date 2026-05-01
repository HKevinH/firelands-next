CREATE DATABASE IF NOT EXISTS `firelands_world`;
USE `firelands_world`;

-- Backfill migration: `2_playercreateinfo.sql` may have been marked applied
-- before this table existed.
CREATE TABLE IF NOT EXISTS `playercreateinfo_reputation` (
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',   -- 0 = wildcard
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',  -- 0 = wildcard
  `minLevel` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `maxLevel` tinyint(3) unsigned NOT NULL DEFAULT '255',
  `factionId` int(10) unsigned NOT NULL,             -- Faction.dbc ID
  `flags` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `standing` int(10) NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `minLevel`, `maxLevel`, `factionId`),
  KEY `idx_race_class` (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Minimal starter reputations by race (home city coalition), factionId based.
-- flags: 0x01 Visible.
INSERT IGNORE INTO `playercreateinfo_reputation`
  (`race`, `class`, `minLevel`, `maxLevel`, `factionId`, `flags`, `standing`)
VALUES
  -- Alliance coalition (Humans use this seed — you can extend per-race later)
  (1, 0, 1, 255, 72,  1, 0),  -- Stormwind
  (1, 0, 1, 255, 47,  1, 0),  -- Ironforge
  (1, 0, 1, 255, 69,  1, 0),  -- Darnassus
  (1, 0, 1, 255, 54,  1, 0),  -- Gnomeregan
  (1, 0, 1, 255, 930, 1, 0),  -- Exodar
  -- Horde coalition (Orcs use this seed — covers Trolls too via faction list)
  (2, 0, 1, 255, 76,  1, 0),  -- Orgrimmar
  (2, 0, 1, 255, 81,  1, 0),  -- Thunder Bluff
  (2, 0, 1, 255, 68,  1, 0),  -- Undercity
  (2, 0, 1, 255, 530, 1, 0),  -- Darkspear Trolls
  (2, 0, 1, 255, 911, 1, 0);  -- Silvermoon City


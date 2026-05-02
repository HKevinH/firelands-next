CREATE DATABASE IF NOT EXISTS `firelands_world`;
USE `firelands_world`;

-- Backfill migration: some installs may have `2_playercreateinfo.sql` marked as applied
-- while the tables were never created (splitter/comment issues, permissions, etc.).
-- This migration is idempotent and ensures the full set exists.

CREATE TABLE IF NOT EXISTS `playercreateinfo_stats` (
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `str` int(10) unsigned NOT NULL DEFAULT '0',
  `agi` int(10) unsigned NOT NULL DEFAULT '0',
  `sta` int(10) unsigned NOT NULL DEFAULT '0',
  `intel` int(10) unsigned NOT NULL DEFAULT '0',
  `spi` int(10) unsigned NOT NULL DEFAULT '0',
  `maxHealth` int(10) unsigned NOT NULL DEFAULT '100',
  `maxMana` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_spell` (
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `spellId` int(10) unsigned NOT NULL,
  PRIMARY KEY (`race`, `class`, `spellId`),
  KEY `idx_class` (`class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_skill` (
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `skillId` smallint(5) unsigned NOT NULL,
  `rank` smallint(5) unsigned NOT NULL DEFAULT '0',
  `maxRank` smallint(5) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `skillId`),
  KEY `idx_class` (`class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_faction` (
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `reputationListId` smallint(5) unsigned NOT NULL,
  `flags` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `standing` int(10) NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `reputationListId`),
  KEY `idx_class` (`class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_reputation` (
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `minLevel` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `maxLevel` tinyint(3) unsigned NOT NULL DEFAULT '255',
  `factionId` int(10) unsigned NOT NULL,
  `flags` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `standing` int(10) NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `minLevel`, `maxLevel`, `factionId`),
  KEY `idx_race_class` (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;


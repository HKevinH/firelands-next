USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `playercreateinfo_stats` (
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `str` int unsigned NOT NULL DEFAULT '0',
  `agi` int unsigned NOT NULL DEFAULT '0',
  `sta` int unsigned NOT NULL DEFAULT '0',
  `inte` int unsigned NOT NULL DEFAULT '0',
  `spi` int unsigned NOT NULL DEFAULT '0',
  `maxHealth` int unsigned NOT NULL DEFAULT '100',
  `maxMana` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_spell` (
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `spellId` int unsigned NOT NULL,
  PRIMARY KEY (`race`, `class`, `spellId`),
  KEY `idx_class` (`class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_skill` (
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `skillId` smallint unsigned NOT NULL,
  `rank` smallint unsigned NOT NULL DEFAULT '0',
  `maxRank` smallint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `skillId`),
  KEY `idx_class` (`class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_faction` (
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `reputationListId` smallint unsigned NOT NULL,
  `flags` tinyint unsigned NOT NULL DEFAULT '0',
  `standing` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `reputationListId`),
  KEY `idx_class` (`class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_reputation` (
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `minLevel` tinyint unsigned NOT NULL DEFAULT '1',
  `maxLevel` tinyint unsigned NOT NULL DEFAULT '255',
  `factionId` int unsigned NOT NULL,
  `flags` tinyint unsigned NOT NULL DEFAULT '0',
  `standing` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `minLevel`, `maxLevel`, `factionId`),
  KEY `idx_race_class` (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_visual_items` (
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `gender` tinyint unsigned NOT NULL DEFAULT '2',
  `outfitId` tinyint unsigned NOT NULL DEFAULT '0',
  `slot` tinyint unsigned NOT NULL,
  `invType` tinyint unsigned NOT NULL DEFAULT '0',
  `displayId` int unsigned NOT NULL DEFAULT '0',
  `displayEnchantId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `gender`, `outfitId`, `slot`),
  KEY `idx_class` (`class`),
  KEY `idx_race_class` (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_item` (
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `itemId` int unsigned NOT NULL DEFAULT '0',
  `amount` tinyint NOT NULL DEFAULT '1',
  PRIMARY KEY (`race`, `class`, `itemId`),
  KEY `idx_race_class` (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_action` (
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `spec` tinyint unsigned NOT NULL DEFAULT '0',
  `button` smallint unsigned NOT NULL DEFAULT '0',
  `action` int unsigned NOT NULL DEFAULT '0',
  `type` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`race`, `class`, `spec`, `button`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `player_classlevelstats` (
  `class` tinyint unsigned NOT NULL,
  `level` tinyint unsigned NOT NULL,
  `str` smallint unsigned NOT NULL DEFAULT 0,
  `agi` smallint unsigned NOT NULL DEFAULT 0,
  `sta` smallint unsigned NOT NULL DEFAULT 0,
  `inte` smallint unsigned NOT NULL DEFAULT 0,
  `spi` smallint unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`class`, `level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `player_racestats` (
  `race` tinyint unsigned NOT NULL,
  `str` smallint NOT NULL DEFAULT 0,
  `agi` smallint NOT NULL DEFAULT 0,
  `sta` smallint NOT NULL DEFAULT 0,
  `inte` smallint NOT NULL DEFAULT 0,
  `spi` smallint NOT NULL DEFAULT 0,
  PRIMARY KEY (`race`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `player_xp_for_level` (
  `level` tinyint unsigned NOT NULL,
  `experience` int unsigned NOT NULL,
  PRIMARY KEY (`level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `player_levelstats` (
  `level` tinyint unsigned NOT NULL DEFAULT '0',
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `str` smallint unsigned NOT NULL DEFAULT '0',
  `agi` smallint unsigned NOT NULL DEFAULT '0',
  `sta` smallint unsigned NOT NULL DEFAULT '0',
  `inte` smallint unsigned NOT NULL DEFAULT '0',
  `spi` smallint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`level`, `race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
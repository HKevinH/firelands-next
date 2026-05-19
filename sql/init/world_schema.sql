CREATE DATABASE IF NOT EXISTS `firelands_world`;
USE `firelands_world`;

-- World database usually contains static data like items, quests, etc.
-- We'll start with a small table for version tracking.
CREATE TABLE IF NOT EXISTS `version` (
  `core_version` varchar(120) NOT NULL,
  `db_version` varchar(120) DEFAULT NULL,
  PRIMARY KEY (`core_version`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT IGNORE INTO `version` (`core_version`, `db_version`) VALUES ('Firelands 4.3.4.15595', 'Initial Schema 2026-04-11');

-- Gossip menu definitions (maps menu ID to default NPC text).
CREATE TABLE IF NOT EXISTS `gossip_menu` (
  `MenuID` int unsigned NOT NULL DEFAULT '0',
  `TextID` int unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` smallint NOT NULL DEFAULT '0',
  PRIMARY KEY (`MenuID`, `TextID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Gossip menu options (individual items shown in a gossip menu).
CREATE TABLE IF NOT EXISTS `gossip_menu_option` (
  `MenuId` int unsigned NOT NULL DEFAULT '0',
  `OptionIndex` int unsigned NOT NULL DEFAULT '0',
  `OptionIcon` tinyint unsigned NOT NULL DEFAULT '0',
  `OptionText` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `OptionBroadcastTextId` int unsigned NOT NULL DEFAULT '0',
  `OptionType` int unsigned NOT NULL DEFAULT '0',
  `OptionNpcflag` bigint unsigned NOT NULL DEFAULT '0',
  `VerifiedBuild` smallint NOT NULL DEFAULT '0',
  PRIMARY KEY (`MenuId`, `OptionIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Gossip menu option action data (menu chaining, POI markers).
CREATE TABLE IF NOT EXISTS `gossip_menu_option_action` (
  `MenuId` int unsigned NOT NULL DEFAULT '0',
  `OptionIndex` int unsigned NOT NULL DEFAULT '0',
  `ActionMenuId` int unsigned NOT NULL DEFAULT '0',
  `ActionPoiId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`MenuId`, `OptionIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Gossip menu option box (password/money confirmation popups).
CREATE TABLE IF NOT EXISTS `gossip_menu_option_box` (
  `MenuId` int unsigned NOT NULL DEFAULT '0',
  `OptionIndex` int unsigned NOT NULL DEFAULT '0',
  `BoxCoded` tinyint unsigned NOT NULL DEFAULT '0',
  `BoxMoney` int unsigned NOT NULL DEFAULT '0',
  `BoxText` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `BoxBroadcastTextId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`MenuId`, `OptionIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `gossip_menu_option` (
  `menuId` int unsigned NOT NULL DEFAULT '0',
  `optionIndex` int unsigned NOT NULL DEFAULT '0',
  `optionIcon` tinyint unsigned NOT NULL DEFAULT '0',
  `optionText` text,
  `optionBroadcastTextId` int unsigned NOT NULL DEFAULT '0',
  `optionType` int unsigned NOT NULL DEFAULT '0',
  `optionNpcFlag` bigint unsigned NOT NULL DEFAULT '0',
  `verifiedBuild` smallint NOT NULL DEFAULT '0',
  PRIMARY KEY (`menuId`, `optionIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `gossip_menu_option_action` (
  `menuId` int unsigned NOT NULL DEFAULT '0',
  `optionIndex` int unsigned NOT NULL DEFAULT '0',
  `actionMenuId` int unsigned NOT NULL DEFAULT '0',
  `actionPoiId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`menuId`, `optionIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `gossip_menu_option_box` (
  `menuId` int unsigned NOT NULL DEFAULT '0',
  `optionIndex` int unsigned NOT NULL DEFAULT '0',
  `boxCoded` tinyint unsigned NOT NULL DEFAULT '0',
  `boxMoney` int unsigned NOT NULL DEFAULT '0',
  `boxText` text,
  `boxBroadcastTextId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`menuId`, `optionIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `item_proto_cache` (
  `entry` int(10) unsigned NOT NULL,
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `subclass` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `InventoryType` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `displayid` int(10) unsigned NOT NULL DEFAULT '0',
  `buy_count` int(11) NOT NULL DEFAULT '1',
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `playercreateinfo_item` (
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `itemid` int(10) unsigned NOT NULL DEFAULT '0',
  `amount` tinyint(4) NOT NULL DEFAULT '1',
  PRIMARY KEY (`race`,`class`,`itemid`),
  KEY `idx_race_class` (`race`,`class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

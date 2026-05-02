USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `item_template` (
  `entry` int(10) unsigned NOT NULL,
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `subclass` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `InventoryType` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `displayid` int(10) unsigned NOT NULL DEFAULT '0',
  `BuyCount` int(11) NOT NULL DEFAULT '1',
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

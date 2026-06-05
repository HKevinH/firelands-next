USE `firelands_world`;

-- Named teleport destinations for the `.tele <name>` command. Loaded into the
-- in-memory GameTeleStore at world startup. Cross-map aware (each row has its
-- own `map`). Import a full TrinityCore game_tele dump on top to replace these.
DROP TABLE IF EXISTS `game_tele`;
CREATE TABLE `game_tele` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `map` smallint unsigned NOT NULL DEFAULT '0',
  `name` varchar(100) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Tele Command';

INSERT INTO `game_tele`
  (`position_x`, `position_y`, `position_z`, `orientation`, `map`, `name`) VALUES
-- Alliance capitals
  (-8913.23,  554.633,  93.79,  0.64,   0, 'Stormwind'),
  (-4918.88, -940.406, 501.564, 0.00,   0, 'Ironforge'),
  ( 9947.52, 2482.73, 1316.21,  0.00,   1, 'Darnassus'),
-- Horde capitals
  ( 1676.21, -4315.29,  61.69,  0.00,   1, 'Orgrimmar'),
  (-1196.07,   29.13,  176.27,  0.00,   1, 'ThunderBluff'),
  ( 1633.75,  240.17,  -43.10,  0.00,   0, 'Undercity'),
-- Neutral hubs
  (-1838.16, 5301.79,  -12.42,  0.00, 530, 'Shattrath'),
  ( 5807.21,  588.42,  661.04,  0.00, 571, 'Dalaran'),
  (-14297.0,  553.0,     8.90,  0.00,   0, 'BootyBay'),
  (-7170.0, -3742.0,     8.50,  0.00,   1, 'Gadgetzan'),
-- Leveling zones (Outland ~58-68, includes level 65 content)
  (-1899.0, 8112.0,    23.00,  0.00, 530, 'Nagrand'),
  ( -350.0, 1106.0,    54.00,  0.00, 530, 'Hellfire'),
-- Starter areas
  (-9449.0,   64.0,    56.00,  0.00,   0, 'Elwynn'),
  ( -293.0, -3747.0,   31.00,  0.00,   1, 'Durotar');

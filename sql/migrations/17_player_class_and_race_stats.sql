-- Trinity-style primary attributes (world DB). HP/mana come from client gtOCT*.dbc
-- loaded by the world server; replace/extend rows from your reference world dump.
USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `player_classlevelstats` (
  `class` tinyint unsigned NOT NULL,
  `level` tinyint unsigned NOT NULL,
  `str` smallint unsigned NOT NULL DEFAULT 0,
  `agi` smallint unsigned NOT NULL DEFAULT 0,
  `sta` smallint unsigned NOT NULL DEFAULT 0,
  `inte` smallint unsigned NOT NULL DEFAULT 0,
  `spi` smallint unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`class`,`level`)
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

-- Minimal level-1 seed (WotLK-era ballpark; import full `player_classlevelstats` from
-- `firelands-cata-ref` / TrinityCataclysm world SQL for production parity).
INSERT IGNORE INTO `player_classlevelstats`
  (`class`, `level`, `str`, `agi`, `sta`, `inte`, `spi`) VALUES
(1,1,23,20,22,20,21),
(2,1,23,20,22,20,22),
(3,1,22,21,22,20,21),
(4,1,23,21,21,20,21),
(5,1,17,22,22,22,23),
(6,1,25,19,22,20,22),
(7,1,22,21,22,20,22),
(8,1,17,22,22,23,23),
(9,1,21,21,22,23,23),
(11,1,22,20,22,22,23);

INSERT IGNORE INTO `player_racestats`
  (`race`, `str`, `agi`, `sta`, `inte`, `spi`) VALUES
(1,0,0,0,0,0),(2,3,-3,3,-3,0),(3,0,0,1,0,0),(4,-4,2,0,0,0),
(5,0,0,0,0,0),(6,1,0,1,0,0),(7,-5,2,0,3,0),(8,1,2,0,0,0),
(9,0,0,0,0,0),(10,0,0,0,0,0),(11,0,0,0,2,0),(22,0,0,0,0,0);

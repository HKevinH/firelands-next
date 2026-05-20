-- Rebuild spell/skill tables when an older `race`/`class` schema is still present.
-- NOTE: This drops the tables and leaves them empty. Migration 45 re-inserts all
-- data after this schema is in place. Do not insert data here.
USE `firelands_world`;

DROP TABLE IF EXISTS `playercreateinfo_spell`;
DROP TABLE IF EXISTS `playercreateinfo_skill`;

CREATE TABLE `playercreateinfo_spell` (
  `raceMask` int unsigned NOT NULL DEFAULT 0,
  `classMask` int unsigned NOT NULL DEFAULT 0,
  `spellId` int unsigned NOT NULL,
  PRIMARY KEY (`raceMask`,`classMask`,`spellId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `playercreateinfo_skill` (
  `raceMask` int unsigned NOT NULL DEFAULT 0,
  `classMask` int unsigned NOT NULL DEFAULT 0,
  `skillId` smallint unsigned NOT NULL,
  `rank` smallint unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`raceMask`,`classMask`,`skillId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

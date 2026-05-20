-- Rebuild playercreateinfo_spell / playercreateinfo_skill with raceMask/classMask columns.
-- Use when migrations fail with: Unknown column 'raceMask' in 'field list'
--
-- Docker (repo root):
--   docker compose exec -T db mysql -uroot -proot < tools/sql/fix_playercreateinfo_spell_skill_schema.sql
--
-- Local:
--   mysql -h127.0.0.1 -ufirelands -pfirelands < tools/sql/fix_playercreateinfo_spell_skill_schema.sql

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

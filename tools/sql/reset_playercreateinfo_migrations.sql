-- Drop legacy spell/skill tables, recreate raceMask/classMask schema, and unmark
-- migrations so the next world/auth startup re-runs 42 + 43.
--
-- Docker (repo root):
--   docker compose exec -T db mysql -uroot -proot < tools/sql/reset_playercreateinfo_migrations.sql
--
-- Local:
--   mysql -h127.0.0.1 -ufirelands -pfirelands < tools/sql/reset_playercreateinfo_migrations.sql
--
-- Then restart `world` to apply migration 43 (starter spell/skill data).

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

DELETE FROM `firelands_auth`.`schema_migrations`
WHERE `migration` IN (
  '42_world_playercreateinfo_tables.sql',
  '43_world_playercreateinfo_data.sql'
);

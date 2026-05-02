-- Money per character, extra learned spells, account login lock (ban).
-- Fully-qualified table names (no USE). Column locked is placed after expansion
-- so this file applies even if 15_account_access_level.sql was never run.

ALTER TABLE `firelands_auth`.`account`
  ADD COLUMN `locked` tinyint unsigned NOT NULL DEFAULT '0'
  AFTER `expansion`;

ALTER TABLE `firelands_characters`.`characters`
  ADD COLUMN `money` int unsigned NOT NULL DEFAULT '0'
  AFTER `firstLogin`;

CREATE TABLE IF NOT EXISTS `firelands_characters`.`character_spell` (
  `guid` int unsigned NOT NULL,
  `spell` int unsigned NOT NULL,
  PRIMARY KEY (`guid`, `spell`),
  KEY `idx_guid` (`guid`),
  CONSTRAINT `fk_character_spell_guid` FOREIGN KEY (`guid`) REFERENCES `firelands_characters`.`characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Minimal quest progress tables for phase condition evaluation.
USE `firelands_characters`;

CREATE TABLE IF NOT EXISTS `character_queststatus` (
  `guid` int unsigned NOT NULL,
  `quest` int unsigned NOT NULL DEFAULT '0',
  `status` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`, `quest`),
  KEY `idx_guid` (`guid`),
  CONSTRAINT `fk_character_queststatus_guid`
    FOREIGN KEY (`guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `character_queststatus_rewarded` (
  `guid` int unsigned NOT NULL,
  `quest` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`, `quest`),
  KEY `idx_guid` (`guid`),
  CONSTRAINT `fk_character_queststatus_rewarded_guid`
    FOREIGN KEY (`guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

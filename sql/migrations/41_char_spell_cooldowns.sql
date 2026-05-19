-- Migration: character spell cooldown persistence
-- Target: `firelands_characters` — per-character spell/category recovery timers

CREATE DATABASE IF NOT EXISTS `firelands_characters`;
USE `firelands_characters`;

CREATE TABLE IF NOT EXISTS `character_spell_cooldown` (
  `guid` int unsigned NOT NULL,
  `spell` int unsigned NOT NULL,
  `remaining_ms` int unsigned NOT NULL,
  PRIMARY KEY (`guid`, `spell`),
  KEY `idx_guid` (`guid`),
  CONSTRAINT `fk_character_spell_cooldown_guid` FOREIGN KEY (`guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `character_spell_category_cooldown` (
  `guid` int unsigned NOT NULL,
  `category` int unsigned NOT NULL,
  `remaining_ms` int unsigned NOT NULL,
  PRIMARY KEY (`guid`, `category`),
  KEY `idx_guid` (`guid`),
  CONSTRAINT `fk_character_spell_category_cooldown_guid` FOREIGN KEY (`guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

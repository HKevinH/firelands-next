CREATE DATABASE IF NOT EXISTS `firelands_characters`;
USE `firelands_characters`;

-- Persist client-selected outfitId (CMSG_CHAR_CREATE). Idempotent: safe if Docker
-- init already created this column (characters_schema.sql) or migration re-run.

SET @exist :=
  (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'characters'
     AND COLUMN_NAME = 'outfitId');

SET @sqlstmt := IF(@exist = 0,
  'ALTER TABLE `characters` ADD COLUMN `outfitId` tinyint unsigned NOT NULL DEFAULT 0 AFTER `facialHair`',
  'SELECT 1');

PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

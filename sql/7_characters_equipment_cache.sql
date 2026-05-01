CREATE DATABASE IF NOT EXISTS `firelands_characters`;
USE `firelands_characters`;

-- FirelandsCore-style equipment cache string for CHAR_ENUM / visible items.
-- Idempotent: skip ADD if column already present.

SET @exist :=
  (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'characters'
     AND COLUMN_NAME = 'equipmentCache');

SET @sqlstmt := IF(@exist = 0,
  'ALTER TABLE `characters` ADD COLUMN `equipmentCache` mediumtext NULL AFTER `outfitId`',
  'SELECT 1');

PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

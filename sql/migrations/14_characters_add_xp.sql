-- Character experience (persisted for PLAYER_XP / logout save).
CREATE DATABASE IF NOT EXISTS `firelands_characters`;
USE `firelands_characters`;

SET @exist :=
  (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'characters'
     AND COLUMN_NAME = 'xp');

SET @sqlstmt := IF(@exist = 0,
  'ALTER TABLE `characters` ADD COLUMN `xp` int unsigned NOT NULL DEFAULT 0 AFTER `money`',
  'SELECT 1');

PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Persist last known primary resource (health, POWER1) between sessions so
-- SpellManager runtime changes survive relog (Phase D follow-up).

USE `firelands_characters`;

SET @exist :=
  (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'characters'
     AND COLUMN_NAME = 'live_health');

SET @sqlstmt := IF(@exist = 0,
  'ALTER TABLE `characters` ADD COLUMN `live_health` int unsigned NULL DEFAULT NULL AFTER `xp`',
  'SELECT 1');

PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @exist :=
  (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'characters'
     AND COLUMN_NAME = 'live_power1');

SET @sqlstmt := IF(@exist = 0,
  'ALTER TABLE `characters` ADD COLUMN `live_power1` int unsigned NULL DEFAULT NULL AFTER `live_health`',
  'SELECT 1');

PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

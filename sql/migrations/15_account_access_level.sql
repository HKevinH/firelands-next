-- Adds staff / privilege tier for each auth account (see shared/game/AccessLevel.h).
USE `firelands_auth`;

ALTER TABLE `account`
  ADD COLUMN `access_level` tinyint unsigned NOT NULL DEFAULT '0'
  AFTER `expansion`;

-- Merged auth migrations (Firelands Next)
-- Execution order: 3 -> 15 -> 16

-- === 3_fix_account_data.sql ===
CREATE DATABASE IF NOT EXISTS `firelands_auth`;
USE `firelands_auth`;

CREATE TABLE IF NOT EXISTS `account_data` (
  `accountId` int(10) unsigned NOT NULL,
  `type` tinyint(3) unsigned NOT NULL,
  `time` int(10) unsigned NOT NULL DEFAULT '0',
  `data` blob NOT NULL,
  PRIMARY KEY (`accountId`, `type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- === 15_account_access_level.sql ===
ALTER TABLE `account`
  ADD COLUMN `access_level` tinyint unsigned NOT NULL DEFAULT '0'
  AFTER `expansion`;

-- === 16_gameplay_money_spells_account_lock.sql (auth portion) ===
ALTER TABLE `firelands_auth`.`account`
  ADD COLUMN `locked` tinyint unsigned NOT NULL DEFAULT '0'
  AFTER `expansion`;
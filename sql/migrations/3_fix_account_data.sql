CREATE DATABASE IF NOT EXISTS `firelands_auth`;
USE `firelands_auth`;

-- Safety net: some installs may have marked auth_schema.sql as applied while the
-- table was never created (permissions/partial migration). This migration is
-- idempotent and ensures the table exists.
CREATE TABLE IF NOT EXISTS `account_data` (
  `accountId` int(10) unsigned NOT NULL,
  `type` tinyint(3) unsigned NOT NULL,
  `time` int(10) unsigned NOT NULL DEFAULT '0',
  `data` blob NOT NULL,
  PRIMARY KEY (`accountId`, `type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;


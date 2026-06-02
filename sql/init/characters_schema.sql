CREATE DATABASE IF NOT EXISTS `firelands_characters`;
USE `firelands_characters`;

CREATE TABLE IF NOT EXISTS `characters` (
  `guid` int unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int unsigned NOT NULL DEFAULT '0',
  `name` varchar(12) NOT NULL DEFAULT '',
  `race` tinyint unsigned NOT NULL DEFAULT '0',
  `class` tinyint unsigned NOT NULL DEFAULT '0',
  `gender` tinyint unsigned NOT NULL DEFAULT '0',
  `skin` tinyint unsigned NOT NULL DEFAULT '0',
  `face` tinyint unsigned NOT NULL DEFAULT '0',
  `hairStyle` tinyint unsigned NOT NULL DEFAULT '0',
  `hairColor` tinyint unsigned NOT NULL DEFAULT '0',
  `facialHair` tinyint unsigned NOT NULL DEFAULT '0',
  `outfitId` tinyint unsigned NOT NULL DEFAULT '0',
  `equipmentCache` mediumtext,
  `level` tinyint unsigned NOT NULL DEFAULT '1',
  `zoneId` smallint unsigned NOT NULL DEFAULT '0',
  `mapId` smallint unsigned NOT NULL DEFAULT '0',
  `x` float NOT NULL DEFAULT '0',
  `y` float NOT NULL DEFAULT '0',
  `z` float NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `guildId` int unsigned NOT NULL DEFAULT '0',
  `characterFlags` int unsigned NOT NULL DEFAULT '0',
  `customizationFlags` int unsigned NOT NULL DEFAULT '0',
  `firstLogin` tinyint unsigned NOT NULL DEFAULT '1',
  `money` int unsigned NOT NULL DEFAULT '0',
  `xp` int unsigned NOT NULL DEFAULT '0',
  `restBonus` float NOT NULL DEFAULT '0',
  `liveHealth` int unsigned DEFAULT NULL,
  `livePower1` int unsigned DEFAULT NULL,
  `tutorial0` int unsigned NOT NULL DEFAULT '0',
  `tutorial1` int unsigned NOT NULL DEFAULT '0',
  `tutorial2` int unsigned NOT NULL DEFAULT '0',
  `tutorial3` int unsigned NOT NULL DEFAULT '0',
  `tutorial4` int unsigned NOT NULL DEFAULT '0',
  `tutorial5` int unsigned NOT NULL DEFAULT '0',
  `tutorial6` int unsigned NOT NULL DEFAULT '0',
  `tutorial7` int unsigned NOT NULL DEFAULT '0',
  `actionBarToggles` tinyint unsigned NOT NULL DEFAULT '255',
  PRIMARY KEY (`guid`),
  KEY `idx_account` (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_account_data` (
  `character_guid` int unsigned NOT NULL,
  `type` tinyint unsigned NOT NULL,
  `time` int unsigned NOT NULL DEFAULT '0',
  `data` blob NOT NULL,
  PRIMARY KEY (`character_guid`, `type`),
  CONSTRAINT `fk_character_account_data_character`
    FOREIGN KEY (`character_guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_spell` (
  `character_guid` int unsigned NOT NULL,
  `spell_id` int unsigned NOT NULL,
  PRIMARY KEY (`character_guid`, `spell_id`),
  KEY `idx_character` (`character_guid`),
  CONSTRAINT `fk_character_spell_character`
    FOREIGN KEY (`character_guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_spell_cooldown` (
  `character_guid` int unsigned NOT NULL,
  `spell_id` int unsigned NOT NULL,
  `remainingMs` int unsigned NOT NULL,
  PRIMARY KEY (`character_guid`, `spell_id`),
  KEY `idx_character` (`character_guid`),
  CONSTRAINT `fk_character_spell_cooldown_character`
    FOREIGN KEY (`character_guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_spell_category_cooldown` (
  `character_guid` int unsigned NOT NULL,
  `category` int unsigned NOT NULL,
  `remainingMs` int unsigned NOT NULL,
  PRIMARY KEY (`character_guid`, `category`),
  KEY `idx_character` (`character_guid`),
  CONSTRAINT `fk_character_spell_category_cooldown_character`
    FOREIGN KEY (`character_guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_action` (
  `character_guid` int unsigned NOT NULL,
  `spec` tinyint unsigned NOT NULL DEFAULT '0',
  `button` tinyint unsigned NOT NULL,
  `action` int unsigned NOT NULL DEFAULT '0',
  `actionType` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`character_guid`, `spec`, `button`),
  KEY `idx_character` (`character_guid`),
  CONSTRAINT `fk_character_action_character`
    FOREIGN KEY (`character_guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `gm_ticket` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int unsigned NOT NULL,
  `character_guid` int unsigned NOT NULL,
  `status` tinyint unsigned NOT NULL DEFAULT '0',
  `category` tinyint unsigned NOT NULL DEFAULT '0',
  `needMoreHelp` tinyint unsigned NOT NULL DEFAULT '0',
  `message` text NOT NULL,
  `gmResponse` text,
  `mapId` smallint unsigned NOT NULL DEFAULT '0',
  `posX` float NOT NULL DEFAULT '0',
  `posY` float NOT NULL DEFAULT '0',
  `posZ` float NOT NULL DEFAULT '0',
  `assignedAccountId` int unsigned DEFAULT NULL,
  `createdAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updatedAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `assignedAt` timestamp NULL DEFAULT NULL,
  `closedAt` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_character_open` (`character_guid`, `status`),
  KEY `idx_queue` (`status`, `createdAt`),
  KEY `idx_assigned` (`assignedAccountId`, `status`),
  CONSTRAINT `fk_gm_ticket_character`
    FOREIGN KEY (`character_guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `mail` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `receiverGuid` int unsigned NOT NULL,
  `senderGuid` int unsigned NOT NULL DEFAULT '0',
  `subject` varchar(200) NOT NULL DEFAULT 'Item delivery',
  `body` text,
  `deliverTime` int unsigned NOT NULL DEFAULT '0',
  `expireTime` int unsigned NOT NULL DEFAULT '0',
  `checked` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_receiver` (`receiverGuid`),
  CONSTRAINT `fk_mail_receiver`
    FOREIGN KEY (`receiverGuid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `mail_items` (
  `mailId` bigint unsigned NOT NULL,
  `itemGuid` int unsigned NOT NULL,
  `receiverGuid` int unsigned NOT NULL,
  PRIMARY KEY (`itemGuid`),
  KEY `idx_mail` (`mailId`),
  CONSTRAINT `fk_mail_items_mail`
    FOREIGN KEY (`mailId`) REFERENCES `mail` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_mail_items_receiver`
    FOREIGN KEY (`receiverGuid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `instance` (
  `id` int unsigned NOT NULL DEFAULT '0',
  `mapId` smallint unsigned NOT NULL DEFAULT '0',
  `resetTime` bigint unsigned NOT NULL DEFAULT '0',
  `difficulty` tinyint unsigned NOT NULL DEFAULT '0',
  `completedEncounters` int unsigned NOT NULL DEFAULT '0',
  `data` tinytext NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_map` (`mapId`),
  KEY `idx_resettime` (`resetTime`),
  KEY `idx_difficulty` (`difficulty`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `instance_reset` (
  `mapId` smallint unsigned NOT NULL DEFAULT '0',
  `difficulty` tinyint unsigned NOT NULL DEFAULT '0',
  `resetTime` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`mapId`, `difficulty`),
  KEY `idx_difficulty` (`difficulty`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_instance` (
  `character_guid` int unsigned NOT NULL DEFAULT '0',
  `instanceId` int unsigned NOT NULL DEFAULT '0',
  `permanent` tinyint unsigned NOT NULL DEFAULT '0',
  `extendState` tinyint unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`character_guid`, `instanceId`),
  KEY `idx_instance` (`instanceId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `group_instance` (
  `guid` int unsigned NOT NULL DEFAULT '0',
  `instanceId` int unsigned NOT NULL DEFAULT '0',
  `permanent` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`, `instanceId`),
  KEY `idx_instance` (`instanceId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `account_instance_times` (
  `accountId` int unsigned NOT NULL,
  `instanceId` int unsigned NOT NULL DEFAULT '0',
  `releaseTime` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`accountId`, `instanceId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `item_refund_instance` (
  `itemGuid` int unsigned NOT NULL,
  `playerGuid` int unsigned NOT NULL,
  `paidMoney` int unsigned NOT NULL DEFAULT '0',
  `paidExtendedCost` smallint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`itemGuid`, `playerGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `schema_migrations` (
  `migration` varchar(255) NOT NULL,
  `applied_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`migration`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
-- GM / player help tickets (Cataclysm 4.3.4 client UI + server persistence).
-- Fully-qualified names (no USE). Lives in `firelands_characters` with characters.

CREATE TABLE IF NOT EXISTS `firelands_characters`.`gm_ticket` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int unsigned NOT NULL COMMENT 'Player account that opened the ticket',
  `character_guid` int unsigned NOT NULL,
  `status` tinyint unsigned NOT NULL DEFAULT '0' COMMENT '0=open 1=assigned 2=gm_answered 3=closed_resolved 4=closed_abandoned 5=closed_staff',
  `category` tinyint unsigned NOT NULL DEFAULT '0',
  `need_more_help` tinyint unsigned NOT NULL DEFAULT '0',
  `message` text NOT NULL,
  `gm_response` text NULL COMMENT 'Last GM reply shown to player when status is gm_answered',
  `map_id` smallint unsigned NOT NULL DEFAULT '0',
  `pos_x` float NOT NULL DEFAULT '0',
  `pos_y` float NOT NULL DEFAULT '0',
  `pos_z` float NOT NULL DEFAULT '0',
  `assigned_account_id` int unsigned NULL COMMENT 'Staff account.id currently handling the ticket',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `assigned_at` timestamp NULL DEFAULT NULL,
  `closed_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_character_open` (`character_guid`, `status`),
  KEY `idx_queue` (`status`, `created_at`),
  KEY `idx_assigned` (`assigned_account_id`, `status`),
  CONSTRAINT `fk_gm_ticket_character` FOREIGN KEY (`character_guid`) REFERENCES `firelands_characters`.`characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

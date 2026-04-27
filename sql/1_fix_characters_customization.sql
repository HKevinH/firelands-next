-- Fix characters table structure for Cataclysm 4.3.4 (Build 15595)
-- Adding missing customization columns

USE `firelands_characters`;

ALTER TABLE `characters` 
ADD COLUMN `skin` tinyint(3) unsigned NOT NULL DEFAULT '0' AFTER `gender`,
ADD COLUMN `face` tinyint(3) unsigned NOT NULL DEFAULT '0' AFTER `skin`,
ADD COLUMN `hairStyle` tinyint(3) unsigned NOT NULL DEFAULT '0' AFTER `face`,
ADD COLUMN `hairColor` tinyint(3) unsigned NOT NULL DEFAULT '0' AFTER `hairStyle`,
ADD COLUMN `facialHair` tinyint(3) unsigned NOT NULL DEFAULT '0' AFTER `hairColor`,
ADD COLUMN `customizationFlags` int(10) unsigned NOT NULL DEFAULT '0' AFTER `characterFlags`;

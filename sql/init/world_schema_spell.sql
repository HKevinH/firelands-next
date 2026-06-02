USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `spell_dbc` (
  `id` int unsigned NOT NULL,
  `attributes` int unsigned NOT NULL DEFAULT 0,
  `attributesEx` int unsigned NOT NULL DEFAULT 0,
  `attributesEx2` int unsigned NOT NULL DEFAULT 0,
  `attributesEx3` int unsigned NOT NULL DEFAULT 0,
  `attributesEx4` int unsigned NOT NULL DEFAULT 0,
  `attributesEx5` int unsigned NOT NULL DEFAULT 0,
  `attributesEx6` int unsigned NOT NULL DEFAULT 0,
  `attributesEx7` int unsigned NOT NULL DEFAULT 0,
  `attributesEx8` int unsigned NOT NULL DEFAULT 0,
  `attributesEx9` int unsigned NOT NULL DEFAULT 0,
  `attributesEx10` int unsigned NOT NULL DEFAULT 0,
  `castingTimeIndex` int unsigned NOT NULL DEFAULT 1,
  `durationIndex` int unsigned NOT NULL DEFAULT 0,
  `rangeIndex` int unsigned NOT NULL DEFAULT 1,
  `schoolMask` int unsigned NOT NULL DEFAULT 0,
  `spellAuraOptionsId` int unsigned NOT NULL DEFAULT 0,
  `spellCastingRequirementsId` int unsigned NOT NULL DEFAULT 0,
  `spellCategoriesId` int unsigned NOT NULL DEFAULT 0,
  `spellClassOptionsId` int unsigned NOT NULL DEFAULT 0,
  `spellEquippedItemsId` int unsigned NOT NULL DEFAULT 0,
  `spellInterruptsId` int unsigned NOT NULL DEFAULT 0,
  `spellLevelsId` int unsigned NOT NULL DEFAULT 0,
  `spellTargetRestrictionsId` int unsigned NOT NULL DEFAULT 0,
  `powerType` int unsigned DEFAULT NULL,
  `ovAttributes` int unsigned DEFAULT NULL,
  `ovCastingTimeIndex` int unsigned DEFAULT NULL,
  `ovDurationIndex` int unsigned DEFAULT NULL,
  `ovRangeIndex` int unsigned DEFAULT NULL,
  `ovSchoolMask` int unsigned DEFAULT NULL,
  `spellName` varchar(128) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spelleffect_dbc` (
  `id` int unsigned NOT NULL,
  `effect` int unsigned NOT NULL DEFAULT 0,
  `effectAmplitude` float NOT NULL DEFAULT 0,
  `effectAura` int unsigned NOT NULL DEFAULT 0,
  `effectAuraPeriod` int unsigned NOT NULL DEFAULT 0,
  `effectBasePoints` int NOT NULL DEFAULT 0,
  `effectBonusCoefficient` float NOT NULL DEFAULT 0,
  `effectChainAmplitude` float NOT NULL DEFAULT 0,
  `effectChainTargets` int unsigned NOT NULL DEFAULT 0,
  `effectDieSides` int NOT NULL DEFAULT 0,
  `effectItemType` int unsigned NOT NULL DEFAULT 0,
  `effectMechanic` int unsigned NOT NULL DEFAULT 0,
  `effectMiscValue` int NOT NULL DEFAULT 0,
  `effectMiscValueB` int NOT NULL DEFAULT 0,
  `effectPointsPerResource` float NOT NULL DEFAULT 0,
  `effectRadiusIndex` int unsigned NOT NULL DEFAULT 0,
  `effectRadiusMaxIndex` int unsigned NOT NULL DEFAULT 0,
  `effectRealPointsPerLevel` float NOT NULL DEFAULT 0,
  `effectSpellClassMaskA` int unsigned NOT NULL DEFAULT 0,
  `effectSpellClassMaskB` int unsigned NOT NULL DEFAULT 0,
  `effectSpellClassMaskC` int unsigned NOT NULL DEFAULT 0,
  `effectTriggerSpell` int unsigned NOT NULL DEFAULT 0,
  `effectImplicitTargetA` int unsigned NOT NULL DEFAULT 0,
  `effectImplicitTargetB` int unsigned NOT NULL DEFAULT 0,
  `spellId` int unsigned NOT NULL,
  `effectIndex` int unsigned NOT NULL DEFAULT 0,
  `comment` varchar(128) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_area` (
  `spell` int unsigned NOT NULL DEFAULT 0,
  `area` int unsigned NOT NULL DEFAULT 0,
  `questStart` int unsigned NOT NULL DEFAULT 0,
  `questEnd` int unsigned NOT NULL DEFAULT 0,
  `auraSpell` int NOT NULL DEFAULT 0,
  `raceMask` int unsigned NOT NULL DEFAULT 0,
  `gender` tinyint unsigned NOT NULL DEFAULT 2,
  `flags` tinyint unsigned NOT NULL DEFAULT 3,
  `questStartStatus` int NOT NULL DEFAULT 64,
  `questEndStatus` int NOT NULL DEFAULT 11,
  PRIMARY KEY (`spell`, `area`, `questStart`, `auraSpell`, `raceMask`, `gender`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_group` (
  `id` int unsigned NOT NULL DEFAULT 0,
  `spellId` int NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`, `spellId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_group_stack_rules` (
  `groupId` int unsigned NOT NULL DEFAULT 0,
  `stackRule` tinyint NOT NULL DEFAULT 0,
  PRIMARY KEY (`groupId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_learn_spell` (
  `entry` int unsigned NOT NULL DEFAULT 0,
  `spellId` int unsigned NOT NULL DEFAULT 0,
  `active` tinyint unsigned NOT NULL DEFAULT 1,
  PRIMARY KEY (`entry`, `spellId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_linked_spell` (
  `spellTrigger` int NOT NULL,
  `spellEffect` int NOT NULL DEFAULT 0,
  `type` tinyint unsigned NOT NULL DEFAULT 0,
  `comment` text NOT NULL,
  UNIQUE KEY `uk_trigger_effect_type` (`spellTrigger`, `spellEffect`, `type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_proc` (
  `spellId` int NOT NULL DEFAULT 0,
  `schoolMask` tinyint unsigned NOT NULL DEFAULT 0,
  `spellFamilyName` smallint unsigned NOT NULL DEFAULT 0,
  `spellFamilyMask0` int unsigned NOT NULL DEFAULT 0,
  `spellFamilyMask1` int unsigned NOT NULL DEFAULT 0,
  `spellFamilyMask2` int unsigned NOT NULL DEFAULT 0,
  `procFlags` int unsigned NOT NULL DEFAULT 0,
  `spellTypeMask` int unsigned NOT NULL DEFAULT 0,
  `spellPhaseMask` int unsigned NOT NULL DEFAULT 0,
  `hitMask` int unsigned NOT NULL DEFAULT 0,
  `attributesMask` int unsigned NOT NULL DEFAULT 0,
  `disableEffectsMask` int unsigned NOT NULL DEFAULT 0,
  `procsPerMinute` float NOT NULL DEFAULT 0,
  `chance` float NOT NULL DEFAULT 0,
  `cooldown` int unsigned NOT NULL DEFAULT 0,
  `charges` tinyint unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`spellId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_rank` (
  `firstSpellId` int unsigned NOT NULL DEFAULT 0,
  `spellId` int unsigned NOT NULL DEFAULT 0,
  `rank` tinyint unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`firstSpellId`, `rank`),
  UNIQUE KEY `uk_spell_id` (`spellId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_required` (
  `spellId` int NOT NULL DEFAULT 0,
  `reqSpell` int NOT NULL DEFAULT 0,
  PRIMARY KEY (`spellId`, `reqSpell`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_script_name` (
  `spellId` int NOT NULL,
  `scriptName` varchar(64) NOT NULL,
  UNIQUE KEY `uk_spell_id_script` (`spellId`, `scriptName`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spell_target_position` (
  `id` int unsigned NOT NULL DEFAULT 0,
  `effectIndex` tinyint unsigned NOT NULL DEFAULT 0,
  `mapId` smallint unsigned NOT NULL DEFAULT 0,
  `positionX` float NOT NULL DEFAULT 0,
  `positionY` float NOT NULL DEFAULT 0,
  `positionZ` float NOT NULL DEFAULT 0,
  `orientation` float NOT NULL DEFAULT 0,
  `verifiedBuild` smallint DEFAULT 0,
  PRIMARY KEY (`id`, `effectIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `spelldifficulty_dbc` (
  `id` int unsigned NOT NULL DEFAULT 0,
  `spellId0` int unsigned NOT NULL DEFAULT 0,
  `spellId1` int unsigned NOT NULL DEFAULT 0,
  `spellId2` int unsigned NOT NULL DEFAULT 0,
  `spellId3` int unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
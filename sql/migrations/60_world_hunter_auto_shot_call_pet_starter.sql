-- Hunter level-1 Auto Shot (75) and Call Pet (883): ref grants these via
-- Player::LearnSkillRewardedSpells when class-tab skills are learned at create.
-- Rows here keep spellbooks correct when DBC merge is unavailable.
USE `firelands_world`;

INSERT IGNORE INTO `playercreateinfo_spell` (`raceMask`, `classMask`, `spellId`)
VALUES (0, 4, 75),
       (0, 4, 883);

USE `firelands_auth`;

INSERT IGNORE INTO `schema_migrations` (`migration`) VALUES
  ('auth_schema.sql');

INSERT IGNORE INTO `rbac_role` (`name`, `permission_mask`) VALUES
  ('moderator', 517),
  ('gamemaster', 975),
  ('administrator', 1023);
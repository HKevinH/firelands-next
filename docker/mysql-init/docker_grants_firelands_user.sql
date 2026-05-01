-- Applied by Docker MySQL entrypoint (root) and by `docker compose --profile migrate`.
-- Do NOT place this file under sql/: worldserver runs migrations as user `firelands`,
-- which cannot execute GRANT.

GRANT ALL PRIVILEGES ON `firelands_auth`.* TO 'firelands'@'%';
GRANT ALL PRIVILEGES ON `firelands_characters`.* TO 'firelands'@'%';
GRANT ALL PRIVILEGES ON `firelands_world`.* TO 'firelands'@'%';
GRANT ALL PRIVILEGES ON `firelands`.* TO 'firelands'@'%';
FLUSH PRIVILEGES;

CREATE DATABASE IF NOT EXISTS `firelands_auth`;
CREATE DATABASE IF NOT EXISTS `firelands_characters`;
CREATE DATABASE IF NOT EXISTS `firelands_world`;

GRANT ALL PRIVILEGES ON `firelands_auth`.* TO 'firelands'@'%';
GRANT ALL PRIVILEGES ON `firelands_characters`.* TO 'firelands'@'%';
GRANT ALL PRIVILEGES ON `firelands_world`.* TO 'firelands'@'%';
FLUSH PRIVILEGES;

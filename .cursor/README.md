# Cursor project configuration

## Slash commands (`commands/`)

Cursor loads Markdown files from `.cursor/commands/` when you type `/` in chat.

| Command | File | Description |
|---------|------|-------------|
| `/auto-commit` | [commands/auto-commit.md](commands/auto-commit.md) | Grouped Conventional Commits; Firelands SQL merge + `db_version` bumps |

**Do not delete** `commands/auto-commit.md` — it is the project-registered command (also mirrored at `~/.cursor/commands/auto-commit.md` on your machine).

Global commands live in `~/.cursor/commands/` and appear alongside project commands in the `/` menu.

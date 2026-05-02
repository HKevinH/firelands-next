# GM administration and staff commands

This document describes how Firelands handles **Game Master (GM) tooling**, **staff chat commands** (messages starting with `.`), and the **world server interactive console**. It maps the main types and files so you can extend privileges or add commands safely.

## High-level architecture

| Concern | Location | Role |
|--------|----------|------|
| Stored account tier | `shared/game/AccessLevel.h` | Values persisted in `account.access_level` (0 = player … 3 = administrator). `AccessLevel::Console` exists only at runtime for the server REPL. |
| Effective privilege | `AccessLevel.h` + `PrivilegeOrigin` | `EffectiveAccess()` upgrades the acting level to **Console** when `PrivilegeOrigin::ServerConsole` is used (world terminal), so console commands are not limited by the stub session’s stored level. |
| Fine-grained caps | `shared/game/Permissions.h` | `Permission` bitmask + `DefaultPermissions(AccessLevel)` + `HasPermission()`. Add new bits here and wire them in `CommandService`. |
| Command execution | `application/services/CommandService.{h,cpp}` | Parses `.command args…`, checks `consoleOnly`, permissions, and optional **console-first character name** for remote targeting. |
| Session abstraction | `application/ports/ICommandSession.h` | Minimal port: notifications, position, teleport, GM appearance hooks, gameplay helpers (`GmLearnSpell`, etc.). |
| Online player index | `application/services/OnlineCharacterSessionRegistry.{h,cpp}` | Thread-safe map **lowercased character name → weak `ICommandSession`**. Used for `.online`, `.kick`, `.goto`, `.summon`, `.announce`, and console `.tele` / `.gps` / gameplay-on-target. |
| Client session | `infrastructure/network/sessions/WorldSession.{h,cpp}` | Implements `ICommandSession`; registers/unregisters in `OnlineCharacterSessionRegistry` on world entry / disconnect. |
| GM visuals & movement | `WorldSessionGmState.cpp`, `shared/game/PlayerGmAppearance.{h,cpp}`, `shared/network/MovementSetPackets.h` | Updates `PLAYER_FLAGS` / `UNIT_FIELD_FLAGS` for tags and invisibility; broadcasts Cataclysm 4.3.4 **move set** packets for fly mode and speed. |
| World stdin / TUI | `world/WorldInteractiveConsole.{h,cpp}` | Queues lines; runs `CommandService::ExecuteCommand(..., PrivilegeOrigin::ServerConsole)`. Uses an internal `ServerConsoleCommandSession` stub (no in-world character). |
| Wiring | `world/main.cpp` | Constructs `OnlineCharacterSessionRegistry`, `CommandService(onlineCharRegistry, accountRepo)`, injects both into `WorldSession`. |

**Dependency direction:** command logic lives in **application**; **infrastructure** only implements `ICommandSession` and forwards chat to `ICommandService`.

## How commands enter the system

### In-game (chat)

`WorldSessionChatHandlers.cpp` inspects the decoded chat message. If `ICommandService::IsCommand(message)` is true (message starts with `.`), it calls `ExecuteCommand` with `PrivilegeOrigin::GameClient` **before** normal chat broadcast.

Important behavior in `CommandService::ExecuteCommand`:

- For **game clients**, only accounts with **`AccessLevel::GameMaster` or higher** may use dot commands. Moderators and players get **no response** (including no “unknown command” message), by design.
- After that gate, each registered command checks **`HasPermission`** against `DefaultPermissions(EffectiveAccess(account, origin))`.

### World server console

`WorldInteractiveConsole` only processes lines that pass `IsCommand` (leading `.`), except `quit` / `exit` (with or without a dot) which request process shutdown. Valid commands run with **`PrivilegeOrigin::ServerConsole`**, so **`EffectiveAccess` is `Console`** and all permission bits are granted unless a handler is marked **`consoleOnly`** (those still require console origin, not “extra” bits).

Some commands need a **target online character** when run from the console: see **Console argument layouts** below.

## Console argument layouts (`ConsoleArgLayout`)

Defined on `CommandService::CommandEntry`:

- **`SameAsInGame`** — arguments match the in-game form (e.g. `.account create …`).
- **`TargetOnlineCharacterFirst`** — from the console, the **first** token is an online **character name**, then the same arguments as in-game. Example: `.tele Annabell -8759 544 97` or `.learn Annabell 475`. `CommandService` wraps the target session in **`DelegatingCommandSession`**: gameplay and teleport apply to the **target**, while `SendNotification` goes to the **operator** (console), so feedback stays in the terminal.

If the registry is missing or the name is not online, the console gets an explicit error string.

## Permission defaults by access level

From `Permissions.h` (summary):

| Level | Typical capabilities |
|-------|----------------------|
| **Player** | No staff permissions. |
| **Moderator** | `ModerateChat`, `CommandGps`. |
| **GameMaster** | Moderator set + `CommandTeleport`, `ManagePlayers`, `CommandGmTools`, `CommandGameplay`. |
| **Administrator** | GM set + `ManageAccounts`, `ServerControl`. |
| **Console** (runtime) | All bits (`UINT64_MAX`) for permission checks. |

Individual commands register a **`PermissionMask`**; `required == 0` means any authenticated context that passed the in-game GM gate (or console) may run it (e.g. `.help`).

## Command catalog (implementation)

All names are registered **without** the leading dot in `CommandService`’s constructor. User input uses `.name`.

| Command | Permission | `consoleOnly` | Console layout | Notes |
|---------|------------|---------------|----------------|-------|
| `help`, `commands` | 0 | no | same | WoW-colored help text; console strips `\|c…\|r` color tokens for readability. |
| `gps` | `CommandGps` | no | target first (console) | Prints `MovementInfo` for session / target. |
| `tele` | `CommandTeleport` | no | target first (console) | Optional `mapId` as 4th numeric arg in-game; default map handling in `TeleportTo`. |
| `gm`, `dnd`, `dev` | `CommandGmTools` | no | same | `on` / `off` toggles; drives `WorldSession` GM appearance state. |
| `visible` | `CommandGmTools` | no | same | `on` = visible; `off` = `UNIT_FLAG_INVISIBLE` for others. |
| `fly` | `CommandGmTools` | no | same | Toggles can-fly packets to the client. |
| `speed` | `CommandGmTools` | no | same | Numeric speed or `reset` (default 7); clamped in `WorldSession::SetGmRunSpeed`. |
| `online`, `who` | `ManagePlayers` | no | same | Lists registry keys (sorted). |
| `announce` | `ManagePlayers` | no | same | System chat line + `SMSG_NOTIFICATION` via registry broadcast. |
| `kick` | `ManagePlayers` | no | same | `.kick Name [reason…]`; uses `RequestDisconnect`. |
| `goto`, `appear` | `ManagePlayers` | no | same | In-game: teleport self to target. Console: `.goto Who Target` / `.appear …`. |
| `summon` | `ManagePlayers` | no | same | In-game: bring target to self. Console: `.summon Who Anchor`. |
| `learn` | `CommandGameplay` | no | target first (console) | Persists spell in `character_spell` when DB supports it. |
| `money` | `CommandGameplay` | no | target first (console) | Signed copper delta; persists `characters.money`. |
| `additem` | `CommandGameplay` | no | target first (console) | First free backpack slot. |
| `level` | `CommandGameplay` | no | target first (console) | Clamps to supported range (e.g. 1–85). |
| `account` | `ManageAccounts` | **yes** | same | `create`, `setaccess`, `delete` against `IAccountRepository`. |
| `ban`, `unban` | `ManageAccounts` | **yes** | same | Toggles `account.locked` (login lock), not only a runtime kick. |

## GM appearance and chat wire format

- **`PlayerGmAppearanceForUpdates`** holds GM tag, DND, dev tag, and visibility. **`MergeGmAppearanceIntoPlayerFields`** merges those bits into update field maps without dropping other flags (used when building self/other player updates in `WorldSessionObjectUpdate.cpp` and related paths).
- **`WorldSessionGmState.cpp`** publishes immediate **nearby broadcasts** of value updates and movement packets when GM toggles change mid-session.
- When **GM tag** is on, outgoing player chat can use **`SMSG_GM_MESSAGECHAT`** with sender name and **`ChatTag`** wire flags for DND/dev/GM (`WorldSessionChatHandlers.cpp`).

## Database support (staff gameplay)

Migration **`sql/16_gameplay_money_spells_account_lock.sql`** (and related schema) adds:

- **`firelands_auth.account.locked`** — used by `.ban` / `.unban` via `MySqlAccountRepository::SetLockedByUsername`.
- **`firelands_characters.characters.money`** — copper balance for `.money`.
- **`firelands_characters.character_spell`** — persisted extra spells for `.learn`.

Apply migrations through the world (or auth) startup migrator as configured; see [tools-sql-build.md](tools-sql-build.md).

## Configuration pointers

- **`worldserver.yaml`** — `Console.Enabled`, `Console.StyledPrompt`, `Console.Tui` affect whether the interactive console runs and how input is rendered (see [executables.md](executables.md)).
- Staff **access level** is stored per account; changing it with `.account setaccess` requires **re-login** for the client session to pick up new privileges.

## Related documentation

- Application layer overview: [application.md](application.md) (mentions `CommandService`).
- World/auth executables: [executables.md](executables.md).
- CLI DB tool (separate from in-world `.account`): [../devtools.md](../devtools.md).

## Extending the system

1. Add a **`Permission`** value and include it in **`DefaultPermissions`** for the right `AccessLevel` values.
2. Register the command in **`CommandService::CommandService`** with `RegisterCommand`, choosing **`consoleOnly`** and **`ConsoleArgLayout`** as needed.
3. If the command needs world-side effects, extend **`ICommandSession`** with a new virtual (with a default no-op) and implement it on **`WorldSession`** (and adjust **`DelegatingCommandSession`** if the console must forward calls).

Keep opcode and update-field constants aligned with **Cataclysm 4.3.4 (build 15595)** expectations when touching wire formats.

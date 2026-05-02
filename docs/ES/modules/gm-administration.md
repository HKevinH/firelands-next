# Administración GM y comandos de staff

Este documento describe cómo Firelands implementa **herramientas de Game Master (GM)**, **comandos de staff en el chat** (mensajes que empiezan por `.`) y la **consola interactiva del world server**. Sirve de mapa de tipos y archivos para ampliar permisos o nuevos comandos con criterio.

## Arquitectura general

| Aspecto | Ubicación | Función |
|--------|-----------|---------|
| Nivel de cuenta persistido | `shared/game/AccessLevel.h` | Valores en `account.access_level` (0 = jugador … 3 = administrador). `AccessLevel::Console` solo existe en tiempo de ejecución para la REPL del servidor. |
| Privilegio efectivo | `AccessLevel.h` + `PrivilegeOrigin` | `EffectiveAccess()` eleva el nivel a **Console** cuando el origen es `PrivilegeOrigin::ServerConsole` (terminal del world), de modo que la consola no queda limitada por el nivel “almacenado” de la sesión stub. |
| Capacidades finas | `shared/game/Permissions.h` | Máscara `Permission`, `DefaultPermissions(AccessLevel)` y `HasPermission()`. Los nuevos privilegios se añaden aquí y se enlazan en `CommandService`. |
| Ejecución de comandos | `application/services/CommandService.{h,cpp}` | Parsea `.comando args…`, comprueba `consoleOnly`, permisos y el **nombre de personaje online** opcional al inicio en consola. |
| Abstracción de sesión | `application/ports/ICommandSession.h` | Puerto mínimo: notificaciones, posición, teletransporte, apariencia GM y helpers de gameplay (`GmLearnSpell`, etc.). |
| Índice de jugadores conectados | `application/services/OnlineCharacterSessionRegistry.{h,cpp}` | Mapa thread-safe **nombre de personaje en minúsculas ASCII → `weak_ptr<ICommandSession>`**. Usado por `.online`, `.kick`, `.goto`, `.summon`, `.announce` y por la consola para `.tele` / `.gps` / gameplay sobre otro personaje. |
| Sesión de cliente | `infrastructure/network/sessions/WorldSession.{h,cpp}` | Implementa `ICommandSession`; se registra y da de baja en `OnlineCharacterSessionRegistry` al entrar al mundo y al desconectar. |
| Apariencia GM y movimiento | `WorldSessionGmState.cpp`, `shared/game/PlayerGmAppearance.{h,cpp}`, `shared/network/MovementSetPackets.h` | Actualiza `PLAYER_FLAGS` / `UNIT_FIELD_FLAGS` (etiquetas e invisibilidad); emite paquetes **move set** 4.3.4 para vuelo y velocidad. |
| Stdin / TUI del world | `world/WorldInteractiveConsole.{h,cpp}` | Encola líneas y llama a `CommandService::ExecuteCommand(..., PrivilegeOrigin::ServerConsole)`. Usa una sesión interna `ServerConsoleCommandSession` (sin personaje en el mundo). |
| Cableado | `world/main.cpp` | Crea `OnlineCharacterSessionRegistry`, `CommandService(onlineCharRegistry, accountRepo)` e inyecta ambos en `WorldSession`. |

**Dirección de dependencias:** la lógica de comandos vive en **application**; **infrastructure** solo implementa `ICommandSession` y deriva el chat hacia `ICommandService`.

## Entrada de comandos

### En juego (chat)

`WorldSessionChatHandlers.cpp` decodifica el mensaje. Si `ICommandService::IsCommand(message)` es cierto (empieza por `.`), se llama a `ExecuteCommand` con `PrivilegeOrigin::GameClient` **antes** de reenviar el chat normal.

Comportamiento relevante en `CommandService::ExecuteCommand`:

- Para **clientes de juego**, solo cuentas con **`AccessLevel::GameMaster` o superior** pueden usar comandos con punto. Moderadores y jugadores **no reciben ninguna respuesta** (ni siquiera “comando desconocido”), por diseño.
- Tras ese filtro, cada comando comprueba **`HasPermission`** frente a `DefaultPermissions(EffectiveAccess(cuenta, origen))`.

### Consola del world server

`WorldInteractiveConsole` solo trata líneas que pasan `IsCommand` (prefijo `.`), salvo `quit` / `exit` (con o sin punto), que piden el cierre del proceso. Los comandos válidos se ejecutan con **`PrivilegeOrigin::ServerConsole`**, así que **`EffectiveAccess` es `Console`** y los bits de permiso son todos los permitidos salvo que el comando sea **`consoleOnly`** (entonces sigue requiriendo origen consola, no “bits extra”).

Algunos comandos exigen un **personaje online como primer argumento** desde consola: véase **Disposición de argumentos en consola**.

## Disposición de argumentos en consola (`ConsoleArgLayout`)

Definido en cada `CommandEntry` de `CommandService`:

- **`SameAsInGame`** — mismos argumentos que en juego (p. ej. `.account create …`).
- **`TargetOnlineCharacterFirst`** — desde consola, el **primer** token es el **nombre** de un personaje conectado; el resto iguala al uso en juego. Ejemplo: `.tele Annabell -8759 544 97` o `.learn Annabell 475`. El servicio envuelve la sesión objetivo en **`DelegatingCommandSession`**: teletransporte y gameplay actúan sobre el **objetivo**, mientras `SendNotification` va al **operador** (consola).

Si falta el registro o el nombre no está online, la consola recibe un mensaje de error explícito.

## Permisos por nivel de acceso

Resumen según `Permissions.h`:

| Nivel | Capacidades típicas |
|-------|----------------------|
| **Player** | Sin permisos de staff. |
| **Moderator** | `ModerateChat`, `CommandGps`. |
| **GameMaster** | Moderador + `CommandTeleport`, `ManagePlayers`, `CommandGmTools`, `CommandGameplay`. |
| **Administrator** | GM + `ManageAccounts`, `ServerControl`. |
| **Console** (solo runtime) | Todos los bits (`UINT64_MAX`) para la comprobación de permisos. |

Cada comando registra una **`PermissionMask`**; `required == 0` permite ejecutarlo a cualquier contexto que haya superado el filtro in-game de GM (o la consola), p. ej. `.help`.

## Catálogo de comandos (implementación)

Los nombres se registran **sin** el punto inicial en el constructor de `CommandService`. El usuario escribe `.nombre`.

| Comando | Permiso | `consoleOnly` | Consola | Notas |
|---------|---------|---------------|---------|-------|
| `help`, `commands` | 0 | no | igual | Texto de ayuda con colores WoW; en consola se eliminan tokens `\|c…\|r`. |
| `gps` | `CommandGps` | no | objetivo primero | Muestra `MovementInfo` de la sesión / objetivo. |
| `tele` | `CommandTeleport` | no | objetivo primero | `mapId` opcional como cuarto número in-game; el mapa por defecto lo resuelve `TeleportTo`. |
| `gm`, `dnd`, `dev` | `CommandGmTools` | no | igual | `on` / `off`; estado de apariencia GM en `WorldSession`. |
| `visible` | `CommandGmTools` | no | igual | `on` = visible; `off` = `UNIT_FLAG_INVISIBLE` para los demás. |
| `fly` | `CommandGmTools` | no | igual | Activa/desactiva paquetes “can fly” hacia el cliente. |
| `speed` | `CommandGmTools` | no | igual | Velocidad numérica o `reset` (7 por defecto); acotado en `SetGmRunSpeed`. |
| `online`, `who` | `ManagePlayers` | no | igual | Lista claves del registro (ordenadas). |
| `announce` | `ManagePlayers` | no | igual | Línea de sistema + `SMSG_NOTIFICATION` vía broadcast del registro. |
| `kick` | `ManagePlayers` | no | igual | `.kick Nombre [motivo…]`; usa `RequestDisconnect`. |
| `goto`, `appear` | `ManagePlayers` | no | igual | En juego: te teletransporta al objetivo. Consola: `.goto Quién Objetivo`. |
| `summon` | `ManagePlayers` | no | igual | En juego: trae al objetivo a ti. Consola: `.summon Quién Ancla`. |
| `learn` | `CommandGameplay` | no | objetivo primero | Persiste hechizo en `character_spell` si la BD lo soporta. |
| `money` | `CommandGameplay` | no | objetivo primero | Delta de cobre con signo; persiste `characters.money`. |
| `additem` | `CommandGameplay` | no | objetivo primero | Primera ranura libre de la mochila. |
| `level` | `CommandGameplay` | no | objetivo primero | Rango admitido (p. ej. 1–85). |
| `account` | `ManageAccounts` | **sí** | igual | `create`, `setaccess`, `delete` vía `IAccountRepository`. |
| `ban`, `unban` | `ManageAccounts` | **sí** | igual | Activa/desactiva `account.locked` (bloqueo de login), no solo expulsar en caliente. |

## Apariencia GM y formato de chat

- **`PlayerGmAppearanceForUpdates`** guarda etiqueta GM, DND, dev y visibilidad. **`MergeGmAppearanceIntoPlayerFields`** fusiona esos bits en mapas de campos de actualización sin pisar el resto de flags (p. ej. en `WorldSessionObjectUpdate.cpp`).
- **`WorldSessionGmState.cpp`** difunde actualizaciones de valores y paquetes de movimiento a los jugadores cercanos cuando cambian los toggles GM en sesión.
- Con la **etiqueta GM** activada, el chat del jugador puede salir como **`SMSG_GM_MESSAGECHAT`** con nombre del remitente y byte **`ChatTag`** para DND/dev/GM (`WorldSessionChatHandlers.cpp`).

## Base de datos (gameplay de staff)

La migración **`sql/16_gameplay_money_spells_account_lock.sql`** (y el esquema alineado) añade:

- **`firelands_auth.account.locked`** — usado por `.ban` / `.unban` (`SetLockedByUsername`).
- **`firelands_characters.characters.money`** — cobre para `.money`.
- **`firelands_characters.character_spell`** — hechizos extra persistidos para `.learn`.

Aplicar migraciones según el arranque del migrator; ver [tools-sql-build.md](tools-sql-build.md).

## Configuración

- **`worldserver.yaml`** — `Console.Enabled`, `Console.StyledPrompt`, `Console.Tui` controlan si hay consola interactiva y el aspecto del prompt (véase [executables.md](executables.md)).
- El **nivel de acceso** de staff está en la cuenta; cambiarlo con `.account setaccess` exige **volver a entrar** para que la sesión del cliente cargue los nuevos permisos.

## Documentación relacionada

- Capa de aplicación: [application.md](application.md) (menciona `CommandService`).
- Ejecutables world/auth: [executables.md](executables.md).
- Herramienta CLI de BD (independiente de `.account` in-game): [../devtools.md](../devtools.md).

## Cómo extender el sistema

1. Añadir un valor en **`Permission`** e incluirlo en **`DefaultPermissions`** para los `AccessLevel` adecuados.
2. Registrar el comando en el constructor de **`CommandService`** con `RegisterCommand`, eligiendo **`consoleOnly`** y **`ConsoleArgLayout`** según corresponda.
3. Si hace falta lógica en el mundo, ampliar **`ICommandSession`** con un virtual nuevo (con no-op por defecto) e implementarlo en **`WorldSession`** (y en **`DelegatingCommandSession`** si la consola debe reenviar llamadas).

Los opcodes y campos de actualización deben seguir alineados con **Cataclysm 4.3.4 (build 15595)** cuando se toque el protocolo.

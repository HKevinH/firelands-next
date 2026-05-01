# Módulo: `FirelandsApplication` (`src/application`)

## Rol

La capa **aplicación** contiene los **casos de uso** y los **puertos** (interfaces que implementa la infraestructura). Orquesta el dominio y los repositorios sin conocer MariaDB ni los detalles de ASIO.

## Biblioteca compilada (`FirelandsApplication`)

`src/application/CMakeLists.txt` compila:

- `RealmListService.cpp` — combina filas persistidas de reinos con estado **en vivo** opcional (`IRealmLiveState`; ver `RealmLiveRegistry`).
- `CommandService.cpp` — despacho de comandos (GM/jugador) usado por sesiones del mundo.
- `WorldService.cpp` — singleton de estado del mundo: mapas, alta/baja de jugadores/criaturas/GO, host de Lua (`IGameScriptHost`) y colisión (`IMapCollisionQueries`).

## Servicios (representativos)

- **`AuthService`** — búsqueda de cuentas, verificación SRP (`SRPService`), manejo de session keys vía `IAccountRepository`.
- **`CharacterService`** — orquestación de personajes vía `ICharacterRepository`.
- **`PlayerCreateInfoService`** — plantillas de creación de personaje vía `IPlayerCreateInfoRepository`.
- **`WebSessionService`** — sesiones para REST vía `IWebSessionRepository` (hoy hay implementación en memoria).
- **`SRPService`** — helpers SRP6a para login.

## Puertos (`application/ports/`)

Interfaces como `INetworkServer`, `IAuthSession`, `ICommandService`, `IGameScriptHost`, `IMapCollisionQueries`, `IRealmLiveState` permiten que infraestructura conecte ASIO, Lua, stubs y realm-link sin invertir dependencias.

## Regla de dependencias

Aplicación depende solo de **dominio** + **shared**. No debe incluir headers concretos de MySQL o ASIO.


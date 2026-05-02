# Module: `FirelandsApplication` (`src/application`)

## Role

The **application** layer implements **use cases** and **ports** (interfaces that infrastructure satisfies). It orchestrates domain objects and repositories without knowing MariaDB or Boost ASIO.

## Built library (`FirelandsApplication`)

`src/application/CMakeLists.txt` compiles:

- `RealmListService.cpp` — combines persisted realm rows with optional **live** population/load data from `IRealmLiveState` (see infrastructure `RealmLiveRegistry`).
- `CommandService.cpp` — staff command dispatch (in-game `.` commands and world console); see [gm-administration.md](gm-administration.md).
- `OnlineCharacterSessionRegistry.cpp` — online character name → session for console targeting and broadcasts (documented in the same guide).
- `WorldService.cpp` — singleton façade for runtime world state: map lookup/create, adding players/creatures/game objects, optional Lua script host (`IGameScriptHost`), optional collision port (`IMapCollisionQueries`).

## Header-only services (representative)

- **`AuthService`** — account lookup, credential verification via `SRPService`, session key storage through `IAccountRepository`, account-bound opaque data (timestamps / blobs).
- **`CharacterService`** — character list and persistence orchestration via `ICharacterRepository`.
- **`PlayerCreateInfoService`** — starting positions/stats for character creation using `IPlayerCreateInfoRepository`.
- **`WebSessionService`** — REST-oriented session tracking via `IWebSessionRepository` (in-memory impl in infrastructure today).
- **`SRPService`** — SRP6a-style verification helpers used by auth flows.

## Ports (`application/ports/`)

Interfaces such as `INetworkServer`, `IAuthSession`, `ICommandService`, `IMapNotifier`, `IGameScriptHost`, `IMapCollisionQueries`, `IRealmLiveState` allow infrastructure to plug in async servers, Lua, stubs, and realm-link state without reversing dependencies.

## Dependency rule

Application depends on **domain** + **shared** only. It must not include concrete MySQL or ASIO headers.


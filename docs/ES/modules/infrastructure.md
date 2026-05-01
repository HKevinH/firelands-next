# Módulo: `FirelandsInfrastructure` (`src/infrastructure`)

## Rol

La **infraestructura** conecta el emulador con **MariaDB**, **TCP (Boost.Asio)**, **REST opcional**, el canal **realm-link** entre auth y world, **Lua** y **colisión** (stub). Implementa los repositorios y las sesiones de red que la aplicación usa como puertos.

## Persistencia

- **`DatabaseMigrator`** — ejecuta migraciones `.sql` en orden y lleva registro en la BD.
- Repositorios **`MySql*`** — implementan `IAccountRepository`, `IRealmRepository`, `ICharacterRepository`, `IPlayerCreateInfoRepository`.
- **`MemoryWebSessionRepository`** — sesiones REST en memoria.

## Red

- **`AsyncNetworkServer`** — acepta conexiones y crea sesiones.
- **`AuthSession`** / **`WorldSession`** — protocolos auth y mundo clásicos.
- **`RestAuthServer`** — API HTTP paralela al login tradicional.
- **Realm-link** — el **world** envía estado al **auth** (`RealmLinkOutbound` → `RealmLinkSession` + `RealmLiveRegistry`) para enriquecer la lista de reinos.

## Scripts y mundo

- **`LuaGameScriptHost`** — scripts bajo la carpeta configurada; eventos hacia Lua.
- **`MapCollisionQueriesStub`** — sustituto temporal de colisión hasta datos vmap completos.

## CMake

Enlaza aplicación, MariaDB, Boost, JSON, Lua y dependencias del conector; Lua puede estar excluido del precompilado de cabeceras.


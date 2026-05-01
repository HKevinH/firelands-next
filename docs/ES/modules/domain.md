# Módulo: `FirelandsDomain` (`src/domain`)

## Rol

La capa **dominio** modela conceptos del juego y de cuentas y define las **interfaces de repositorio** (puertos hacia la persistencia). No contiene SQL ni código de red.

## Fuentes compiladas (`FirelandsDomain`)

Según `src/domain/CMakeLists.txt`:

- `Realm.cpp` — datos de reinos para lista de reinos y estado en vivo.
- `Map.cpp` — rejilla de mapa e índices de objetos.
- `Creature.cpp`, `GameObject.cpp` — entidades del mundo.

## Partes solo en cabeceras

- **Modelos** — personajes, cuentas (tipos usados por repos), `PlayerCreateInfo`, sesiones web, etc.
- **Entidades de mundo** — `Player`, `WorldObject`, `Unit`, APIs de `Map`.
- **Interfaces `I*Repository`** — implementadas en infraestructura por `MySql*`.

## Principios

El dominio describe **qué** existe en el emulador; la infraestructura implementa **cómo** se guarda y cómo viajan los mensajes en red.


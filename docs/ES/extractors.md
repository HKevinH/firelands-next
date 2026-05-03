# Extractores de datos del cliente (4.3.4 / MPQ)

Las herramientas leen un folder retail de **Cataclysm** `World of Warcraft/Data` (archivos **MPQ**, no CASC) y extraen archivos usando **StormLib v9.26** (pinned en el `CMakeLists.txt` raíz).

## Requisitos

- Build con CMake del proyecto (misma toolchain que el resto de Firelands).
- Paquetes de desarrollo de **zlib** y **bzip2** (StormLib los usa cuando `STORM_USE_BUNDLED_LIBRARIES` está apagado).

## Binarios

| Target | Propósito |
|--------|-----------|
| **`firelands-extractors`** | **TUI pantalla completa** (FTXUI): banner Firelands, elegir tarea (DBC/DB2, maps, listar MPQs), rutas y ejecutar; salida en consola integrada. |
| `firelands-dbc-extractor` | Extrae `DBFilesClient\*.dbc` y `DBFilesClient\*.db2`; exige **`--data`** / **`--out`** (o **`--list-mpqs`**); **`--help`** muestra uso. |
| `firelands-map-extractor` | Extracción de mapas; mismo contrato CLI que la herramienta DBC (**sin** menú si lo lanzas solo). |

Los artefactos quedan en `${CMAKE_BINARY_DIR}/bin/`.

## Lanzador TUI (`firelands-extractors`)

Ejecuta **sin argumentos** desde una terminal interactiva (TTY):

```bash
./firelands-extractors
```

Elige la operación, completa **`Data`** de WoW y (salvo listar MPQs) la carpeta de **salida**, luego **Run**. Desplázate en la consola con **Re Pág / Av Pág** o la rueda del ratón; **Q** sale cuando no hay trabajo en curso. Sin TTY (CI / pipes), la herramienta falla con mensaje — usa los binarios dedicados más abajo.

`firelands-extractors --help` resume el uso por scripts.

## Modo solo CLI (scripts / CI)

Listar el orden de MPQs que usará StormLib (base + parches):

```bash
./firelands-dbc-extractor --data "/path/to/WoW/Data" --list-mpqs
```

Extraer todas las tablas `DBFilesClient` del cliente (`.dbc` y `.db2`):

```bash
./firelands-dbc-extractor --data "/path/to/WoW/Data" --out ./client-dbc
```

Extraer archivos relacionados a mapas:

```bash
./firelands-map-extractor --data "/path/to/WoW/Data" --out ./client-maps
```

## Orden de MPQ

Los MPQs bajo `Data` se ordenan para overlay de parches: el primero se abre como base y cada uno siguiente se aplica con `SFileOpenPatchArchive` para que el último “gane” si hay rutas internas repetidas. Para detalles: **`STORM_LIB_ROADMAP.md`**.

## Ver también

- `STORM_LIB_ROADMAP.md` — versiones, riesgos y milestones.
- StormLib oficial: [ladislav-zezula/StormLib](https://github.com/ladislav-zezula/StormLib).


# Módulo: `FirelandsShared` (`src/shared`)

## Rol

`FirelandsShared` es la biblioteca estática **más baja** del proyecto; la usan el dominio, la aplicación y la infraestructura. Agrupa utilidades transversales **sin** acoplarse a servicios de juego ni a bases de datos:

- **Configuración** — `Config` carga YAML (yaml-cpp) con rutas de búsqueda (directorio de trabajo, carpeta del ejecutable, variables de entorno). Expone claves anidadas y lectura tipada para `authserver.yaml` / `worldserver.yaml`.
- **Logging** — `Logger` centraliza spdlog (consola, archivo, rotación) y niveles de log coherentes con el YAML.
- **Red (tipos y paquetes)** — En `shared/network/` viven buffers, opcodes, layouts de paquetes de auth/mundo, cifrado de sesión, campos de actualización y codecs como `SpellCastWire`.
- **Criptografía / SRP** — Constantes y utilidades que apoyan el flujo SRP del login (junto con `SRPService` en aplicación).
- **Lectura DBC** — `DbcReader` para tablas binarias estilo cliente `.dbc`.
- **Común** — Tipos compartidos y banners de consola.

## CMake

`src/shared/CMakeLists.txt` enlaza OpenSSL, spdlog, nlohmann_json y yaml-cpp. Solo algunos `.cpp` se compilan; muchos headers son solo cabeceras incluidas desde otras capas.

## Cuándo añadir código aquí

Solo si es **reutilizable**, **no** depende de MariaDB ni de sesiones ASIO ni de agregados del dominio, y lo necesitan varias capas.


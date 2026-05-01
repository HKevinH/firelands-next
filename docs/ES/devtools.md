# Firelands DevTools

`FirelandsDevTools` es una herramienta de línea de comandos para administrar la base de datos del emulador Firelands. Sirve para tareas comunes como crear cuentas y registrar reinos.

## Requisitos

- **Binario compilado**: compila el proyecto con CMake para generar el ejecutable `FirelandsDevTools`.
- **Base de datos corriendo**: se conecta a MariaDB/MySQL usando credenciales definidas en la configuración del proyecto (normalmente usuario `firelands` con password `firelands` en localhost).

## Uso

Ejecuta el binario desde la carpeta de artefactos (por ejemplo `build/bin`):

```bash
./FirelandsDevTools <command> [arguments]
```

### Comandos

#### 1. Gestión de cuentas
Crea o actualiza una cuenta en la base `auth`. Calcula hashes y requisitos SRP (Secure Remote Password) automáticamente.

```bash
./FirelandsDevTools account <username> <password> [email] [expansion]
```

- **username**: nombre de login.
- **password**: contraseña en texto plano (la herramienta la procesa).
- **email**: (Opcional) correo; por defecto `<username>@firelands.com`.
- **expansion**: (Opcional) expansión (0-3); por defecto `3` (Cataclysm).

**Ejemplo:**
```bash
./FirelandsDevTools account admin admin123 admin@example.com 3
```

#### 2. Gestión de reinos
Registra o actualiza un reino en la tabla `realmlist`.

```bash
./FirelandsDevTools realm <id> <name> <address> <port> [icon] [timezone] [secLevel] [population]
```

- **id**: ID único del reino.
- **name**: nombre visible en la lista de reinos.
- **address**: IP o hostname del world server.
- **port**: puerto del world server (por ejemplo `8085`).
- **icon**: (Opcional) tipo de reino (0=Normal, 1=PvP, 4=RP, 6=RPPvP, 8=No estándar). Default `0`.
- **timezone**: (Opcional) zona horaria (1=Dev, 2=US, 3=Oceanic, etc.). Default `1`.
- **secLevel**: (Opcional) nivel mínimo de seguridad. Default `0`.
- **population**: (Opcional) indicador de población (float). Default `0.0`.

**Ejemplo:**
```bash
./FirelandsDevTools realm 1 "Firelands Test" 127.0.0.1 8085 1 1 0 0.0
```

## Troubleshooting

- **Error de conexión**: verifica que la base esté corriendo (por ejemplo `docker-compose up -d`) y que las credenciales en `DevTools.cpp` coincidan con tu entorno.
- **Permisos**: confirma que el usuario de BD tenga permisos de escritura sobre `firelands_auth`.


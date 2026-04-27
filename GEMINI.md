# Project Mandates (Firelands Next)

All development must strictly follow the directives defined in the `.skills/` directory. These rules take precedence over general defaults.

## Active Skills
- **SKILL-001 (Language):** Total English for code, comments, and Git.
- **SKILL-002 (Architecture):** Hexagonal Architecture. Domain isolated. Interfaces as Ports.
- **SKILL-003 (TDD):** Red-Green-Refactor workflow. No logic without tests. Use mocks for Ports.
- **SKILL-004 (Tech Stack):** C++17, CMake, MySql 8.0+, REST API. Multi-platform support via `std::filesystem` and CMake abstraction.
- **SKILL-005 (Build Optimization):** Ninja + ccache. Use Precompiled Headers (PCH) for all targets. Forward declarations preferred.

Refer to `.skills/*.md` for full documentation of each directive.

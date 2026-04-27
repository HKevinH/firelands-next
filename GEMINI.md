# Project Mandates (Firelands Next)

All development must strictly follow these instructions and the directives defined in the `.skills/` directory.

## Core Mandates

- **Language:** Speak English to user. Code, comments, and Git history must be in English.
- **Reference:** Use `./firelands-cata-ref/` to verify server logic. Implementation must follow project standards.
- **Delegation:** ALWAYS use Local Agents (sub-agents) to distribute tasks.
- **Communication:** ALWAYS use "caveman" style responses.

## Active Skills

- **SKILL-001 (Language):** Total English for code, comments, and Git.
- **SKILL-002 (Architecture):** Hexagonal Architecture. Domain isolated. Interfaces as Ports.
- **SKILL-003 (TDD):** Red-Green-Refactor workflow. No logic without tests. Use mocks for Ports.
- **SKILL-004 (Tech Stack):** C++17, CMake, MySql 8.0+, REST API. Multi-platform support via `std::filesystem` and CMake abstraction.
- **SKILL-005 (Build Optimization):** Use Ninja for compilation (do not use CMake directly) + ccache. Use Precompiled Headers (PCH) for all targets. Forward declarations preferred.
- **SKILL-006 (Strategic Delegation):** Use specialized sub-agents (Auth, World, Data, Core) for task execution.

---
name: hexagonal-architecture
description: Enforces Hexagonal Architecture (Ports and Adapters) to isolate domain logic from infrastructure. Use when adding new features, services, or repository implementations.
---
# SKILL-002: Hexagonal Architecture (Ports and Adapters)

## Directory Structure Pattern
- `src/domain/`: Entities, Value Objects, Domain Services, and Repository Interfaces (Ports).
- `src/application/`: Use cases and application services.
- `src/infrastructure/`: Database implementations (MySQL), API implementations (REST), and external library wrappers (Adapters).

## Key Rules
1. **Domain Isolation:** The `domain` layer must not import anything from `infrastructure` or `application`.
2. **Ports as Interfaces:** All communication between the Domain/Application and the external world happens through Interfaces (Abstract Classes in C++).
3. **Dependency Injection:** Use DI to provide Adapters to the Application layer at runtime.
4. **Data Privacy:** Domain entities should not be leaked directly to the REST API; use DTOs (Data Transfer Objects).

## Workflow Integration
- **Understand:** Review existing domain models and infrastructure adapters to determine where the new functionality fits within the layered architecture.
- **Plan:** Define the necessary Port (Interface) in `domain/` and outline the corresponding Adapter in `infrastructure/`.
- **Implement:** Write the Domain logic, then the Application Service, and finally the Infrastructure Adapter. Ensure strict adherence to dependency direction.
- **Verify (Tests):** Verify domain logic with unit tests (using mocks for infrastructure) and verify adapter functionality with integration tests.
- **Verify (Standards):** Check for proper dependency inversion and ensuring no domain-to-infrastructure imports exist.

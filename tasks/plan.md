# Implementation Plan: Dual-client Intelligent Lab Reservation System

## Overview

Keep the GitHub Pages web client and add a Windows Qt/C++ client. Both clients follow the assignment's nine one-hour slots, explicit seat-state symbols, role separation, exports, and a shared backend contract. The web client remains runnable in local demo mode until Supabase project credentials are supplied; a database transaction becomes the authoritative multi-device conflict check once configured.

## Architecture Decisions

- Keep the existing HTML/CSS visual system and refactor its JavaScript into domain, storage, and UI modules.
- Use exactly nine hourly slots from 08:00 to 17:00 in both clients.
- Use `*` for available, `#` for reserved, and `!` for faulty equipment. Faulty and reserved seats cannot be booked by students.
- Use a Supabase/PostgreSQL schema, Row Level Security, and an atomic reservation RPC as the shared production contract. No service key belongs in either client.
- Allow public student registration only. Administrator accounts are seeded or created through a server-side invitation path.
- Provide a Qt 6 Widgets client with a CMake build, file-backed demo mode, and a network layer reserved for the shared API.

## Task List

### Phase 1: Web correctness and structure

- [ ] Task 1: Refactor web domain and persistence logic into modules with detailed comments.
- [ ] Task 2: Convert reservation, reporting, duration, and export logic to one-hour slots.
- [ ] Task 3: Make seat status explicit with `*`, `#`, and `!`; preserve disabled controls and accessible labels.
- [ ] Task 4: Restrict public registration to students and preserve a documented pre-seeded admin path.

### Checkpoint: Web foundation

- [ ] JavaScript module syntax passes.
- [ ] Nine hourly slots are shown in booking and exports.
- [ ] Students cannot select reserved or faulty equipment.

### Phase 2: Shared data contract

- [ ] Task 5: Add documented SQL schema, policies, unique constraints, and atomic reservation function.
- [ ] Task 6: Add a web backend adapter that uses the contract when configuration is supplied and keeps demo data local otherwise.

### Checkpoint: Data contract

- [ ] Schema enforces one valid reservation per device/date/hour.
- [ ] No privileged backend secret appears in client files.

### Phase 3: Qt/C++ client

- [ ] Task 7: Create a documented Qt 6/CMake application skeleton and data models.
- [ ] Task 8: Implement authentication, student booking, and administrator maintenance windows in file-backed demo mode.
- [ ] Task 9: Implement weekly statistics and `count.txt` / `error.txt` export in the Qt client.

### Checkpoint: Qt client

- [ ] CMake configures and builds when Qt 6 Widgets is installed.
- [ ] Student and administrator flows use the same one-hour and seat-state rules.

### Phase 4: Verification

- [ ] Task 10: Add focused JavaScript logic tests and C++ model tests.
- [ ] Task 11: Run syntax, build, and regression checks; fix verified findings and repeat once.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| No Supabase credentials are available locally | High | Provide the migration and client contract; do not claim cross-device live behavior before a project is connected. |
| Qt 6 SDK is not installed | High | Detect it before the build; preserve a standards-compliant CMake project and install only after checking the available Windows toolchain. |
| Static demo storage is mistaken for a real backend | High | Clearly separate demo mode from shared-backend mode in README and UI copy. |
| Refactor changes existing visual behavior | Medium | Keep the existing markup and CSS contract, then run browser-level regression checks. |

## Acceptance Criteria

- Both clients use nine hourly slots and the same seat-state semantics.
- No public interface can self-assign the administrator role.
- Web and Qt sources are separated, documented, buildable, and committed in the same repository.
- A database-level uniqueness rule, not client timing, resolves real multi-device reservation races after backend configuration.

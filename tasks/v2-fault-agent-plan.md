# Implementation Plan: V2 Fault Response Agent

## Scope

Add an administrator-only fault-response workflow without altering historical
reservations: assess the future booking impact of a reported device fault,
confirm isolation, record a repair outcome, and expose recurrence metrics.
Affected student bookings receive replacement suggestions only; this release
does not silently reschedule them.

## Decisions

- Reuse `issue_reports`, `devices`, `reservations`, and `maintenance_windows`.
- Add an append-only fault-response event log rather than another fault master
  table.
- Use security-definer RPCs for all isolation and repair mutations.
- Treat `devices.status = 'faulty'` as the immediate reservation block. A
  maintenance window remains available for planned time-bounded work.

## Tasks

### Task 1: Additive database contract

Create an independent V2.1 migration with the response-event table, RLS,
indexes, and administrator RPCs for impact analysis, isolation, repair, and
summary metrics.

Acceptance criteria:
- Historical issue reports and reservations remain unchanged.
- An administrator can isolate an unresolved issue; a student cannot.
- Impact results contain only future active reservations and legal same-slot
  replacement candidates.

### Task 2: Administrator fault-response UI

Add an issue-level “analyse impact” action and a confirmation dialog that
shows severity, affected reservations, replacement options, and the explicit
no-automatic-rebooking boundary.

Acceptance criteria:
- Isolation is impossible until the administrator confirms it.
- The existing repair action uses the audited V2.1 RPC.
- Both Chinese and English render consistently.

### Task 3: Documentation and verification

Document the API and add static migration/UI contract checks.

Acceptance criteria:
- JavaScript syntax and whitespace checks pass.
- Tests prove no direct browser mutations of `devices` or `issue_reports`
  remain in the new fault workflow.

## Production checkpoint

The V2.1 SQL migration is not run against Supabase until the user separately
confirms the production database operation.

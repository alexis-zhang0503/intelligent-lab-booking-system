# Agent Layer Design Plan

## Goal

Build three narrow agents for the laboratory booking system: booking planning,
administrator analysis, and fault response. The booking planner must support a
single confirmation for a complete plan with multiple devices, multiple time
slots, and a weekly recurring rule. Each agent may read only its approved
business RPCs. A model can propose and explain; it cannot mutate a business
table. Every state-changing action remains a separate, explicit user
confirmation and is revalidated by the existing PostgreSQL transaction.

## Architecture decisions

- Keep one JWT-protected Supabase Edge Function as the agent gateway. Route by
  an explicit `agentType` from the UI rather than using an LLM intent router.
- Use a shared response envelope: request id, agent type, outcome, factual
  data, natural-language summary, suggested actions, and audit reference.
- Keep deterministic rules in Supabase RPCs. DeepSeek is used only to choose
  from approved read tools and explain their returned facts.
- Use a common `proposal -> user confirmation -> execution RPC` pattern for
  every mutation. The execution endpoint validates the proposal type, caller,
  arguments, expiry, and idempotency key before calling an RPC.
- Materialize a recurring plan as ordinary reservation rows linked to a new
  reservation-series parent. The parent preserves the rule and lets a student
  later cancel future occurrences as a group; historical rows remain intact.
- Use all-or-nothing semantics for the first recurring-plan release: if any
  selected device-hour fails final validation, create none and ask the student
  to re-plan. A best-effort partial-plan mode is deferred because it needs a
  separate, very clear student choice.
- Treat a same-time multi-device reservation as one `booking_block`. All rows
  in one block share a generated identifier, so a student can reserve the
  requested machines for one activity but cannot create another, unrelated
  active block at the same date and hour.
- Keep the existing per-device partial unique index as the authoritative
  cross-user conflict guard. The new plan RPC must also reject duplicate
  occurrences in its own payload and an active same-user, same-date/hour
  block before inserting any rows.
- A preview is never a hold. At confirmation, the database revalidates every
  occurrence and lets the transaction that first obtains the unique device
  slot win; a losing plan is rolled back in full and re-planned against live
  availability.
- Preserve the current fault console as a safe direct-RPC fallback while the
  fault-response agent is introduced. No automatic rescheduling is allowed.

## Agent boundaries

| Agent | Read tools | Can propose | Confirmed execution |
| --- | --- | --- | --- |
| Booking planner | new `v3_plan_schedule` | One-off or recurring multi-device plan | new `v3_create_reservation_plan` only |
| Admin analyst | `v2_admin_metrics`, `v2_fault_response_metrics` | Operational observations and follow-up links | None |
| Fault responder | `v2_fault_impact_analysis`, `v2_fault_response_metrics` | Isolation, repair, and per-booking replacement suggestions | `v2_isolate_fault` or `v2_complete_fault_repair` only |

## Task list

### Phase 1: Common contract

## Task 1: Define the shared agent request and response contract

**Description:** Add a versioned Edge Function contract with explicit agent
types, input validation, response outcomes, and an audit payload that never
stores secrets.

**Acceptance criteria:**
- [ ] Requests with an unknown agent type or action are rejected before a model call.
- [ ] Every success, fallback, and failure returns the same response envelope.
- [ ] Audit records contain request id, agent type, tool names, outcome, and no credentials.

**Verification:**
- [ ] Static/unit tests cover malformed requests and each outcome.
- [ ] Existing booking-agent request remains backward compatible during migration.

**Dependencies:** None.

**Likely files:** `backend/supabase/functions/deepseek-booking-agent/index.ts`,
`docs/v2-api.md`, new Edge Function tests.

### Task 2: Make proposal confirmation generic

**Description:** Represent a model recommendation as a short-lived proposal
with an allowed action and exact arguments. Store the proposal server-side so
a recurring plan cannot be altered by the browser between planning and
confirmation. Confirmation invokes the matching business RPC; it never
accepts free-form model-generated SQL or tool names.

**Acceptance criteria:**
- [ ] A proposal cannot be executed by another user, after expiry, or twice with a new result.
- [ ] Booking confirmation retains idempotency and the database conflict check.
- [ ] The proposal records every occurrence, selected device, recurrence rule,
  and all-or-nothing execution policy.
- [ ] Unsupported write actions fail closed.

**Verification:**
- [ ] Tests cover replay, role mismatch, expiry, and booking conflict.

**Dependencies:** Task 1.

**Likely files:** Edge Function, `v2-0-migration.sql`, tests.

### Checkpoint: foundation

- [ ] Booking suggestions and confirmed bookings work as before.
- [ ] No new direct browser mutation is introduced.

### Phase 2: Three vertical agent slices

## Task 3: Booking planner slice

**Description:** Extend booking mode from a single device-hour query to a
plan request. It collects a start/end date, one or more time slots, device
count, optional preferred devices, and an optional weekly recurrence rule. A
new deterministic planning RPC expands the rule, checks every target
device-hour, detects self-conflicting blocks, and returns whole-plan variants. The Agent explains those
database-returned variants and creates a single confirmation proposal.

**Acceptance criteria:**
- [ ] Only database-returned plan occurrences appear as bookable.
- [ ] A preview shows every date, time, device, recurrence rule, and total
  device-hours before confirmation.
- [ ] The answer distinguishes a plan proposal from a confirmed reservation series.
- [ ] A fixed-device preference is honoured when possible; otherwise the plan
  explicitly marks a permitted device rotation rather than silently changing machines.
- [ ] Rows for the same activity/date/hour share one booking-block id, while a
  second active booking block for the same student/date/hour is rejected.
- [ ] Missing model configuration returns the existing rule-based candidates.

**Verification:**
- [ ] Student staging flow: ask for two devices in two time slots every week,
  inspect the complete plan, confirm once, then verify one confirmed row per
  selected device-hour and one series parent.
- [ ] Introduce a conflicting device-hour between preview and confirmation;
  verify that the confirmation creates no rows.
- [ ] Run two students' overlapping confirmations concurrently; exactly one
  plan wins a contested device-hour and the losing plan creates no rows.
- [ ] Run one student's two unrelated overlapping plans concurrently; at most
  one booking block is created for that student/date/hour.

**Dependencies:** Tasks 1-2.

**Likely files:** Edge Function, `app.js`, a V3 additive migration,
`docs/v2-api.md`, booking-agent tests.

## Task 4: Administrator analysis slice

**Description:** Add read-only operational analysis based on booking,
check-in, cancellation, no-show, capacity, data-completeness, and fault
metrics. The model may summarize returned facts but cannot issue management
actions.

**Acceptance criteria:**
- [ ] Students cannot invoke the agent or its metrics tools.
- [ ] The answer always labels reservation rate and actual check-in usage rate separately.
- [ ] Fallback presents raw live metrics without fabricated interpretation.

**Verification:**
- [ ] Seeded staging data matches a hand-calculated metrics fixture.
- [ ] Student JWT is denied for every administrator tool.

**Dependencies:** Task 1.

**Likely files:** Edge Function, `app.js`, administrator metrics tests.

## Task 5: Fault-response slice

**Description:** Wrap the existing impact analysis and audited isolation/repair
RPCs in the proposal-confirmation pattern. The agent recommends replacements
but never transfers or cancels student reservations.

**Acceptance criteria:**
- [ ] Analysis changes no database rows and offers at most three legal same-slot replacements.
- [ ] Isolation and repair require an administrator's explicit note and confirmation.
- [ ] Existing reservations remain intact after isolation.

**Verification:**
- [ ] Administrator staging flow covers analyse, isolate, repair, and audit event creation.
- [ ] Student calls are denied and direct event reads remain unavailable.

**Dependencies:** Tasks 1-2 and the V2.1 migration.

**Likely files:** Edge Function, `app.js`, `v2-1-fault-response-agent-migration.sql`, fault tests.

### Checkpoint: feature complete

- [ ] Each agent has only the tools in the boundary table.
- [ ] Every write passes through a confirmation and its database RPC.
- [ ] Chinese and English UI state agree with the response envelope.

### Phase 3: release assurance

## Task 6: Staging validation and production runbook update

**Description:** Run the existing V2 test matrix in an isolated Supabase
project, including tool denial, fallback, concurrency, RLS, audit, and fault
flows. Production migration and secret configuration remain a separately
approved operation.

**Acceptance criteria:**
- [ ] All documented staging cases have recorded evidence.
- [ ] Edge Function type check and live DeepSeek tool-call path pass.
- [ ] Production rollout has an explicit rollback and secret-rotation procedure.

**Verification:**
- [ ] Staging evidence is linked from `docs/v2-test-plan.md`.

**Dependencies:** Tasks 1-5.

**Likely files:** `docs/v2-test-plan.md`, `docs/v2-deployment.md`, test evidence.

## Risks and mitigations

| Risk | Mitigation |
| --- | --- |
| Model invents availability or actions | Render bookable options only from returned RPC data; validate proposal actions server-side. |
| Prompt injection attempts tool escalation | Fixed tool allowlist per agent, server-side role checks, no service-role key. |
| Duplicate/conflicting writes | Short-lived proposals, idempotency keys, and final PostgreSQL conflict checks. |
| A recurring plan is partly created after a conflict | Create all occurrences in one transaction; a conflict rolls back the entire plan. |
| One student accidentally occupies the same hour twice | Permit multiple devices only inside one booking block; reject a second active block at that date/hour. |
| Preview becomes stale while another student confirms | Treat preview as advisory only and return a live conflict/re-plan result after the losing transaction rolls back. |
| Unavailable model/API | Deterministic RPC fallback with the same safe action boundary. |
| Unverified production data contract | Stage V2.0/V2.1 migrations and RLS before any production operation. |

## Open decisions

- Whether a confirmed fault isolation should create a student notification record; it is out of scope until a notification channel and consent policy are designed.
- Whether the UI exposes separate pages or a single assistant page with three role-scoped workspaces. The backend contract supports either.
- Whether a later release should offer best-effort partial confirmation. The
  recommended first release is all-or-nothing, so a student never receives an
  incomplete recurring schedule without deliberately choosing that outcome.

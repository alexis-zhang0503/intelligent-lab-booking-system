# Agent Layer V3.0 architecture

## Scope and audit result

V3 extends the existing `deepseek-booking-agent` Edge Function; it does not create a second gateway and does not modify the Qt client. Before V3, the gateway had only `booking` and `admin` modes. The normal DeepSeek path could return prose only while the fallback returned a top-level `candidates` array; the browser then confirmed client-supplied date, hour and devices. `reservations` also had neither persistent plans nor user booking groups.

V3 keeps historical migrations and reservations unchanged. New tables and nullable foreign keys are in `backend/supabase/v3-1-agent-layer-migration.sql` and are applied after V2.0/V2.1/V2.2/V3.0.

## Gateway contract

The caller selects exactly one explicit type:

- `booking_planner` — deterministic candidate plan, then a reservation proposal.
- `admin_analyst` — read-only V3 usage metrics plus V2.1 fault metrics.
- `fault_responder` — V2.1 impact analysis; isolation and repair are proposals.

Every successful response has `requestId`, `sessionId`, `agentType`, `outcome`, `facts`, `message`, `proposals`, and `errors`. The model receives database facts only. Whether DeepSeek succeeds, times out (10 seconds), has no key, or returns unusable content, facts and proposal cards have the same structure. API keys stay in Edge Function environment variables.

The function has an explicit per-agent tool whitelist, JWT/profile checks, administrator checks for the two administrator agents, a 10-second model timeout, no model database write capability, and an audit record for every result. DeepSeek is used only to explain facts; no tool-call loop can let it invent a write operation.

## State and confirmation

`agent_sessions` stores structured constraints per user and type for 24 hours. A request merges new fields, so a later turn can alter only a date, a device preference, or a recurrence field. Raw chat transcripts are not stored.

`agent_proposals` uses `draft → ready → executed`; terminal states also include `expired`, `invalidated`, and `rejected`. A booking proposal expires after 15 minutes. The browser sends only:

```json
{"proposalId":"uuid","idempotencyKey":"uuid"}
```

The confirmation RPC reloads proposal items, locks devices in ascending order, rechecks opening rules, maintenance, current device state, device collision and the user's own calendar, then inserts all reservation rows in one transaction. A conflict rolls back everything. Idempotency results are retained in `agent_execution_keys`.

## Scheduling and conflicts

`v3_plan_schedule` uses `Asia/Shanghai`, one-hour device blocks, an inclusive date interval, optional weekday/start-hour recurrence, 1–9 hour duration, preferred devices, and two deterministic strategies: fixed device (when the same devices are free throughout) and rotation. Plans are limited to 90 days and 360 device-hours. Candidates never make a reservation.

`booking_blocks` is the new user/date/hour grouping unit. A partial unique index permits multiple devices inside one block but prevents a user from creating a second active block for the same hour. `v2_create_reservations` and `reserve_hourly_devices` are redefined to create a block too. The final V3 confirmation additionally checks legacy active reservations with NULL `booking_block_id`, so they cannot be bypassed.

Historical rows are not backfilled: a past same-user multi-device pattern cannot reliably reveal whether it was one logical group. This is reported as legacy grouping data, not fabricated.

## Metrics and fault safety

`admin_analyst` consumes `v3_admin_usage_metrics`; this preserves V3.0's distinction between booked device-minutes, final checked-in/checked-out device-minutes, provisional use, and incomplete historical capacity. It must not interpret a null/limited rate as low utilization.

`fault_responder` preserves affected reservations and sets `auto_reschedule=false`. Isolation and repair first create a proposal and then execute V2.1 RPCs only after confirmation; affected users still need their own replacement confirmation.

# Agent Layer V3.0 API

`POST /functions/v1/deepseek-booking-agent` requires the caller's Supabase bearer token.

## Request

```json
{
  "requestId": "optional UUID",
  "sessionId": "optional UUID",
  "agentType": "booking_planner",
  "language": "zh",
  "message": "下周一到周三上午两台，优先 1、2 号机",
  "input": {
    "startDate": "2026-11-09",
    "endDate": "2026-11-11",
    "weekdays": [1, 2, 3],
    "startHours": [8],
    "durationHours": 1,
    "requiredDeviceCount": 2,
    "preferredDevices": [1, 2]
  }
}
```

`booking_planner` accepts `date` as a single-day shorthand. `weekdays` follows PostgreSQL `extract(dow)`: Sunday 0 through Saturday 6. The planner enforces 90 days and 360 device-hours. `admin_analyst` accepts `startDate`, `endDate`, and optional `deviceNumber`. `fault_responder` requires `issueId`; add `actionType` (`isolate_fault` or `complete_fault_repair`) and `note` to request an action proposal.

Compatibility: old callers using `mode: "booking" | "admin"` are mapped to the first two types. They should migrate because legacy response fields such as top-level `candidates` are no longer authoritative.

## Response

```json
{
  "requestId": "uuid",
  "sessionId": "uuid",
  "agentType": "booking_planner",
  "outcome": "suggested",
  "facts": {"schedule": {"candidates": []}, "timezone": "Asia/Shanghai"},
  "message": "...",
  "proposals": [{"proposalId": "uuid", "state": "ready", "expiresAt": "..."}],
  "errors": []
}
```

`facts` is present for model and fallback paths. `outcome: fallback` means the explanation model was unavailable, not that scheduling facts are stale. `admin_analyst.facts.usageMetrics.capacity_data_complete=false` requires a limitation notice; usage efficiency must not be inferred.

## Confirmation

```json
{
  "sessionId": "uuid",
  "agentType": "booking_planner",
  "confirmation": {
    "proposalId": "uuid",
    "idempotencyKey": "uuid"
  }
}
```

No date, hour, device number, issue id, or repair note is accepted in confirmation. Booking uses `v3_create_reservation_plan`; fault actions use `v3_execute_agent_proposal`. A stale or conflicting proposal returns HTTP 409 with a structured error.

## Database RPCs

- `v3_plan_schedule(p_request jsonb)` — deterministic candidates only.
- `v3_create_booking_proposal(p_session_id uuid, p_candidate jsonb)` — persists a selected candidate as `ready`.
- `v3_create_reservation_plan(p_proposal_id uuid, p_idempotency_key uuid)` — atomic all-or-nothing booking.
- `v3_cancel_reservation_series(p_series_id uuid)` — cancels only future unstarted rows.
- `v3_create_fault_proposal(...)` / `v3_execute_agent_proposal(...)` — two-stage V2.1 fault actions.

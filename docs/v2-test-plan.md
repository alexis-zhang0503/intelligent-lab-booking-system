# V2.0 Staging Test Plan

These tests require an isolated Supabase staging project populated with two students, one administrator, and the V2 migration. They are intentionally not run against the currently connected production project.

| Case | Action | Expected evidence |
| --- | --- | --- |
| Legacy regression | Log in through the existing Web flow; view devices, old reservations, fault reports, profile, and Qt offline demo. | Legacy `reserved` rows display and can be cancelled before starting. Qt regression test remains green. |
| Concurrent booking | Two authenticated sessions submit different idempotency keys for the same device/date/hour at the same time. | Exactly one confirmed row; the other RPC returns the conflict error. |
| Duplicate submit | Resend `v2_create_reservations` with identical request/key after a simulated response loss. | Same reservation UUIDs returned; no duplicate rows/events. |
| Conflict and maintenance | Add an active device or whole-lab maintenance window, then request the slot directly and through `v2_schedule_candidates`. | Booking is rejected; candidate list excludes the slot. |
| State transitions | Attempt `confirmed -> completed`, early check-in, checkout without check-in, and a valid check-in/out. | Invalid transitions fail; valid transitions create one usage record and append events. |
| No-show | Let a confirmed reservation expire, call administrator `v2_refresh_expired_reservations`. | It becomes `no_show`; a checked-in row never becomes no-show. |
| RLS | Query usage/events, agent logs, metrics, opening/maintenance rules as each role. | Student sees only own usage/events/logs and cannot call metrics or maintain rules. |
| Metrics | Seed known capacity, bookings, check-ins, cancellations, no-shows, and maintenance intervals. | `v2_admin_metrics` returns independently hand-calculated reservation and actual check-in rates. |
| DeepSeek fallback | In staging, remove only `DEEPSEEK_API_KEY` from the Edge Function. | Booking mode returns database candidate fallback; no booking occurs until explicit confirmation. |
| DeepSeek tool boundary | Ask the Agent for a schedule, then inspect audit logs and reservation rows before confirmation. | It has only read/audit RPCs; no reservation is created before confirmation. |
| Fault impact analysis | As an administrator, analyse an unresolved issue with future `reserved` and `confirmed` reservations. | The result lists only future active reservations on that device, offers at most three legal same-slot alternatives, and changes no rows. |
| Fault isolation | Confirm isolation for an unresolved issue. | Device becomes `faulty`, issue becomes `faulty`, one `isolated` audit event is written, and affected reservations remain intact. |
| Fault repair | Submit a valid repair note after isolation. | Issue becomes `resolved`, device returns to `available`, and one `repaired` audit event is written. |
| Fault response RLS | Call all V2.1 fault RPCs as a student. | Calls fail with administrator access errors; direct event reads return no rows. |
| V3 50-minute use | Create a 60-minute reservation with check-in at +5 min and check-out at +55 min. | `actual_used_minutes` increases by 50, never 60. |
| V3 short use and no-show | Create one 10-minute complete record and one no-show reservation. | Actual use increases by 10 only; no-show contributes to booked minutes and no-show rate. |
| V3 incomplete and provisional attendance | Create an in-use reservation with check-in but no check-out. | It is counted in `checked_in_missing_checkout_count`; any displayed provisional minutes do not change `actual_usage_rate`. |
| V3 boundary and adjacent slots | Seed attendance before a slot and after its end, plus two adjacent completed reservations. | Each record is clipped to its own slot; no adjacent-slot minute is duplicated. |
| V3 maintenance union | Add overlapping device and whole-lab maintenance windows covering the same slot. | Capacity loses 60 device-minutes once, not once per maintenance row. |
| V3 historical capacity | Query dates before the V3 baseline. | `capacity_data_complete=false`; all capacity-derived rates are null and the limitation is returned. |
| V3 metrics RLS | Call `v3_admin_usage_metrics` with a student JWT. | The call fails with an administrator access error. |

## Local checks completed in this worktree

- `node --check web/app.js` passed.
- Local page load at `http://127.0.0.1:4174/` completed with no captured browser console errors.
- Static migration checks confirmed all required V2 API/table contracts and rejected destructive reservation SQL patterns.
- Qt `LabRepositoryCheck` built and passed: 1/1 tests.

## Not run locally

- SQL execution/RLS/concurrency/maintenance/metric integration tests: no disposable Supabase/PostgreSQL staging environment was supplied.
- Edge Function type check: Deno is not installed on this machine.
- DeepSeek live call: no `DEEPSEEK_API_KEY` was configured, by design.

# V2.0 API Contract

All database RPCs require a Supabase authenticated JWT. Every write is executed server-side in a PostgreSQL transaction and validates the caller identity.

| API | Input | Result / rule |
| --- | --- | --- |
| `v2_create_reservations` | date, hour, device array, UUID idempotency key | Creates one `confirmed` row per device or none. Reusing the same key and payload returns the original rows. |
| `v2_cancel_reservation` | reservation UUID | Owner/admin can cancel only unstarted `reserved`/`confirmed` rows. |
| `v2_reschedule_reservation` | reservation UUID, replacement date/hour/device, UUID key | Cancels the original and creates one confirmed replacement atomically. |
| `v2_check_in` | reservation UUID | Owner/admin only, during the booked hour; records manual check-in and enters `in_use`. |
| `v2_check_out` | reservation UUID | Owner/admin only after check-in; persists checkout and completes the reservation. |
| `v2_refresh_expired_reservations` | none | Administrator only; marks expired unstarted rows as `no_show`. |
| `v2_schedule_candidates` | date, preferred hour, required device count, preferred devices | Read-only Top-3 legal candidates filtered by opening rules, maintenance, status, and active booking conflicts. |
| `v2_admin_metrics` | start date, end date | Administrator-only JSON metrics. |
| `v2_log_agent_interaction` | request UUID, type, request/response JSON, outcome | Writes an auditable Agent event through a controlled RPC. |

## Edge Function endpoint

`POST /functions/v1/deepseek-booking-agent`

The body is `{ mode: "booking" | "admin", message, input, confirmation? }`. A booking-mode request returns suggestions only. A second request with `confirmation.accepted: true` invokes the normal V2 booking RPC. An unavailable DeepSeek service falls back to the database scheduling candidates (or live administrator metrics) and returns `outcome: "fallback"`.

# V2.0 Architecture

```text
Web browser
  ├─ Supabase Auth + Realtime
  ├─ V2 business RPCs ──────────────┐
  └─ deepseek-booking-agent ────────┼─> DeepSeek Chat Completions API
                                     │       (controlled read-only tools)
PostgreSQL / Supabase                │
  ├─ reservations (existing master) ◄┘  final transactional conflict check
  ├─ opening rules + maintenance windows
  ├─ usage records + status events
  └─ metrics / schedule / audit RPCs
```

## Boundaries

- The browser has only the Supabase publishable key. It never receives `DEEPSEEK_API_KEY` or a Supabase service-role key.
- `deepseek-booking-agent` forwards the caller's JWT and only uses RPCs. Model tools can read real-time scheduling candidates or, for administrators, metrics. They cannot mutate tables.
- A model recommendation is not a reservation. Only the explicit confirmation payload invokes `v2_create_reservations`, which rechecks availability in PostgreSQL.
- Existing `reservations` remains the one reservation master table. Legacy `reserved` is supported; all V2 creates are `confirmed`.

## State model

| From | Action | To | Authority |
| --- | --- | --- | --- |
| `confirmed` | manual check-in during the booked hour | `in_use` | owner or administrator RPC |
| `in_use` | manual check-out | `completed` | owner or administrator RPC |
| `confirmed` | cancel before the hour begins | `cancelled` | owner or administrator RPC |
| `confirmed` | administrator refresh after the hour ends with no check-in | `no_show` | administrator RPC |

`reserved` is accepted in V2 transition RPCs as a legacy equivalent of `confirmed`. There is deliberately no approval status and no temporary hold status.

## Metric definitions

- **预约率** = non-cancelled booked device-hours / available device-hours.
- **实际签到使用率** = device-hours with a manual check-in / available device-hours.
- **No-show rate** = `no_show` device-hours / non-cancelled booked device-hours.
- **Cancellation rate** = cancelled device-hours / all created device-hours.

The current V2 metrics use current device state and new maintenance windows to calculate capacity. They do not reconstruct device or maintenance history that was not stored before V2.

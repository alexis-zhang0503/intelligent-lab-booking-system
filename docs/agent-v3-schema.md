# Agent Layer V3.0 schema and migration guide

## New data model

| Object | Purpose | Historical-data treatment |
| --- | --- | --- |
| `reservation_series` | A user's recurring plan boundary and recurrence payload. | No historical row is inserted. |
| `booking_blocks` | A logical same-user/date/hour group; may contain several devices. | Existing reservations keep `NULL`. |
| `reservations.series_id`, `reservations.booking_block_id` | Nullable links; `reservations` remains the only reservation master. | Additive nullable columns only. |
| `agent_sessions` | Per-user/type structured constraints, expires after 24 hours. | New records only. |
| `agent_proposals`, `agent_proposal_items` | Expiring state machine and immutable selected device-hour items. | New records only. |
| `agent_execution_keys` | Per-user proposal idempotency result. | New records only. |

The durable database guards are:

1. Existing partial indexes still prevent another user from reserving the same device/date/hour.
2. `v3_one_active_user_booking_block_per_slot` prevents a user from holding more than one active group at that hour.
3. `agent_proposal_items` has a composite primary key, so a plan cannot duplicate a device-hour.
4. Final RPC revalidation checks old active reservations too, because pre-V3 rows have no block id.

## Migration precheck

Run these queries in a staging SQL editor before applying V3.1; do not run them against production without separate approval.

```sql
select count(*) as reservations_without_group
from public.reservations where booking_block_id is null;

select user_id, reservation_date, start_hour, count(*) as active_rows
from public.reservations
where status in ('reserved','confirmed','in_use','completed')
group by user_id, reservation_date, start_hour
having count(*) > 1;
```

The second result is an integrity report, not deletion/backfill input: multiple old rows can represent a valid old multi-device booking or unrelated legacy behavior. Preserve it with the deployment record.

Apply in order:

1. `schema.sql`, `cloud-migration.sql`, `v2-0-migration.sql`, `v2-1-fault-response-agent-migration.sql`, `v2-2-attendance-fix.sql`.
2. `v3-0-usage-metrics-migration.sql`.
3. `v3-1-agent-layer-migration.sql`.
4. `v3-2-agent-fault-note-compat.sql`.
5. `v3-3-agent-session-clarification-fix.sql`.

## Verification and rollback

```sql
select public.v3_plan_schedule('{"date":"2030-01-02","startHours":[8],"requiredDeviceCount":1}'::jsonb);
select conname from pg_constraint where conrelid = 'public.agent_proposals'::regclass;
select indexname from pg_indexes where tablename = 'booking_blocks';
```

No migration drops legacy data. Application rollback is first: deploy the prior Edge Function/Web bundle; V2 routes still work and already use the V3 booking-block guard. If schema rollback is necessary, first ensure no V3 proposal/series/reservation references exist or retain the schema read-only. Dropping V3 tables/columns is destructive and is intentionally not supplied as an automatic script.

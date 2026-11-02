# V2.0 Schema Audit

Audit date: 2026-11-02. Scope: checked-in repository SQL and Web source only. No production database connection was made.

## Existing contract

| Area | Verified implementation | V2 compatibility decision |
| --- | --- | --- |
| Identity | `auth.users` trigger creates `public.profiles`; persisted roles are `student` and `admin`. `cloud-migration.sql` adds `profiles.active`. | Reuse `profiles`; new RPCs check active accounts and `is_admin()`. |
| Resources | `public.devices(number smallint primary key)` holds the 40 physical devices and `available`/`faulty` state. | Reuse; do not introduce a duplicate equipment master table. |
| Reservations | `public.reservations` has one device/hour row: `device_number`, `user_id`, `reservation_date`, `start_hour`, `status`, `created_at`, `cancelled_at`. | Reuse as the sole business master. New tables reference its UUID. |
| Conflict guard | `active_reservation_per_device_hour` partially uniquely indexes `(device_number, reservation_date, start_hour)` for `reserved` and `completed`. Existing `reserve_hourly_devices` RPC catches `unique_violation`. | Preserve it and add a second V2 partial unique guard for `confirmed` and `in_use`; final commit always uses a database RPC. |
| Legacy status | Check constraint: `reserved`, `completed`, `cancelled`. The cloud client creates `reserved`; browser-only refresh renders elapsed records as completed without persisting it. | Add, not replace: `confirmed`, `in_use`, `no_show`; legacy `reserved` remains legal and is treated as compatible confirmed state in V2 transitions. |
| Faults | `public.issue_reports` records report/status/resolution metadata. | Unchanged. V2 maintenance windows are a separate planned-resource constraint. |
| Auth/RLS | RLS is enabled on profiles, devices, reservations, and issues. Users read availability, own profile/issues; administrators manage protected tables. Student cancellation policy permits only own `reserved -> cancelled`. | Retain policies; V2 writes run only through security-definer RPCs with explicit ownership/role checks. |
| Realtime | Existing publication contains devices, reservations, and issue reports. | Add usage and maintenance change feeds in V2 migration. |

## Verified API/client behaviour

- The Web client is a static ES-module GitHub Pages application using the Supabase publishable key. There is no server process, API route, or secret storage.
- Existing cloud booking uses `reserve_hourly_devices`; cancellation directly updates `reservations`; repair performs two non-transactional updates.
- The Qt application is an offline local demonstration client and does not call Supabase.
- Existing pages expose reservation records and aggregate booked slots, but not manual check-in/out, planned maintenance, usage records, AI calls, or separate booking versus actual-use metrics.

## Required V2 database additions

The additive migration `backend/supabase/v2-0-migration.sql` introduces:

- `lab_opening_rules`, seeded to the existing daily 08:00-17:00 hours;
- `maintenance_windows`, including whole-lab (`device_number IS NULL`) windows;
- `reservation_usage_records` and `reservation_status_events`;
- `reservation_idempotency_keys`, `agent_audit_logs`, and `scheduling_plan_logs`;
- V2 RPCs for create, cancel, reschedule, manual check-in/out, no-show refresh, candidate scheduling, metrics, and agent audit logging.

New reservations are directly `confirmed`; there is no approval queue or temporary reservation lock. The supported state flow is `confirmed -> in_use -> completed`, plus `confirmed -> cancelled` and expired `confirmed -> no_show`. Legacy `reserved` is accepted as the pre-V2 equivalent of `confirmed` for cancellation and check-in.

## RLS and index findings

- Current policies correctly avoid client-provided administrator roles, but direct cloud cancellation is limited to legacy `reserved` records. The Web upgrade must use `v2_cancel_reservation` for new `confirmed` records.
- Existing data-visible reservation reads intentionally expose availability to all signed-in users. The new usage/event tables restrict users to their own reservation rows; administrators retain full visibility.
- There is no index for planned maintenance, status history, usage, agent logs, or idempotency. The V2 migration adds lookup indexes and a new unique active-conflict index without dropping the original one.

## Items requiring production confirmation

- **待确认:** the target Supabase project has executed `schema.sql`, `cloud-migration.sql`, `admin-invite-migration.sql`, `admin-profile-migration.sql`, and `issue-tracking-migration.sql` in the documented order.
- **待确认:** no production `reservations.status` values exist outside the documented set. The migration intentionally stops before changing the constraint if unknown values are found.
- **待确认:** no hand-added columns, renamed constraints, custom RLS policies, or external consumers rely on direct writes to `reservations`.
- **待确认:** the correct operational timezone for database time checks. The repository currently relies on PostgreSQL/Supabase defaults; the V2 RPCs use database `now()`/`current_date` and should be run with the project timezone explicitly reviewed.
- **待确认:** historical device availability and historical maintenance intervals are not stored today. V2 availability metrics use current device state plus new maintenance data, so past reports are not a reconstructed historical-capacity audit.

## Production boundary

This audit and migration are source artifacts only. Applying the migration, deploying an Edge Function, setting `DEEPSEEK_API_KEY`, or changing Supabase/Auth settings are production operations and require separate user confirmation.

# Shared Supabase backend

This directory contains the database contract shared by the GitHub Pages client and the Qt desktop client.

## Setup

1. Create a Supabase project and run `schema.sql` in its SQL editor.
2. Auth signup metadata creates **student** profiles automatically. The project maintainer creates an administrator in the Supabase Dashboard, then promotes only that profile to `admin` in the protected SQL editor. Follow [ADMIN_ACCOUNT_RUNBOOK.md](ADMIN_ACCOUNT_RUNBOOK.md). Do not expose a service-role key in a browser or Qt executable.
3. Place only the project URL and publishable key in each client configuration.
4. Enable Realtime for `devices`, `reservations`, and `issue_reports`.
5. Run `cloud-migration.sql` after `schema.sql`. It enables secure student cancellation and administrator-led student deactivation for the cloud Web client.
6. Run `admin-invite-migration.sql` to enable one-time administrator invitations generated inside the Web administrator interface.

## Security model

- Public registration creates an Auth account whose trigger creates a **student** profile only.
- An administrator is seeded and promoted only by the project maintainer.
- An existing administrator can generate a one-time invitation for a new administrator. The generated code is shown once, expires within 30 days, and is consumed inside the Auth profile trigger.
- `reserve_hourly_devices` performs the final reservation check in one database transaction.
- The unique index on `(device_number, reservation_date, start_hour)` prevents two clients from holding the same one-hour slot.

## Development boundary

The Web client is configured for the project’s public URL and publishable key. In **Authentication → Providers → Email**, turn off **Confirm email** before student testing: students sign in with a student ID, which the client safely maps to an internal course-only Auth email. Administrator accounts continue to use their real email address. The Qt client remains an offline local demonstration client until its network adapter is added.

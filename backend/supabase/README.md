# Shared Supabase backend

This directory contains the database contract shared by the GitHub Pages client and the Qt desktop client.

## Setup

1. Create a Supabase project and run `schema.sql` in its SQL editor.
2. Auth signup metadata creates **student** profiles automatically. The project maintainer creates an administrator in the Supabase Dashboard, then promotes only that profile to `admin` in the protected SQL editor. Follow [ADMIN_ACCOUNT_RUNBOOK.md](ADMIN_ACCOUNT_RUNBOOK.md). Do not expose a service-role key in a browser or Qt executable.
3. Place only the project URL and publishable key in each client configuration.
4. Enable Realtime for `devices`, `reservations`, and `issue_reports`.

## Security model

- Public registration creates an Auth account whose trigger creates a **student** profile only.
- An administrator is seeded and promoted only by the project maintainer.
- `reserve_hourly_devices` performs the final reservation check in one database transaction.
- The unique index on `(device_number, reservation_date, start_hour)` prevents two clients from holding the same one-hour slot.

## Development boundary

The existing web page continues to offer browser-local demo data until project credentials are configured. Local demo mode is not multi-device realtime and must not be presented as the shared backend.

# Implementation Plan: V3 Device-Minute Usage Metrics

## Overview

Replace the Web administrator's duration-inaccurate usage display with a new,
additive V3 metrics API. Preserve V2 compatibility and withhold capacity-derived
rates when device status history cannot support an honest historical result.

## Architecture decisions

- Keep `reservations` as the only reservation master and add no destructive data migration.
- Preserve `v2_admin_metrics`; add `v3_admin_usage_metrics` for device-minute semantics.
- Use complete check-in/check-out pairs for final use, and return incomplete-session duration only as provisional data.
- Record device status from a next-complete-day baseline and future status changes; never use current status to infer earlier capacity.

## Completed task list

- [x] Audit V2 reservation, usage, maintenance, timezone, RLS, client, and Agent paths.
- [x] Add a V3 migration with prechecks, device-status history, timezone-safe slot calculation, metrics RPC, and grants.
- [x] Switch the administrator dashboard and Agent metrics tool to V3 without changing the V2 API.
- [x] Add formula, UI-contract, PostgreSQL integration, staging, deployment, and rollback documentation.
- [x] Validate with Node checks, device-minute contract tests, attendance regression tests, and disposable PGlite integration tests.

## Remaining release gate

- [ ] Apply V3 only to a disposable Supabase staging project and record the staging evidence before any separately approved production operation.

# Agent Layer V3.0 test report

## Executed locally

The following were run in a disposable PGlite PostgreSQL-compatible database on 2026-11-04. No Supabase project, staging project, or production project was modified.

```text
node tests/usage-metrics-db.mjs
PASS: V3 device-minute metrics, clipping, incomplete attendance,
maintenance union, no-show and administrator boundary

node tests/agent-v3-db.mjs
PASS: V3 proposal confirmation is idempotent; user-slot and cross-user
device guards apply to V3 and V2 paths; fault isolation preserves reservations;
future series cancellation is grouped

node --test tests/usage-metrics-contract.test.cjs tests/attendance.test.mjs
PASS: 17/17

node --check app.js
PASS
git diff --check
PASS
```

## Coverage matrix

| Scenario | Evidence | Result |
| --- | --- | --- |
| Normal and fallback share `facts`/`proposals` envelope | Edge Function contract and static inspection | Implemented; cloud invocation not run. |
| No client date/device confirmation | Edge Function accepts only proposal/idempotency key | Implemented. |
| One-hour, 50-minute final use | `usage-metrics-db.mjs` | Passed. |
| One-hour, 10-minute final use | `usage-metrics-db.mjs` | Passed. |
| No-show has zero actual use | `usage-metrics-db.mjs` | Passed. |
| Check-in without check-out is incomplete/provisional only | `usage-metrics-db.mjs` | Passed. |
| Boundary clipping | `usage-metrics-db.mjs` | Passed. |
| Overlapping maintenance union | `usage-metrics-db.mjs` | Passed. |
| Incomplete historical device status withholds rates | `usage-metrics-db.mjs` | Passed in V3.0 suite. |
| Non-admin cannot read admin metrics | `usage-metrics-db.mjs` | Passed. |
| Candidate plan is persisted before confirmation | `agent-v3-db.mjs` | Passed. |
| Confirmation replay is idempotent | `agent-v3-db.mjs` | Passed. |
| Same user, same hour, different V3/V2 group | `agent-v3-db.mjs` | Passed. |
| Same group can contain two devices | `agent-v3-db.mjs` | Passed. |
| Different users cannot take same device-hour | `agent-v3-db.mjs` | Passed. |
| Legacy V2 creation uses V3 block guard | `agent-v3-db.mjs` | Passed. |
| Series cancellation cancels future unstarted items as one group | `agent-v3-db.mjs` | Passed. |
| Fault isolation is a proposal and preserves reservations | `agent-v3-db.mjs` | Passed locally; real Supabase execution not run. |
| Admin Agent uses corrected metrics | Edge/app code inspection | Implemented; authenticated E2E not run. |

## Known verification limits

PGlite does not prove Supabase RLS, Edge Function deployment, DeepSeek network behavior, or concurrent transaction behavior on a managed PostgreSQL instance. Existing legacy browser CJS suites remain incompatible with ESM `app.js` before these changes. These are not reported as passing.

## Staging deployment checklist

1. Create/choose a non-production Supabase staging project and back up its schema/data.
2. Run the precheck in `agent-v3-schema.md`; retain the legacy grouping report.
3. Apply V3.0 then V3.1 SQL through the staging SQL editor.
4. Deploy the revised `deepseek-booking-agent` function with only server-side `DEEPSEEK_API_KEY`; set no key in browser code.
5. Use a student and an admin to exercise candidate/proposal/replay/conflict/fault paths, then compare Admin Agent metrics to `v3_admin_usage_metrics` directly.
6. Roll back the Web/Edge bundle first if needed. Do not apply destructive schema rollback automatically.

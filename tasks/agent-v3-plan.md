# Implementation Plan: Agent Layer V3.0

## Architecture decisions

- Extend the existing JWT-protected Edge Function into an explicit three-agent gateway; the page supplies the agent type.
- Persist a normalized proposal before confirmation. Confirmation accepts only proposal id and idempotency key.
- Keep `reservations` as the sole reservation master. A nullable series id and booking-block id add grouping without rewriting historical rows.
- Let PostgreSQL determine all candidates and final conflicts. DeepSeek explains returned facts and never invents availability.
- A plan is bounded to 90 days and 360 device-hours. It is all-or-nothing at execution.

## Ordered slices

1. Add the non-destructive V3.1 schema, session/proposal state machine, scheduling engine, booking blocks, and execution RPCs. Verify with disposable PostgreSQL tests.
2. Update legacy V2 booking entry points to create booking blocks and therefore obey the same-user time guard. Verify a second activity at the same hour is rejected while a multi-device block is allowed.
3. Replace the Edge Function request/response contract and provide deterministic booking fallback facts/proposals. Verify contract tests.
4. Update the Web Agent UI to render `facts` and proposal cards, and submit only `proposalId + idempotencyKey`.
5. Document architecture, schema, API, test evidence, staging deployment, and rollback. Do not run any production operation.

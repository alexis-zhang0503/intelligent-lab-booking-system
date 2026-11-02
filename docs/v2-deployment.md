# V2.0 Deployment Runbook

## Before production changes

1. Back up the Supabase database and inspect the production confirmation items in [v2-schema-audit.md](v2-schema-audit.md).
2. Verify the project has applied the existing migrations in the documented order.
3. Run `backend/supabase/v2-0-migration.sql` once in a staging Supabase project first. It is additive, but it deliberately stops if it finds an unknown reservation status.
4. Test staged RLS with a student and an administrator before production rollout.

## Edge Function

Deploy `backend/supabase/functions/deepseek-booking-agent/index.ts` as a JWT-protected Supabase Edge Function. Configure these secrets in the function environment, never in the web source:

```text
DEEPSEEK_API_KEY=...
SUPABASE_URL=...              # normally supplied by Supabase
SUPABASE_ANON_KEY=...         # normally supplied by Supabase
```

Keep JWT verification enabled. The function uses the calling user's JWT, not the service-role key. The DeepSeek request follows the documented Chat Completions function-tool flow; tool arguments are validated before an RPC is called. See [DeepSeek Chat Completions](https://api-docs.deepseek.com/api/create-chat-completion/) and [Tool Calls](https://api-docs.deepseek.com/guides/tool_calls/).

## Rollout checks

1. Confirm a new Web reservation has `status = confirmed` and a status-event row.
2. Confirm two users cannot reserve one device-hour simultaneously.
3. Confirm a student cannot read another student's usage/event records or call administrator metrics.
4. Confirm manual check-in/out writes one usage row and state events.
5. Confirm an expired unstarted reservation becomes `no_show` only through the administrator refresh action.
6. Temporarily omit the DeepSeek secret in staging and confirm the scheduling fallback is shown without a failed reservation write.

No production SQL, Edge Function deployment, secret configuration, or authentication configuration has been performed by this code change.

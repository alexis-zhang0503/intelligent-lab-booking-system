-- V3.4: assistant conversation metadata and audit payloads are private to their owner.
-- Apply after V3.3. Administrator reporting continues through aggregate RPCs.

drop policy if exists "users read their agent sessions" on public.agent_sessions;
create policy "users read their agent sessions" on public.agent_sessions for select to authenticated
  using (user_id = auth.uid());

drop policy if exists "users read their agent proposals" on public.agent_proposals;
create policy "users read their agent proposals" on public.agent_proposals for select to authenticated
  using (user_id = auth.uid());

drop policy if exists "users read their agent proposal items" on public.agent_proposal_items;
create policy "users read their agent proposal items" on public.agent_proposal_items for select to authenticated
  using (exists (select 1 from public.agent_proposals p where p.id = proposal_id and p.user_id = auth.uid()));

drop policy if exists "users read their agent audit logs" on public.agent_audit_logs;
create policy "users read their agent audit logs" on public.agent_audit_logs for select to authenticated
  using (user_id = auth.uid());

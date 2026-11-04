-- V3.3 fixes the session-column ambiguity and preserves every existing session.
-- Apply after V3.2. This migration changes only function code.

create or replace function public.v3_save_agent_session(
  p_session_id uuid,
  p_agent_type text,
  p_constraints jsonb
)
returns table(session_id uuid, constraints jsonb, expires_at timestamptz)
language plpgsql security definer set search_path = public
as $$
declare v_id uuid;
begin
  if p_agent_type not in ('booking_planner', 'admin_analyst', 'fault_responder') then raise exception 'Invalid agent type'; end if;
  if p_agent_type in ('admin_analyst', 'fault_responder') and not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if p_session_id is not null then
    select id into v_id from public.agent_sessions where id = p_session_id and user_id = auth.uid() and agent_type = p_agent_type for update;
  end if;
  if v_id is null then
    insert into public.agent_sessions(user_id, agent_type, constraints)
    values(auth.uid(), p_agent_type, coalesce(p_constraints, '{}'::jsonb))
    returning id into v_id;
  else
    update public.agent_sessions as session_row
    set constraints = session_row.constraints || coalesce(p_constraints, '{}'::jsonb),
        updated_at = now(),
        expires_at = now() + interval '24 hours'
    where session_row.id = v_id;
  end if;
  return query
  select session_row.id, session_row.constraints, session_row.expires_at
  from public.agent_sessions session_row
  where session_row.id = v_id;
end;
$$;

revoke all on function public.v3_save_agent_session(uuid,text,jsonb) from public;
grant execute on function public.v3_save_agent_session(uuid,text,jsonb) to authenticated;

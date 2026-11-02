-- V2.1 additive fault-response workflow.
-- Prerequisite: schema.sql, cloud-migration.sql, issue-tracking-migration.sql,
-- and v2-0-migration.sql. This migration never alters historical reservations.

create table if not exists public.fault_response_events (
  id uuid primary key default gen_random_uuid(),
  issue_id uuid not null references public.issue_reports(id) on delete cascade,
  action text not null check (action in ('isolated', 'repaired')),
  actor_id uuid not null references public.profiles(id),
  note text not null check (char_length(trim(note)) between 1 and 200),
  payload jsonb not null default '{}'::jsonb,
  created_at timestamptz not null default now()
);

create index if not exists fault_response_events_issue_created
  on public.fault_response_events (issue_id, created_at desc);
create index if not exists fault_response_events_action_created
  on public.fault_response_events (action, created_at desc);

alter table public.fault_response_events enable row level security;

drop policy if exists "administrators read fault response events" on public.fault_response_events;
create policy "administrators read fault response events" on public.fault_response_events
  for select to authenticated using (public.is_admin());

create or replace function public.v2_fault_impact_analysis(p_issue_id uuid)
returns jsonb language plpgsql stable security definer set search_path = public
as $$
declare
  v_issue public.issue_reports%rowtype;
  v_affected_count integer;
  v_affected jsonb;
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;

  select * into v_issue from public.issue_reports where id = p_issue_id;
  if not found then raise exception 'The issue was not found'; end if;

  select count(*) into v_affected_count
    from public.reservations r
   where r.device_number = v_issue.device_number
     and r.status in ('reserved', 'confirmed')
     and (r.reservation_date > current_date
       or (r.reservation_date = current_date and r.start_hour > extract(hour from current_time)));

  select coalesce(jsonb_agg(jsonb_build_object(
    'reservation_id', r.id,
    'reservation_date', r.reservation_date,
    'start_hour', r.start_hour,
    'user_id', r.user_id,
    'user_name', p.display_name,
    'replacement_device_numbers', coalesce(replacements.device_numbers, '[]'::jsonb),
    'requires_user_confirmation', true
  ) order by r.reservation_date, r.start_hour), '[]'::jsonb)
  into v_affected
  from public.reservations r
  join public.profiles p on p.id = r.user_id
  left join lateral (
    select jsonb_agg(candidate.number order by candidate.number) as device_numbers
    from (
      select d.number
        from public.devices d
       where d.status = 'available'
         and d.number <> v_issue.device_number
         and not exists (
           select 1 from public.reservations occupied
            where occupied.device_number = d.number
              and occupied.reservation_date = r.reservation_date
              and occupied.start_hour = r.start_hour
              and occupied.status in ('reserved', 'confirmed', 'in_use', 'completed')
         )
         and not exists (
           select 1 from public.maintenance_windows m
            where m.reservation_date = r.reservation_date
              and m.status in ('scheduled', 'active')
              and m.start_hour < r.start_hour + 1
              and m.end_hour > r.start_hour
              and (m.device_number is null or m.device_number = d.number)
         )
       order by d.number
       limit 3
    ) candidate
  ) replacements on true
  where r.device_number = v_issue.device_number
    and r.status in ('reserved', 'confirmed')
    and (r.reservation_date > current_date
      or (r.reservation_date = current_date and r.start_hour > extract(hour from current_time)));

  return jsonb_build_object(
    'issue_id', v_issue.id,
    'device_number', v_issue.device_number,
    'issue_status', v_issue.status,
    'description', v_issue.description,
    'affected_count', v_affected_count,
    'severity', case when v_affected_count >= 5 then 'high' when v_affected_count > 0 then 'medium' else 'low' end,
    'affected_reservations', v_affected,
    'auto_reschedule', false
  );
end;
$$;

create or replace function public.v2_isolate_fault(
  p_issue_id uuid,
  p_note text
)
returns jsonb language plpgsql security definer set search_path = public
as $$
declare
  v_issue public.issue_reports%rowtype;
  v_affected_count integer;
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if char_length(trim(coalesce(p_note, ''))) < 1 or char_length(trim(p_note)) > 200 then
    raise exception 'The isolation note must contain 1 to 200 characters';
  end if;

  select * into v_issue from public.issue_reports where id = p_issue_id for update;
  if not found then raise exception 'The issue was not found'; end if;
  if v_issue.status = 'resolved' then raise exception 'A resolved issue cannot be isolated'; end if;

  select count(*) into v_affected_count
    from public.reservations r
   where r.device_number = v_issue.device_number
     and r.status in ('reserved', 'confirmed')
     and (r.reservation_date > current_date
       or (r.reservation_date = current_date and r.start_hour > extract(hour from current_time)));

  update public.devices
     set status = 'faulty', updated_by = auth.uid(), updated_at = now()
   where number = v_issue.device_number;
  update public.issue_reports set status = 'faulty' where id = v_issue.id;

  if not exists (
    select 1 from public.fault_response_events
     where issue_id = v_issue.id and action = 'isolated'
  ) then
    insert into public.fault_response_events (issue_id, action, actor_id, note, payload)
    values (v_issue.id, 'isolated', auth.uid(), trim(p_note), jsonb_build_object('affected_count', v_affected_count));
  end if;

  return jsonb_build_object('issue_id', v_issue.id, 'device_number', v_issue.device_number, 'affected_count', v_affected_count, 'status', 'faulty');
end;
$$;

create or replace function public.v2_complete_fault_repair(
  p_issue_id uuid,
  p_note text
)
returns void language plpgsql security definer set search_path = public
as $$
declare v_issue public.issue_reports%rowtype;
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if char_length(trim(coalesce(p_note, ''))) < 4 or char_length(trim(p_note)) > 120 then
    raise exception 'The repair note must contain 4 to 120 characters';
  end if;

  select * into v_issue from public.issue_reports where id = p_issue_id for update;
  if not found then raise exception 'The issue was not found'; end if;
  if v_issue.status = 'resolved' then return; end if;

  update public.issue_reports
     set status = 'resolved', resolved_at = now(), resolved_by = auth.uid(), resolution_note = trim(p_note)
   where id = v_issue.id;
  update public.devices
     set status = 'available', updated_by = auth.uid(), updated_at = now()
   where number = v_issue.device_number;
  insert into public.fault_response_events (issue_id, action, actor_id, note)
  values (v_issue.id, 'repaired', auth.uid(), trim(p_note));
end;
$$;

create or replace function public.v2_fault_response_metrics(
  p_start_date date,
  p_end_date date
)
returns jsonb language sql stable security definer set search_path = public
as $$
  select case when not public.is_admin() then null else jsonb_build_object(
    'reported_count', (select count(*) from public.issue_reports where created_at::date between p_start_date and p_end_date),
    'open_count', (select count(*) from public.issue_reports where status in ('reported', 'faulty')),
    'isolated_count', (select count(*) from public.fault_response_events where action = 'isolated' and created_at::date between p_start_date and p_end_date),
    'repaired_count', (select count(*) from public.fault_response_events where action = 'repaired' and created_at::date between p_start_date and p_end_date),
    'average_repair_minutes', (
      select round(avg(extract(epoch from (resolved_at - created_at)) / 60.0), 1)
        from public.issue_reports
       where resolved_at is not null and created_at::date between p_start_date and p_end_date
    ),
    'repeat_devices', coalesce((
      select jsonb_agg(jsonb_build_object('device_number', device_number, 'incident_count', incident_count) order by incident_count desc, device_number)
      from (
        select device_number, count(*) as incident_count
          from public.issue_reports
         where created_at::date between p_start_date and p_end_date
         group by device_number
        having count(*) > 1
        order by incident_count desc, device_number
        limit 5
      ) repeated
    ), '[]'::jsonb)
  ) end;
$$;

revoke all on function public.v2_fault_impact_analysis(uuid) from public;
revoke all on function public.v2_isolate_fault(uuid, text) from public;
revoke all on function public.v2_complete_fault_repair(uuid, text) from public;
revoke all on function public.v2_fault_response_metrics(date, date) from public;
grant select on public.fault_response_events to authenticated;
grant execute on function public.v2_fault_impact_analysis(uuid) to authenticated;
grant execute on function public.v2_isolate_fault(uuid, text) to authenticated;
grant execute on function public.v2_complete_fault_repair(uuid, text) to authenticated;
grant execute on function public.v2_fault_response_metrics(date, date) to authenticated;

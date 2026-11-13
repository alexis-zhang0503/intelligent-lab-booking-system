-- V3.1 Agent Layer. Apply after V2.0, V2.1, V2.2 and V3.0.
-- Additive only: historical reservations remain the source of truth and are not backfilled.

create extension if not exists pgcrypto;

create table if not exists public.reservation_series (
  id uuid primary key default gen_random_uuid(),
  user_id uuid not null references public.profiles(id) on delete cascade,
  recurrence jsonb not null default '{}'::jsonb,
  start_date date not null,
  end_date date not null,
  status text not null default 'active' check (status in ('active', 'cancelled', 'completed')),
  created_at timestamptz not null default now(),
  cancelled_at timestamptz,
  check (end_date >= start_date)
);

create table if not exists public.booking_blocks (
  id uuid primary key default gen_random_uuid(),
  user_id uuid not null references public.profiles(id) on delete cascade,
  reservation_date date not null,
  start_hour smallint not null check (start_hour between 0 and 23),
  active boolean not null default true,
  created_at timestamptz not null default now(),
  closed_at timestamptz
);
create unique index if not exists v3_one_active_user_booking_block_per_slot
  on public.booking_blocks (user_id, reservation_date, start_hour) where active;

alter table public.reservations add column if not exists series_id uuid references public.reservation_series(id);
alter table public.reservations add column if not exists booking_block_id uuid references public.booking_blocks(id);
create index if not exists reservations_series_id_idx on public.reservations(series_id);
create index if not exists reservations_booking_block_id_idx on public.reservations(booking_block_id);

create table if not exists public.agent_sessions (
  id uuid primary key default gen_random_uuid(),
  user_id uuid not null references public.profiles(id) on delete cascade,
  agent_type text not null check (agent_type in ('booking_planner', 'admin_analyst', 'fault_responder')),
  constraints jsonb not null default '{}'::jsonb,
  updated_at timestamptz not null default now(),
  expires_at timestamptz not null default (now() + interval '24 hours')
);
create index if not exists agent_sessions_user_updated_idx on public.agent_sessions(user_id, updated_at desc);

create table if not exists public.agent_proposals (
  id uuid primary key default gen_random_uuid(),
  user_id uuid not null references public.profiles(id) on delete cascade,
  session_id uuid references public.agent_sessions(id) on delete set null,
  agent_type text not null check (agent_type in ('booking_planner', 'admin_analyst', 'fault_responder')),
  action_type text not null check (action_type in ('create_reservation_plan', 'isolate_fault', 'complete_fault_repair')),
  state text not null default 'draft' check (state in ('draft', 'ready', 'executed', 'expired', 'invalidated', 'rejected')),
  payload jsonb not null default '{}'::jsonb,
  result jsonb,
  expires_at timestamptz not null default (now() + interval '15 minutes'),
  executed_at timestamptz,
  created_at timestamptz not null default now()
);
create index if not exists agent_proposals_user_state_idx on public.agent_proposals(user_id, state, created_at desc);

create table if not exists public.agent_proposal_items (
  proposal_id uuid not null references public.agent_proposals(id) on delete cascade,
  reservation_date date not null,
  start_hour smallint not null check (start_hour between 0 and 23),
  device_number smallint not null references public.devices(number),
  primary key (proposal_id, reservation_date, start_hour, device_number)
);

create table if not exists public.agent_execution_keys (
  user_id uuid not null references public.profiles(id) on delete cascade,
  idempotency_key uuid not null,
  proposal_id uuid not null references public.agent_proposals(id) on delete cascade,
  result jsonb,
  created_at timestamptz not null default now(),
  primary key (user_id, idempotency_key)
);

alter table public.agent_sessions enable row level security;
alter table public.agent_proposals enable row level security;
alter table public.agent_proposal_items enable row level security;
alter table public.reservation_series enable row level security;
alter table public.booking_blocks enable row level security;

drop policy if exists "users read their agent sessions" on public.agent_sessions;
create policy "users read their agent sessions" on public.agent_sessions for select to authenticated
  using (user_id = auth.uid() or public.is_admin());
drop policy if exists "users read their agent proposals" on public.agent_proposals;
create policy "users read their agent proposals" on public.agent_proposals for select to authenticated
  using (user_id = auth.uid() or public.is_admin());
drop policy if exists "users read their agent proposal items" on public.agent_proposal_items;
create policy "users read their agent proposal items" on public.agent_proposal_items for select to authenticated
  using (exists (select 1 from public.agent_proposals p where p.id = proposal_id and (p.user_id = auth.uid() or public.is_admin())));
drop policy if exists "users read their reservation series" on public.reservation_series;
create policy "users read their reservation series" on public.reservation_series for select to authenticated
  using (user_id = auth.uid() or public.is_admin());
drop policy if exists "users read their booking blocks" on public.booking_blocks;
create policy "users read their booking blocks" on public.booking_blocks for select to authenticated
  using (user_id = auth.uid() or public.is_admin());

-- Future booking operations create a block. Existing records deliberately retain NULL:
-- their historical grouping cannot be reconstructed reliably.
create or replace function public.v3_open_booking_block(p_user_id uuid, p_date date, p_start_hour smallint)
returns uuid language plpgsql security definer set search_path = public
as $$
declare v_block_id uuid;
begin
  insert into public.booking_blocks(user_id, reservation_date, start_hour)
  values (p_user_id, p_date, p_start_hour)
  returning id into v_block_id;
  return v_block_id;
exception when unique_violation then
  raise exception 'The user already has an active booking block in this time slot';
end;
$$;

create or replace function public.v3_close_booking_block_if_empty(p_block_id uuid)
returns void language plpgsql security definer set search_path = public
as $$
begin
  if p_block_id is null then return; end if;
  update public.booking_blocks b set active = false, closed_at = now()
   where b.id = p_block_id and b.active
     and not exists (
       select 1 from public.reservations r
        where r.booking_block_id = b.id
          and r.status in ('reserved', 'confirmed', 'in_use', 'completed')
     );
end;
$$;

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
    insert into public.agent_sessions(user_id, agent_type, constraints) values(auth.uid(), p_agent_type, coalesce(p_constraints, '{}'::jsonb)) returning id into v_id;
  else
    update public.agent_sessions set constraints = constraints || coalesce(p_constraints, '{}'::jsonb), updated_at = now(), expires_at = now() + interval '24 hours' where id = v_id;
  end if;
  return query select s.id, s.constraints, s.expires_at from public.agent_sessions s where s.id = v_id;
end;
$$;

-- Build deterministic candidates; all availability and same-user conflicts are read from PostgreSQL.
create or replace function public.v3_plan_schedule(p_request jsonb)
returns jsonb language plpgsql stable security definer set search_path = public
as $$
declare
  v_start date := coalesce(nullif(p_request->>'startDate','')::date, nullif(p_request->>'date','')::date);
  v_end date := coalesce(nullif(p_request->>'endDate','')::date, coalesce(nullif(p_request->>'date','')::date));
  v_today date := (now() at time zone 'Asia/Shanghai')::date;
  v_count integer := coalesce(nullif(p_request->>'requiredDeviceCount','')::integer, 1);
  v_duration integer := coalesce(nullif(p_request->>'durationHours','')::integer, 1);
  v_preferred smallint[] := coalesce(array(select jsonb_array_elements_text(coalesce(p_request->'preferredDevices','[]'::jsonb))::smallint), '{}'::smallint[]);
  v_hours smallint[] := coalesce(array(select distinct jsonb_array_elements_text(p_request->'startHours')::smallint order by 1), '{}'::smallint[]);
  v_weekdays smallint[] := coalesce(array(select distinct jsonb_array_elements_text(p_request->'weekdays')::smallint), '{}'::smallint[]);
  v_slot record;
  v_device smallint;
  v_available smallint[];
  v_common smallint[] := null;
  v_slots jsonb := '[]'::jsonb;
  v_rotation jsonb := '[]'::jsonb;
  v_fixed jsonb := '[]'::jsonb;
  v_failures jsonb := '[]'::jsonb;
  v_candidates jsonb := '[]'::jsonb;
  v_total integer := 0;
begin
  if v_start is null or v_end is null or v_end < v_start then raise exception 'A valid startDate and endDate are required'; end if;
  if v_start < v_today or v_end > v_today + 90 then raise exception 'Plans must start today or later and end within 90 days'; end if;
  if v_count not between 1 and 40 or v_duration not between 1 and 9 then raise exception 'Invalid device count or duration'; end if;
  if cardinality(v_hours) = 0 then v_hours := array[coalesce(nullif(p_request->>'preferredHour','')::smallint, 8)]; end if;
  if exists(select 1 from unnest(v_hours) h where h not between 0 and 23)
     or exists(select 1 from unnest(v_weekdays) d where d not between 0 and 6) then raise exception 'Invalid schedule hour or weekday'; end if;

  for v_slot in
    select day::date as reservation_date, (start_hour + offset_hour)::smallint as start_hour
      from generate_series(v_start, v_end, interval '1 day') day
      cross join unnest(v_hours) start_hour
      cross join generate_series(0, v_duration - 1) offset_hour
     where cardinality(v_weekdays) = 0 or extract(dow from day)::smallint = any(v_weekdays)
     order by day, start_hour, offset_hour
  loop
    if v_slot.reservation_date = v_today and v_slot.start_hour <= extract(hour from now() at time zone 'Asia/Shanghai') then
      v_failures := v_failures || jsonb_build_array(jsonb_build_object('date',v_slot.reservation_date,'hour',v_slot.start_hour,'reason','slot_has_started'));
      continue;
    end if;
    if not exists(select 1 from public.lab_opening_rules r where r.active and r.weekday = extract(dow from v_slot.reservation_date)::smallint and v_slot.start_hour >= r.start_hour and v_slot.start_hour + 1 <= r.end_hour) then
      v_failures := v_failures || jsonb_build_array(jsonb_build_object('date',v_slot.reservation_date,'hour',v_slot.start_hour,'reason','outside_opening_rules'));
      continue;
    end if;
    select coalesce(array_agg(number order by preferred, number), '{}'::smallint[]) into v_available
    from (
      select d.number, case when d.number = any(v_preferred) then 0 else 1 end as preferred
      from public.devices d
      where d.status = 'available'
        and not exists (select 1 from public.reservations r where r.device_number = d.number and r.reservation_date = v_slot.reservation_date and r.start_hour = v_slot.start_hour and r.status in ('reserved','confirmed','in_use','completed'))
        and not exists (select 1 from public.maintenance_windows m where m.reservation_date = v_slot.reservation_date and m.status in ('scheduled','active') and m.start_hour < v_slot.start_hour + 1 and m.end_hour > v_slot.start_hour and (m.device_number is null or m.device_number = d.number))
        and not exists (select 1 from public.reservations r where r.user_id = auth.uid() and r.reservation_date = v_slot.reservation_date and r.start_hour = v_slot.start_hour and r.status in ('reserved','confirmed','in_use','completed'))
    ) available;
    if cardinality(v_available) < v_count then
      v_failures := v_failures || jsonb_build_array(jsonb_build_object('date',v_slot.reservation_date,'hour',v_slot.start_hour,'reason','insufficient_available_devices','available',cardinality(v_available)));
      continue;
    end if;
    v_slots := v_slots || jsonb_build_array(jsonb_build_object('reservation_date',v_slot.reservation_date,'start_hour',v_slot.start_hour));
    v_rotation := v_rotation || jsonb_build_array(jsonb_build_object('reservation_date',v_slot.reservation_date,'start_hour',v_slot.start_hour,'device_numbers',to_jsonb(v_available[1:v_count])));
    if v_common is null then v_common := v_available; else select coalesce(array_agg(x), '{}'::smallint[]) into v_common from unnest(v_common) x where x = any(v_available); end if;
    v_total := v_total + v_count;
  end loop;
  if jsonb_array_length(v_slots) * v_count > 360 then raise exception 'A plan may contain at most 360 device-hours'; end if;
  if jsonb_array_length(v_failures) > 0 or jsonb_array_length(v_slots) = 0 then
    return jsonb_build_object('candidates','[]'::jsonb,'failures',v_failures,'limits',jsonb_build_object('max_days',90,'max_device_hours',360));
  end if;
  v_candidates := v_candidates || jsonb_build_array(jsonb_build_object('rank',1,'allocation_strategy','rotation','items',v_rotation,'total_device_hours',v_total));
  if cardinality(v_common) >= v_count then
    for v_slot in select * from jsonb_to_recordset(v_slots) as x(reservation_date date, start_hour smallint) loop
      v_fixed := v_fixed || jsonb_build_array(jsonb_build_object('reservation_date',v_slot.reservation_date,'start_hour',v_slot.start_hour,'device_numbers',to_jsonb(v_common[1:v_count])));
    end loop;
    v_candidates := jsonb_build_array(jsonb_build_object('rank',1,'allocation_strategy','fixed_device','items',v_fixed,'total_device_hours',v_total)) ||
                    (select jsonb_agg(value || jsonb_build_object('rank',(value->>'rank')::integer + 1)) from jsonb_array_elements(v_candidates));
  end if;
  return jsonb_build_object('candidates',v_candidates,'failures','[]'::jsonb,'limits',jsonb_build_object('max_days',90,'max_device_hours',360));
end;
$$;

create or replace function public.v3_create_booking_proposal(p_session_id uuid, p_candidate jsonb)
returns jsonb language plpgsql security definer set search_path = public
as $$
declare v_proposal_id uuid; v_item jsonb; v_device text; v_count integer := 0;
begin
  if not exists(select 1 from public.agent_sessions where id = p_session_id and user_id = auth.uid() and agent_type = 'booking_planner' and expires_at > now()) then raise exception 'The booking session is unavailable or expired'; end if;
  if coalesce(p_candidate->>'allocation_strategy','') not in ('rotation','fixed_device') or jsonb_typeof(p_candidate->'items') <> 'array' then raise exception 'Invalid deterministic schedule candidate'; end if;
  insert into public.agent_proposals(user_id,session_id,agent_type,action_type,state,payload)
  values(auth.uid(),p_session_id,'booking_planner','create_reservation_plan','draft',jsonb_build_object('allocation_strategy',p_candidate->>'allocation_strategy')) returning id into v_proposal_id;
  for v_item in select value from jsonb_array_elements(p_candidate->'items') loop
    for v_device in select value from jsonb_array_elements_text(v_item->'device_numbers') loop
      insert into public.agent_proposal_items(proposal_id,reservation_date,start_hour,device_number)
      values(v_proposal_id,(v_item->>'reservation_date')::date,(v_item->>'start_hour')::smallint,v_device::smallint);
      v_count := v_count + 1;
    end loop;
  end loop;
  if v_count = 0 or v_count > 360 then raise exception 'Invalid proposal item count'; end if;
  update public.agent_proposals set state = 'ready' where id = v_proposal_id;
  return (select jsonb_build_object('proposalId',id,'state',state,'expiresAt',expires_at,'actionType',action_type,'itemCount',v_count) from public.agent_proposals where id = v_proposal_id);
end;
$$;

create or replace function public.v3_create_reservation_plan(p_proposal_id uuid, p_idempotency_key uuid)
returns jsonb language plpgsql security definer set search_path = public
as $$
declare v_proposal public.agent_proposals%rowtype; v_existing public.agent_execution_keys%rowtype; v_item record; v_block_id uuid; v_series_id uuid; v_ids uuid[] := '{}'; v_reservation_id uuid; v_device smallint;
begin
  if p_idempotency_key is null then raise exception 'An idempotency key is required'; end if;
  insert into public.agent_execution_keys(user_id,idempotency_key,proposal_id) values(auth.uid(),p_idempotency_key,p_proposal_id) on conflict(user_id,idempotency_key) do nothing;
  select * into v_existing from public.agent_execution_keys where user_id = auth.uid() and idempotency_key = p_idempotency_key for update;
  if v_existing.proposal_id <> p_proposal_id then raise exception 'The idempotency key belongs to another proposal'; end if;
  if v_existing.result is not null then return v_existing.result; end if;
  select * into v_proposal from public.agent_proposals where id = p_proposal_id and user_id = auth.uid() for update;
  if not found then raise exception 'The proposal was not found'; end if;
  if v_proposal.state = 'executed' and v_proposal.result is not null then return v_proposal.result; end if;
  if v_proposal.action_type <> 'create_reservation_plan' or v_proposal.state <> 'ready' then raise exception 'The proposal is not ready for booking'; end if;
  if v_proposal.expires_at <= now() then update public.agent_proposals set state = 'expired' where id = p_proposal_id; raise exception 'The proposal has expired'; end if;
  perform 1 from public.devices d where d.number in (select distinct device_number from public.agent_proposal_items where proposal_id = p_proposal_id) order by d.number for update;
  if exists(select 1 from public.agent_proposal_items i where i.proposal_id = p_proposal_id and (i.reservation_date < (now() at time zone 'Asia/Shanghai')::date or (i.reservation_date = (now() at time zone 'Asia/Shanghai')::date and i.start_hour <= extract(hour from now() at time zone 'Asia/Shanghai')))) then raise exception 'A planned slot has already started'; end if;
  if exists(select 1 from public.agent_proposal_items i join public.devices d on d.number=i.device_number where i.proposal_id=p_proposal_id and d.status <> 'available') then raise exception 'A proposed device is no longer available'; end if;
  if exists(select 1 from public.agent_proposal_items i where i.proposal_id=p_proposal_id and (not exists(select 1 from public.lab_opening_rules r where r.active and r.weekday=extract(dow from i.reservation_date)::smallint and i.start_hour>=r.start_hour and i.start_hour+1<=r.end_hour) or exists(select 1 from public.maintenance_windows m where m.reservation_date=i.reservation_date and m.status in ('scheduled','active') and m.start_hour<i.start_hour+1 and m.end_hour>i.start_hour and (m.device_number is null or m.device_number=i.device_number)) or exists(select 1 from public.reservations r where r.device_number=i.device_number and r.reservation_date=i.reservation_date and r.start_hour=i.start_hour and r.status in ('reserved','confirmed','in_use','completed')))) then raise exception 'The plan has become unavailable; request a new proposal'; end if;
  if exists(select 1 from public.agent_proposal_items i join public.reservations r on r.user_id=auth.uid() and r.reservation_date=i.reservation_date and r.start_hour=i.start_hour and r.status in ('reserved','confirmed','in_use','completed') where i.proposal_id=p_proposal_id) then raise exception 'The user already has a booking in a planned time slot'; end if;
  insert into public.reservation_series(user_id,recurrence,start_date,end_date) select auth.uid(),v_proposal.payload, min(reservation_date),max(reservation_date) from public.agent_proposal_items where proposal_id=p_proposal_id returning id into v_series_id;
  for v_item in select reservation_date,start_hour,array_agg(device_number order by device_number) as devices from public.agent_proposal_items where proposal_id=p_proposal_id group by reservation_date,start_hour order by reservation_date,start_hour loop
    v_block_id := public.v3_open_booking_block(auth.uid(),v_item.reservation_date,v_item.start_hour);
    foreach v_device in array v_item.devices loop
      insert into public.reservations(device_number,user_id,reservation_date,start_hour,status,series_id,booking_block_id) values(v_device,auth.uid(),v_item.reservation_date,v_item.start_hour,'confirmed',v_series_id,v_block_id) returning id into v_reservation_id;
      insert into public.reservation_status_events(reservation_id,from_status,to_status,actor_id,reason) values(v_reservation_id,null,'confirmed',auth.uid(),'Created from Agent V3 proposal');
      v_ids := array_append(v_ids,v_reservation_id);
    end loop;
  end loop;
  update public.agent_proposals set state='executed',executed_at=now(),result=jsonb_build_object('reservationIds',to_jsonb(v_ids),'seriesId',v_series_id) where id=p_proposal_id;
  update public.agent_execution_keys set result=(select result from public.agent_proposals where id=p_proposal_id) where user_id=auth.uid() and idempotency_key=p_idempotency_key;
  return (select result from public.agent_proposals where id=p_proposal_id);
exception when unique_violation then raise exception 'The plan conflicts with a concurrent booking; request a new proposal';
end;
$$;

create or replace function public.v3_cancel_reservation_series(p_series_id uuid)
returns jsonb language plpgsql security definer set search_path = public
as $$
declare v_count integer; v_block uuid;
begin
  if not exists(select 1 from public.reservation_series where id=p_series_id and (user_id=auth.uid() or public.is_admin())) then raise exception 'The reservation series was not found'; end if;
  with target as (select id,status as old_status from public.reservations where series_id=p_series_id and status in ('reserved','confirmed') and (reservation_date > (now() at time zone 'Asia/Shanghai')::date or (reservation_date=(now() at time zone 'Asia/Shanghai')::date and start_hour>extract(hour from now() at time zone 'Asia/Shanghai'))) for update), changed as (update public.reservations r set status='cancelled',cancelled_at=now() from target t where r.id=t.id returning r.id,t.old_status) insert into public.reservation_status_events(reservation_id,from_status,to_status,actor_id,reason) select id,old_status,'cancelled',auth.uid(),'Cancelled reservation series' from changed;
  get diagnostics v_count = row_count;
  update public.reservation_series set status='cancelled',cancelled_at=now() where id=p_series_id;
  for v_block in select distinct booking_block_id from public.reservations where series_id=p_series_id loop perform public.v3_close_booking_block_if_empty(v_block); end loop;
  return jsonb_build_object('seriesId',p_series_id,'cancelledCount',v_count);
end;
$$;

-- Fault actions are persisted proposals. They never reschedule or cancel affected reservations.
create or replace function public.v3_create_fault_proposal(p_session_id uuid, p_action_type text, p_issue_id uuid, p_note text)
returns jsonb language plpgsql security definer set search_path=public
as $$
declare v_proposal_id uuid;
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if p_action_type not in ('isolate_fault','complete_fault_repair') or p_issue_id is null then raise exception 'Invalid fault proposal'; end if;
  if char_length(trim(coalesce(p_note,''))) not between 1 and 200 then raise exception 'A fault proposal note must contain 1 to 200 characters'; end if;
  if not exists(select 1 from public.agent_sessions where id=p_session_id and user_id=auth.uid() and agent_type='fault_responder' and expires_at>now()) then raise exception 'The fault session is unavailable or expired'; end if;
  if not exists(select 1 from public.issue_reports where id=p_issue_id) then raise exception 'The issue was not found'; end if;
  insert into public.agent_proposals(user_id,session_id,agent_type,action_type,state,payload)
  values(auth.uid(),p_session_id,'fault_responder',p_action_type,'ready',jsonb_build_object('issue_id',p_issue_id,'note',trim(p_note),'auto_reschedule',false)) returning id into v_proposal_id;
  return (select jsonb_build_object('proposalId',id,'state',state,'expiresAt',expires_at,'actionType',action_type) from public.agent_proposals where id=v_proposal_id);
end;
$$;

create or replace function public.v3_execute_agent_proposal(p_proposal_id uuid, p_idempotency_key uuid)
returns jsonb language plpgsql security definer set search_path=public
as $$
declare v_proposal public.agent_proposals%rowtype; v_existing public.agent_execution_keys%rowtype; v_result jsonb; v_issue uuid; v_note text;
begin
  if p_idempotency_key is null then raise exception 'An idempotency key is required'; end if;
  insert into public.agent_execution_keys(user_id,idempotency_key,proposal_id) values(auth.uid(),p_idempotency_key,p_proposal_id) on conflict(user_id,idempotency_key) do nothing;
  select * into v_existing from public.agent_execution_keys where user_id=auth.uid() and idempotency_key=p_idempotency_key for update;
  if v_existing.proposal_id<>p_proposal_id then raise exception 'The idempotency key belongs to another proposal'; end if;
  if v_existing.result is not null then return v_existing.result; end if;
  select * into v_proposal from public.agent_proposals where id=p_proposal_id and user_id=auth.uid() for update;
  if not found then raise exception 'The proposal was not found'; end if;
  if v_proposal.expires_at<=now() then update public.agent_proposals set state='expired' where id=p_proposal_id; raise exception 'The proposal has expired'; end if;
  if v_proposal.state<>'ready' then raise exception 'The proposal is not ready for execution'; end if;
  if v_proposal.action_type='create_reservation_plan' then return public.v3_create_reservation_plan(p_proposal_id,p_idempotency_key); end if;
  if not public.is_admin() or v_proposal.agent_type<>'fault_responder' then raise exception 'Administrator access is required'; end if;
  v_issue:=(v_proposal.payload->>'issue_id')::uuid; v_note:=v_proposal.payload->>'note';
  if v_proposal.action_type='isolate_fault' then v_result:=public.v2_isolate_fault(v_issue,v_note); else perform public.v2_complete_fault_repair(v_issue,v_note); v_result:=jsonb_build_object('issueId',v_issue,'status','resolved'); end if;
  update public.agent_proposals set state='executed',executed_at=now(),result=v_result where id=p_proposal_id;
  update public.agent_execution_keys set result=v_result where user_id=auth.uid() and idempotency_key=p_idempotency_key;
  return v_result;
end;
$$;

-- Old entry points now create a group block and therefore cannot bypass the per-user time guard.
create or replace function public.v2_create_reservations(p_date date,p_start_hour smallint,p_device_numbers smallint[],p_idempotency_key uuid)
returns table(reservation_id uuid,device_number smallint,status text) language plpgsql security definer set search_path=public
as $$
declare v_fingerprint text:=md5(coalesce(p_date::text,'')||':'||coalesce(p_start_hour::text,'')||':'||coalesce(array_to_string(p_device_numbers,','),'')); v_existing text; v_ids uuid[]; v_device smallint; v_id uuid; v_block uuid; v_created uuid[]:='{}';
begin
 if p_idempotency_key is null then raise exception 'An idempotency key is required'; end if;
 insert into public.reservation_idempotency_keys(user_id,idempotency_key,request_fingerprint) values(auth.uid(),p_idempotency_key,v_fingerprint) on conflict(user_id,idempotency_key) do nothing;
 select request_fingerprint,reservation_ids into v_existing,v_ids from public.reservation_idempotency_keys where user_id=auth.uid() and idempotency_key=p_idempotency_key for update;
 if v_existing<>v_fingerprint then raise exception 'The idempotency key was already used for another request'; end if;
 if cardinality(v_ids)>0 then return query select r.id,r.device_number,r.status from public.reservations r where r.id=any(v_ids) order by r.device_number; return; end if;
 perform public.v2_assert_booking_request(p_date,p_start_hour,p_device_numbers);
 if exists(select 1 from public.reservations r where r.user_id=auth.uid() and r.reservation_date=p_date and r.start_hour=p_start_hour and r.status in ('reserved','confirmed','in_use','completed')) then raise exception 'The user already has a booking in this time slot'; end if;
 v_block:=public.v3_open_booking_block(auth.uid(),p_date,p_start_hour);
 foreach v_device in array p_device_numbers loop insert into public.reservations(device_number,user_id,reservation_date,start_hour,status,booking_block_id) values(v_device,auth.uid(),p_date,p_start_hour,'confirmed',v_block) returning id into v_id; insert into public.reservation_status_events(reservation_id,from_status,to_status,actor_id,reason) values(v_id,null,'confirmed',auth.uid(),'Created by V2 reservation service'); v_created:=array_append(v_created,v_id); end loop;
 update public.reservation_idempotency_keys set reservation_ids=v_created where user_id=auth.uid() and idempotency_key=p_idempotency_key;
 return query select r.id,r.device_number,r.status from public.reservations r where r.id=any(v_created) order by r.device_number;
exception when unique_violation then raise exception 'A selected device was just reserved by another user'; end;
$$;

create or replace function public.reserve_hourly_devices(requested_date date,requested_hour smallint,requested_devices smallint[])
returns table(reservation_id uuid,device_number smallint) language sql security definer set search_path=public
as $$ select reservation_id,device_number from public.v2_create_reservations(requested_date,requested_hour,requested_devices,gen_random_uuid()); $$;

create or replace function public.v2_cancel_reservation(p_reservation_id uuid)
returns void language plpgsql security definer set search_path=public
as $$ declare v public.reservations%rowtype; begin select * into v from public.reservations where id=p_reservation_id for update; if not found or (v.user_id<>auth.uid() and not public.is_admin()) then raise exception 'The reservation was not found'; end if; if v.status='cancelled' then return; end if; if v.status not in ('reserved','confirmed') then raise exception 'Only an unstarted confirmed reservation can be cancelled'; end if; if v.reservation_date<(now() at time zone 'Asia/Shanghai')::date or (v.reservation_date=(now() at time zone 'Asia/Shanghai')::date and v.start_hour<=extract(hour from now() at time zone 'Asia/Shanghai')) then raise exception 'A reservation cannot be cancelled after it has started'; end if; update public.reservations set status='cancelled',cancelled_at=now() where id=p_reservation_id; insert into public.reservation_status_events(reservation_id,from_status,to_status,actor_id,reason) values(p_reservation_id,v.status,'cancelled',auth.uid(),'Cancelled by reservation service'); perform public.v3_close_booking_block_if_empty(v.booking_block_id); end; $$;

create or replace function public.v2_reschedule_reservation(p_reservation_id uuid,p_date date,p_start_hour smallint,p_device_number smallint,p_idempotency_key uuid)
returns table(reservation_id uuid,device_number smallint,status text) language plpgsql security definer set search_path=public
as $$
declare v_old public.reservations%rowtype; v_new_id uuid; v_block uuid; v_fingerprint text:=md5(coalesce(p_reservation_id::text,'')||':'||coalesce(p_date::text,'')||':'||coalesce(p_start_hour::text,'')||':'||coalesce(p_device_number::text,'')); v_existing text; v_ids uuid[];
begin
  if p_idempotency_key is null then raise exception 'An idempotency key is required'; end if;
  insert into public.reservation_idempotency_keys(user_id,idempotency_key,request_fingerprint) values(auth.uid(),p_idempotency_key,v_fingerprint) on conflict(user_id,idempotency_key) do nothing;
  select request_fingerprint,reservation_ids into v_existing,v_ids from public.reservation_idempotency_keys where user_id=auth.uid() and idempotency_key=p_idempotency_key for update;
  if v_existing<>v_fingerprint then raise exception 'The idempotency key was already used for another request'; end if;
  if cardinality(v_ids)>0 then return query select r.id,r.device_number,r.status from public.reservations r where r.id=any(v_ids); return; end if;
  select * into v_old from public.reservations where id=p_reservation_id for update;
  if not found or (v_old.user_id<>auth.uid() and not public.is_admin()) then raise exception 'The reservation was not found'; end if;
  if v_old.status not in ('reserved','confirmed') then raise exception 'Only an unstarted confirmed reservation can be modified'; end if;
  if v_old.reservation_date<(now() at time zone 'Asia/Shanghai')::date or (v_old.reservation_date=(now() at time zone 'Asia/Shanghai')::date and v_old.start_hour<=extract(hour from now() at time zone 'Asia/Shanghai')) then raise exception 'A started reservation cannot be modified'; end if;
  if v_old.reservation_date=p_date and v_old.start_hour=p_start_hour and v_old.device_number=p_device_number then update public.reservation_idempotency_keys set reservation_ids=array[p_reservation_id] where user_id=auth.uid() and idempotency_key=p_idempotency_key; return query select v_old.id,v_old.device_number,v_old.status; return; end if;
  perform public.v2_assert_booking_request(p_date,p_start_hour,array[p_device_number]);
  if exists(select 1 from public.reservations r where r.user_id=v_old.user_id and r.reservation_date=p_date and r.start_hour=p_start_hour and r.id<>p_reservation_id and r.status in ('reserved','confirmed','in_use','completed')) then raise exception 'The user already has a booking in this time slot'; end if;
  v_block:=public.v3_open_booking_block(v_old.user_id,p_date,p_start_hour);
  update public.reservations set status='cancelled',cancelled_at=now() where id=p_reservation_id;
  insert into public.reservation_status_events(reservation_id,from_status,to_status,actor_id,reason) values(p_reservation_id,v_old.status,'cancelled',auth.uid(),'Rescheduled by V2 reservation service');
  perform public.v3_close_booking_block_if_empty(v_old.booking_block_id);
  insert into public.reservations(device_number,user_id,reservation_date,start_hour,status,booking_block_id) values(p_device_number,v_old.user_id,p_date,p_start_hour,'confirmed',v_block) returning id into v_new_id;
  insert into public.reservation_status_events(reservation_id,from_status,to_status,actor_id,reason) values(v_new_id,null,'confirmed',auth.uid(),'Created by reschedule operation');
  update public.reservation_idempotency_keys set reservation_ids=array[v_new_id] where user_id=auth.uid() and idempotency_key=p_idempotency_key;
  return query select v_new_id,p_device_number,'confirmed'::text;
exception when unique_violation then raise exception 'The requested replacement slot was just reserved by another user'; end;
$$;

-- V3 audit accepts old values and the three explicit agent types.
alter table public.agent_audit_logs drop constraint if exists agent_audit_logs_agent_type_check;
alter table public.agent_audit_logs add constraint agent_audit_logs_agent_type_check check(agent_type in ('booking','admin','booking_planner','admin_analyst','fault_responder'));
create or replace function public.v3_log_agent_interaction(p_request_id uuid,p_agent_type text,p_request_payload jsonb,p_response_payload jsonb,p_outcome text,p_error_message text default null)
returns void language plpgsql security definer set search_path=public
as $$ begin if p_agent_type not in ('booking_planner','admin_analyst','fault_responder') or p_outcome not in ('suggested','confirmed','fallback','failed') then raise exception 'Invalid agent audit values'; end if; if p_agent_type in ('admin_analyst','fault_responder') and not public.is_admin() then raise exception 'Administrator access is required'; end if; insert into public.agent_audit_logs(request_id,user_id,agent_type,request_payload,response_payload,outcome,error_message) values(p_request_id,auth.uid(),p_agent_type,coalesce(p_request_payload,'{}'::jsonb),coalesce(p_response_payload,'{}'::jsonb),p_outcome,p_error_message); end; $$;

revoke all on function public.v3_open_booking_block(uuid,date,smallint) from public;
revoke all on function public.v3_close_booking_block_if_empty(uuid) from public;
revoke all on function public.v3_save_agent_session(uuid,text,jsonb) from public;
revoke all on function public.v3_plan_schedule(jsonb) from public;
revoke all on function public.v3_create_booking_proposal(uuid,jsonb) from public;
revoke all on function public.v3_create_reservation_plan(uuid,uuid) from public;
revoke all on function public.v3_cancel_reservation_series(uuid) from public;
revoke all on function public.v3_create_fault_proposal(uuid,text,uuid,text) from public;
revoke all on function public.v3_execute_agent_proposal(uuid,uuid) from public;
revoke all on function public.v3_log_agent_interaction(uuid,text,jsonb,jsonb,text,text) from public;
grant select on public.reservation_series,public.booking_blocks,public.agent_sessions,public.agent_proposals,public.agent_proposal_items to authenticated;
grant execute on function public.v3_save_agent_session(uuid,text,jsonb),public.v3_plan_schedule(jsonb),public.v3_create_booking_proposal(uuid,jsonb),public.v3_create_reservation_plan(uuid,uuid),public.v3_cancel_reservation_series(uuid),public.v3_create_fault_proposal(uuid,text,uuid,text),public.v3_execute_agent_proposal(uuid,uuid),public.v3_log_agent_interaction(uuid,text,jsonb,jsonb,text,text) to authenticated;

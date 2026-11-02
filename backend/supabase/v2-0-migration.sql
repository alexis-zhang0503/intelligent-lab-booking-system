-- V2.0 additive migration for the existing hourly reservation contract.
-- Prerequisite: schema.sql and cloud-migration.sql. Do not edit historical migrations.
-- This file does not delete rows or replace public.reservations.

create extension if not exists pgcrypto;

-- Keep legacy `reserved` rows valid. V2 writes `confirmed` for all new reservations.
do $$
begin
  if exists (
    select 1 from public.reservations
    where status not in ('reserved', 'confirmed', 'in_use', 'completed', 'cancelled', 'no_show')
  ) then
    raise exception 'Unknown reservation statuses exist; inspect public.reservations before applying V2.';
  end if;
end;
$$;

alter table public.reservations drop constraint if exists reservations_status_check;
alter table public.reservations add constraint reservations_status_check
  check (status in ('reserved', 'confirmed', 'in_use', 'completed', 'cancelled', 'no_show'));

-- The original guard remains untouched for legacy callers. This guard covers V2 states.
create unique index if not exists v2_active_reservation_conflict_guard
  on public.reservations (device_number, reservation_date, start_hour)
  where status in ('reserved', 'confirmed', 'in_use', 'completed');

create table if not exists public.lab_opening_rules (
  id uuid primary key default gen_random_uuid(),
  weekday smallint not null check (weekday between 0 and 6),
  start_hour smallint not null check (start_hour between 0 and 23),
  end_hour smallint not null check (end_hour between 1 and 24 and end_hour > start_hour),
  active boolean not null default true,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now(),
  unique (weekday, start_hour, end_hour)
);

-- Preserve the assignment's original 08:00-17:00 schedule until an administrator changes it.
insert into public.lab_opening_rules (weekday, start_hour, end_hour)
select weekday, 8, 17 from generate_series(0, 6) as weekday
on conflict (weekday, start_hour, end_hour) do nothing;

create table if not exists public.maintenance_windows (
  id uuid primary key default gen_random_uuid(),
  device_number smallint references public.devices(number),
  reservation_date date not null,
  start_hour smallint not null check (start_hour between 0 and 23),
  end_hour smallint not null check (end_hour between 1 and 24 and end_hour > start_hour),
  reason text not null check (char_length(trim(reason)) between 1 and 200),
  status text not null default 'scheduled' check (status in ('scheduled', 'active', 'completed', 'cancelled')),
  created_by uuid references public.profiles(id),
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

create index if not exists maintenance_windows_slot_lookup
  on public.maintenance_windows (reservation_date, start_hour, end_hour, device_number)
  where status in ('scheduled', 'active');

create table if not exists public.reservation_usage_records (
  reservation_id uuid primary key references public.reservations(id) on delete cascade,
  checked_in_at timestamptz,
  checked_in_by uuid references public.profiles(id),
  checked_out_at timestamptz,
  checked_out_by uuid references public.profiles(id),
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now(),
  check (checked_out_at is null or checked_in_at is not null),
  check (checked_out_at is null or checked_out_at >= checked_in_at)
);

create table if not exists public.reservation_status_events (
  id uuid primary key default gen_random_uuid(),
  reservation_id uuid not null references public.reservations(id) on delete cascade,
  from_status text,
  to_status text not null check (to_status in ('reserved', 'confirmed', 'in_use', 'completed', 'cancelled', 'no_show')),
  actor_id uuid references public.profiles(id),
  reason text,
  created_at timestamptz not null default now()
);

create index if not exists reservation_status_events_reservation_created
  on public.reservation_status_events (reservation_id, created_at desc);

create table if not exists public.reservation_idempotency_keys (
  user_id uuid not null references public.profiles(id) on delete cascade,
  idempotency_key uuid not null,
  request_fingerprint text not null,
  reservation_ids uuid[] not null default '{}',
  created_at timestamptz not null default now(),
  primary key (user_id, idempotency_key)
);

create table if not exists public.agent_audit_logs (
  id uuid primary key default gen_random_uuid(),
  request_id uuid not null,
  user_id uuid not null references public.profiles(id) on delete cascade,
  agent_type text not null check (agent_type in ('booking', 'admin')),
  request_payload jsonb not null default '{}'::jsonb,
  response_payload jsonb not null default '{}'::jsonb,
  outcome text not null check (outcome in ('suggested', 'confirmed', 'fallback', 'failed')),
  error_message text,
  created_at timestamptz not null default now()
);

create index if not exists agent_audit_logs_user_created on public.agent_audit_logs (user_id, created_at desc);

create table if not exists public.scheduling_plan_logs (
  id uuid primary key default gen_random_uuid(),
  requester_id uuid not null references public.profiles(id) on delete cascade,
  requested_date date not null,
  preferred_hour smallint,
  required_device_count smallint not null check (required_device_count between 1 and 40),
  candidates jsonb not null,
  created_at timestamptz not null default now()
);

alter table public.lab_opening_rules enable row level security;
alter table public.maintenance_windows enable row level security;
alter table public.reservation_usage_records enable row level security;
alter table public.reservation_status_events enable row level security;
alter table public.reservation_idempotency_keys enable row level security;
alter table public.agent_audit_logs enable row level security;
alter table public.scheduling_plan_logs enable row level security;

drop policy if exists "authenticated read opening rules" on public.lab_opening_rules;
create policy "authenticated read opening rules" on public.lab_opening_rules
  for select to authenticated using (true);
drop policy if exists "administrators manage opening rules" on public.lab_opening_rules;
create policy "administrators manage opening rules" on public.lab_opening_rules
  for all to authenticated using (public.is_admin()) with check (public.is_admin());

drop policy if exists "authenticated read maintenance windows" on public.maintenance_windows;
create policy "authenticated read maintenance windows" on public.maintenance_windows
  for select to authenticated using (true);
drop policy if exists "administrators manage maintenance windows" on public.maintenance_windows;
create policy "administrators manage maintenance windows" on public.maintenance_windows
  for all to authenticated using (public.is_admin()) with check (public.is_admin());

drop policy if exists "users read usage for their reservations" on public.reservation_usage_records;
create policy "users read usage for their reservations" on public.reservation_usage_records
  for select to authenticated using (
    exists (select 1 from public.reservations r where r.id = reservation_id and r.user_id = auth.uid())
    or public.is_admin()
  );
drop policy if exists "users read their reservation events" on public.reservation_status_events;
create policy "users read their reservation events" on public.reservation_status_events
  for select to authenticated using (
    exists (select 1 from public.reservations r where r.id = reservation_id and r.user_id = auth.uid())
    or public.is_admin()
  );
drop policy if exists "users read their agent audit logs" on public.agent_audit_logs;
create policy "users read their agent audit logs" on public.agent_audit_logs
  for select to authenticated using (user_id = auth.uid() or public.is_admin());
drop policy if exists "administrators read scheduling plan logs" on public.scheduling_plan_logs;
create policy "administrators read scheduling plan logs" on public.scheduling_plan_logs
  for select to authenticated using (public.is_admin());

-- Helper used only by security-definer business functions below.
create or replace function public.v2_assert_booking_request(
  p_date date,
  p_start_hour smallint,
  p_device_numbers smallint[]
)
returns void language plpgsql security definer set search_path = public
as $$
declare
  requested_count integer := coalesce(array_length(p_device_numbers, 1), 0);
begin
  if not exists (select 1 from public.profiles where id = auth.uid() and active) then
    raise exception 'This account is inactive';
  end if;
  if p_date < current_date or (p_date = current_date and p_start_hour <= extract(hour from current_time)) then
    raise exception 'The requested reservation time has already started';
  end if;
  if requested_count = 0 or requested_count <> (select count(distinct value) from unnest(p_device_numbers) as value) then
    raise exception 'Select one or more distinct devices';
  end if;
  if not exists (
    select 1 from public.lab_opening_rules r
    where r.active and r.weekday = extract(dow from p_date)
      and p_start_hour >= r.start_hour and p_start_hour + 1 <= r.end_hour
  ) then
    raise exception 'The requested time is outside the laboratory opening rules';
  end if;
  if exists (
    select 1 from unnest(p_device_numbers) as requested(device_number)
    left join public.devices d on d.number = requested.device_number
    where d.number is null or d.status <> 'available'
  ) then
    raise exception 'At least one selected device is unavailable';
  end if;
  if exists (
    select 1 from public.maintenance_windows m
    where m.reservation_date = p_date
      and m.status in ('scheduled', 'active')
      and m.start_hour < p_start_hour + 1 and m.end_hour > p_start_hour
      and (m.device_number is null or m.device_number = any(p_device_numbers))
  ) then
    raise exception 'The requested time overlaps a maintenance window';
  end if;
end;
$$;

create or replace function public.v2_create_reservations(
  p_date date,
  p_start_hour smallint,
  p_device_numbers smallint[],
  p_idempotency_key uuid
)
returns table (reservation_id uuid, device_number smallint, status text)
language plpgsql security definer set search_path = public
as $$
declare
  v_fingerprint text := md5(coalesce(p_date::text, '') || ':' || coalesce(p_start_hour::text, '') || ':' || coalesce(array_to_string(p_device_numbers, ','), ''));
  v_existing_fingerprint text;
  v_existing_ids uuid[];
  v_device smallint;
  v_reservation_id uuid;
  v_created_ids uuid[] := '{}';
begin
  if p_idempotency_key is null then
    raise exception 'An idempotency key is required';
  end if;
  insert into public.reservation_idempotency_keys (user_id, idempotency_key, request_fingerprint)
  values (auth.uid(), p_idempotency_key, v_fingerprint)
  on conflict (user_id, idempotency_key) do nothing;
  select request_fingerprint, reservation_ids into v_existing_fingerprint, v_existing_ids
    from public.reservation_idempotency_keys
    where user_id = auth.uid() and idempotency_key = p_idempotency_key
    for update;
  if v_existing_fingerprint <> v_fingerprint then
    raise exception 'The idempotency key was already used for another request';
  end if;
  if cardinality(v_existing_ids) > 0 then
    return query select r.id, r.device_number, r.status from public.reservations r where r.id = any(v_existing_ids) order by r.device_number;
    return;
  end if;
  perform public.v2_assert_booking_request(p_date, p_start_hour, p_device_numbers);
  foreach v_device in array p_device_numbers loop
    insert into public.reservations (device_number, user_id, reservation_date, start_hour, status)
    values (v_device, auth.uid(), p_date, p_start_hour, 'confirmed')
    returning id into v_reservation_id;
    insert into public.reservation_status_events (reservation_id, from_status, to_status, actor_id, reason)
    values (v_reservation_id, null, 'confirmed', auth.uid(), 'Created by V2 reservation service');
    v_created_ids := array_append(v_created_ids, v_reservation_id);
  end loop;
  update public.reservation_idempotency_keys
    set reservation_ids = v_created_ids
    where user_id = auth.uid() and idempotency_key = p_idempotency_key;
  return query select r.id, r.device_number, r.status from public.reservations r where r.id = any(v_created_ids) order by r.device_number;
exception when unique_violation then
  raise exception 'A selected device was just reserved by another user';
end;
$$;

create or replace function public.v2_cancel_reservation(p_reservation_id uuid)
returns void language plpgsql security definer set search_path = public
as $$
declare v_reservation public.reservations%rowtype;
begin
  select * into v_reservation from public.reservations where id = p_reservation_id for update;
  if not found or (v_reservation.user_id <> auth.uid() and not public.is_admin()) then
    raise exception 'The reservation was not found';
  end if;
  if v_reservation.status = 'cancelled' then return; end if;
  if v_reservation.status not in ('reserved', 'confirmed') then
    raise exception 'Only an unstarted confirmed reservation can be cancelled';
  end if;
  if v_reservation.reservation_date < current_date
     or (v_reservation.reservation_date = current_date and v_reservation.start_hour <= extract(hour from current_time)) then
    raise exception 'A reservation cannot be cancelled after it has started';
  end if;
  update public.reservations set status = 'cancelled', cancelled_at = now() where id = p_reservation_id;
  insert into public.reservation_status_events (reservation_id, from_status, to_status, actor_id, reason)
  values (p_reservation_id, v_reservation.status, 'cancelled', auth.uid(), 'Cancelled by reservation service');
end;
$$;

-- Extend the existing administrator deactivation operation so V2 confirmed slots are released too.
create or replace function public.deactivate_student(target_user uuid)
returns void language plpgsql security definer set search_path = public
as $$
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if not exists (select 1 from public.profiles where id = target_user and role = 'student' and active) then
    raise exception 'The selected active student was not found';
  end if;
  update public.profiles set active = false where id = target_user;
  update public.reservations set status = 'cancelled', cancelled_at = now()
    where user_id = target_user and status in ('reserved', 'confirmed');
end;
$$;

create or replace function public.v2_reschedule_reservation(
  p_reservation_id uuid,
  p_date date,
  p_start_hour smallint,
  p_device_number smallint,
  p_idempotency_key uuid
)
returns table (reservation_id uuid, device_number smallint, status text)
language plpgsql security definer set search_path = public
as $$
declare
  v_old public.reservations%rowtype;
  v_new_id uuid;
  v_fingerprint text := md5(coalesce(p_reservation_id::text, '') || ':' || coalesce(p_date::text, '') || ':' || coalesce(p_start_hour::text, '') || ':' || coalesce(p_device_number::text, ''));
  v_existing_fingerprint text;
  v_existing_ids uuid[];
begin
  if p_idempotency_key is null then raise exception 'An idempotency key is required'; end if;
  insert into public.reservation_idempotency_keys (user_id, idempotency_key, request_fingerprint)
  values (auth.uid(), p_idempotency_key, v_fingerprint)
  on conflict (user_id, idempotency_key) do nothing;
  select request_fingerprint, reservation_ids into v_existing_fingerprint, v_existing_ids
    from public.reservation_idempotency_keys where user_id = auth.uid() and idempotency_key = p_idempotency_key for update;
  if v_existing_fingerprint <> v_fingerprint then raise exception 'The idempotency key was already used for another request'; end if;
  if cardinality(v_existing_ids) > 0 then
    return query select r.id, r.device_number, r.status from public.reservations r where r.id = any(v_existing_ids);
    return;
  end if;
  select * into v_old from public.reservations where id = p_reservation_id for update;
  if not found or (v_old.user_id <> auth.uid() and not public.is_admin()) then raise exception 'The reservation was not found'; end if;
  if v_old.status not in ('reserved', 'confirmed') then raise exception 'Only an unstarted confirmed reservation can be modified'; end if;
  if v_old.reservation_date < current_date or (v_old.reservation_date = current_date and v_old.start_hour <= extract(hour from current_time)) then raise exception 'A started reservation cannot be modified'; end if;
  if v_old.reservation_date = p_date and v_old.start_hour = p_start_hour and v_old.device_number = p_device_number then
    update public.reservation_idempotency_keys set reservation_ids = array[p_reservation_id] where user_id = auth.uid() and idempotency_key = p_idempotency_key;
    return query select v_old.id, v_old.device_number, v_old.status;
    return;
  end if;
  perform public.v2_assert_booking_request(p_date, p_start_hour, array[p_device_number]);
  update public.reservations set status = 'cancelled', cancelled_at = now() where id = p_reservation_id;
  insert into public.reservation_status_events (reservation_id, from_status, to_status, actor_id, reason)
  values (p_reservation_id, v_old.status, 'cancelled', auth.uid(), 'Rescheduled by V2 reservation service');
  insert into public.reservations (device_number, user_id, reservation_date, start_hour, status)
  values (p_device_number, v_old.user_id, p_date, p_start_hour, 'confirmed') returning id into v_new_id;
  insert into public.reservation_status_events (reservation_id, from_status, to_status, actor_id, reason)
  values (v_new_id, null, 'confirmed', auth.uid(), 'Created by reschedule operation');
  update public.reservation_idempotency_keys set reservation_ids = array[v_new_id] where user_id = auth.uid() and idempotency_key = p_idempotency_key;
  return query select v_new_id, p_device_number, 'confirmed'::text;
exception when unique_violation then
  raise exception 'The requested replacement slot was just reserved by another user';
end;
$$;

create or replace function public.v2_check_in(p_reservation_id uuid)
returns void language plpgsql security definer set search_path = public
as $$
declare v_reservation public.reservations%rowtype;
begin
  select * into v_reservation from public.reservations where id = p_reservation_id for update;
  if not found or (v_reservation.user_id <> auth.uid() and not public.is_admin()) then raise exception 'The reservation was not found'; end if;
  if v_reservation.status not in ('reserved', 'confirmed', 'in_use') then raise exception 'Only a confirmed reservation can be checked in'; end if;
  if now() < v_reservation.reservation_date + make_interval(hours => v_reservation.start_hour)
     or now() >= v_reservation.reservation_date + make_interval(hours => v_reservation.start_hour + 1) then
    raise exception 'Manual check-in is only available during the booked hour';
  end if;
  insert into public.reservation_usage_records (reservation_id, checked_in_at, checked_in_by)
  values (p_reservation_id, now(), auth.uid())
  on conflict (reservation_id) do nothing;
  if v_reservation.status <> 'in_use' then
    update public.reservations set status = 'in_use' where id = p_reservation_id;
    insert into public.reservation_status_events (reservation_id, from_status, to_status, actor_id, reason)
    values (p_reservation_id, v_reservation.status, 'in_use', auth.uid(), 'Manual check-in');
  end if;
end;
$$;

create or replace function public.v2_check_out(p_reservation_id uuid)
returns void language plpgsql security definer set search_path = public
as $$
declare v_reservation public.reservations%rowtype;
begin
  select * into v_reservation from public.reservations where id = p_reservation_id for update;
  if not found or (v_reservation.user_id <> auth.uid() and not public.is_admin()) then raise exception 'The reservation was not found'; end if;
  if v_reservation.status = 'completed' then return; end if;
  if v_reservation.status <> 'in_use' then raise exception 'Check-in is required before check-out'; end if;
  update public.reservation_usage_records
    set checked_out_at = coalesce(checked_out_at, now()), checked_out_by = coalesce(checked_out_by, auth.uid()), updated_at = now()
    where reservation_id = p_reservation_id;
  if not found then raise exception 'Check-in is required before check-out'; end if;
  update public.reservations set status = 'completed' where id = p_reservation_id;
  insert into public.reservation_status_events (reservation_id, from_status, to_status, actor_id, reason)
  values (p_reservation_id, 'in_use', 'completed', auth.uid(), 'Manual check-out');
end;
$$;

create or replace function public.v2_refresh_expired_reservations()
returns integer language plpgsql security definer set search_path = public
as $$
declare v_count integer;
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  with expired as (
    select id, status from public.reservations
    where status in ('reserved', 'confirmed')
      and now() >= reservation_date + make_interval(hours => start_hour + 1)
    for update
  ), changed as (
    update public.reservations r set status = 'no_show'
    from expired e where r.id = e.id
    returning r.id
  )
  insert into public.reservation_status_events (reservation_id, from_status, to_status, actor_id, reason)
  select e.id, e.status, 'no_show', auth.uid(), 'Expired without manual check-in'
  from expired e join changed c on c.id = e.id;
  get diagnostics v_count = row_count;
  return v_count;
end;
$$;

create or replace function public.v2_schedule_candidates(
  p_date date,
  p_preferred_hour smallint default null,
  p_required_device_count smallint default 1,
  p_preferred_devices smallint[] default '{}',
  p_limit smallint default 3
)
returns table (candidate_rank bigint, reservation_date date, start_hour smallint, device_numbers smallint[], score numeric, reasons jsonb)
language sql stable security definer set search_path = public
as $$
  with candidate_hours as (
    select distinct hour::smallint as start_hour
    from public.lab_opening_rules r
    cross join lateral generate_series(r.start_hour, r.end_hour - 1) as hour
    where r.active and r.weekday = extract(dow from p_date)
      and (p_date > current_date or (p_date = current_date and r.start_hour > extract(hour from current_time)))
  ), candidates as (
    select h.start_hour,
           available.device_numbers,
           cardinality(available.device_numbers) as available_count
    from candidate_hours h
    cross join lateral (
      select array_agg(d.number order by case when d.number = any(p_preferred_devices) then 0 else 1 end, d.number)::smallint[] as device_numbers
      from public.devices d
      where d.status = 'available'
        and not exists (
          select 1 from public.reservations r
          where r.device_number = d.number and r.reservation_date = p_date and r.start_hour = h.start_hour
            and r.status in ('reserved', 'confirmed', 'in_use', 'completed')
        )
        and not exists (
          select 1 from public.maintenance_windows m
          where m.reservation_date = p_date and m.status in ('scheduled', 'active')
            and m.start_hour < h.start_hour + 1 and m.end_hour > h.start_hour
            and (m.device_number is null or m.device_number = d.number)
        )
    ) available
  ), scored as (
    select start_hour,
           device_numbers[1:p_required_device_count] as selected_devices,
           (100 - 10 * abs(start_hour - coalesce(p_preferred_hour, start_hour)) + least(available_count, 40) * 0.1)::numeric as candidate_score,
           available_count
    from candidates
    where p_date >= current_date and p_required_device_count between 1 and 40 and available_count >= p_required_device_count
  )
  select row_number() over (order by candidate_score desc, start_hour), p_date, start_hour, selected_devices, candidate_score,
         jsonb_build_object('opening_rule', true, 'available_devices', available_count, 'preferred_hour_distance', abs(start_hour - coalesce(p_preferred_hour, start_hour)))
  from scored
  order by candidate_score desc, start_hour
  limit greatest(1, least(coalesce(p_limit, 3), 3));
$$;

create or replace function public.v2_log_schedule_candidates(
  p_date date, p_preferred_hour smallint, p_required_device_count smallint, p_candidates jsonb
)
returns void language plpgsql security definer set search_path = public
as $$
begin
  insert into public.scheduling_plan_logs (requester_id, requested_date, preferred_hour, required_device_count, candidates)
  values (auth.uid(), p_date, p_preferred_hour, p_required_device_count, p_candidates);
end;
$$;

create or replace function public.v2_admin_metrics(p_start_date date, p_end_date date)
returns jsonb language plpgsql security definer set search_path = public
as $$
declare
  v_capacity numeric := 0;
  v_booked numeric := 0;
  v_checked_in numeric := 0;
  v_no_show numeric := 0;
  v_cancelled numeric := 0;
  v_top_hours jsonb := '[]'::jsonb;
  v_missing_usage numeric := 0;
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if p_start_date is null or p_end_date is null or p_end_date < p_start_date then raise exception 'A valid date range is required'; end if;
  select count(*) into v_capacity
  from generate_series(p_start_date, p_end_date, interval '1 day') as days(day)
  join public.lab_opening_rules rule on rule.active and rule.weekday = extract(dow from day)::smallint
  cross join lateral generate_series(rule.start_hour, rule.end_hour - 1) as slot_hour
  join public.devices device on device.status = 'available'
  where not exists (
    select 1 from public.maintenance_windows m
    where m.reservation_date = day::date and m.status in ('scheduled', 'active')
      and m.start_hour < slot_hour + 1 and m.end_hour > slot_hour
      and (m.device_number is null or m.device_number = device.number)
  );
  select count(*) filter (where status in ('reserved', 'confirmed', 'in_use', 'completed', 'no_show')),
         count(*) filter (where status = 'no_show'),
         count(*) filter (where status = 'cancelled')
    into v_booked, v_no_show, v_cancelled
    from public.reservations where reservation_date between p_start_date and p_end_date;
  select count(*) into v_checked_in
    from public.reservations r join public.reservation_usage_records u on u.reservation_id = r.id
    where r.reservation_date between p_start_date and p_end_date and u.checked_in_at is not null;
  select count(*) into v_missing_usage
    from public.reservations r left join public.reservation_usage_records u on u.reservation_id = r.id
    where r.reservation_date between p_start_date and p_end_date and r.status in ('in_use', 'completed') and u.checked_in_at is null;
  select coalesce(jsonb_agg(row_to_json(top_slot)), '[]'::jsonb) into v_top_hours
    from (
      select start_hour, count(*) as reservations
      from public.reservations
      where reservation_date between p_start_date and p_end_date and status in ('reserved', 'confirmed', 'in_use', 'completed', 'no_show')
      group by start_hour order by reservations desc, start_hour limit 3
    ) top_slot;
  return jsonb_build_object(
    'range', jsonb_build_object('start_date', p_start_date, 'end_date', p_end_date),
    'capacity_device_hours', v_capacity,
    'booked_device_hours', v_booked,
    'checked_in_device_hours', v_checked_in,
    'reservation_rate', case when v_capacity = 0 then null else round(v_booked / v_capacity, 4) end,
    'actual_check_in_usage_rate', case when v_capacity = 0 then null else round(v_checked_in / v_capacity, 4) end,
    'no_show_rate', case when v_booked = 0 then null else round(v_no_show / v_booked, 4) end,
    'cancellation_rate', case when v_booked + v_cancelled = 0 then null else round(v_cancelled / (v_booked + v_cancelled), 4) end,
    'popular_hours', v_top_hours,
    'data_completeness', jsonb_build_object('active_or_completed_missing_check_in', v_missing_usage)
  );
end;
$$;

create or replace function public.v2_log_agent_interaction(
  p_request_id uuid, p_agent_type text, p_request_payload jsonb, p_response_payload jsonb, p_outcome text, p_error_message text default null
)
returns void language plpgsql security definer set search_path = public
as $$
begin
  if p_agent_type not in ('booking', 'admin') or p_outcome not in ('suggested', 'confirmed', 'fallback', 'failed') then
    raise exception 'Invalid agent audit values';
  end if;
  if p_agent_type = 'admin' and not public.is_admin() then raise exception 'Administrator access is required'; end if;
  insert into public.agent_audit_logs (request_id, user_id, agent_type, request_payload, response_payload, outcome, error_message)
  values (p_request_id, auth.uid(), p_agent_type, coalesce(p_request_payload, '{}'::jsonb), coalesce(p_response_payload, '{}'::jsonb), p_outcome, p_error_message);
end;
$$;

revoke all on function public.v2_assert_booking_request(date, smallint, smallint[]) from public;
revoke all on function public.v2_create_reservations(date, smallint, smallint[], uuid) from public;
revoke all on function public.v2_cancel_reservation(uuid) from public;
revoke all on function public.v2_reschedule_reservation(uuid, date, smallint, smallint, uuid) from public;
revoke all on function public.v2_check_in(uuid) from public;
revoke all on function public.v2_check_out(uuid) from public;
revoke all on function public.v2_refresh_expired_reservations() from public;
revoke all on function public.v2_schedule_candidates(date, smallint, smallint, smallint[], smallint) from public;
revoke all on function public.v2_log_schedule_candidates(date, smallint, smallint, jsonb) from public;
revoke all on function public.v2_admin_metrics(date, date) from public;
revoke all on function public.v2_log_agent_interaction(uuid, text, jsonb, jsonb, text, text) from public;
grant select on public.lab_opening_rules, public.maintenance_windows, public.reservation_usage_records,
  public.reservation_status_events, public.agent_audit_logs, public.scheduling_plan_logs to authenticated;
grant execute on function public.v2_create_reservations(date, smallint, smallint[], uuid) to authenticated;
grant execute on function public.v2_cancel_reservation(uuid) to authenticated;
grant execute on function public.v2_reschedule_reservation(uuid, date, smallint, smallint, uuid) to authenticated;
grant execute on function public.v2_check_in(uuid) to authenticated;
grant execute on function public.v2_check_out(uuid) to authenticated;
grant execute on function public.v2_refresh_expired_reservations() to authenticated;
grant execute on function public.v2_schedule_candidates(date, smallint, smallint, smallint[], smallint) to authenticated;
grant execute on function public.v2_log_schedule_candidates(date, smallint, smallint, jsonb) to authenticated;
grant execute on function public.v2_admin_metrics(date, date) to authenticated;
grant execute on function public.v2_log_agent_interaction(uuid, text, jsonb, jsonb, text, text) to authenticated;

do $$
begin
  if not exists (
    select 1 from pg_publication_rel pr join pg_publication p on p.oid = pr.prpubid
    where p.pubname = 'supabase_realtime' and pr.prrelid = 'public.reservation_usage_records'::regclass
  ) then alter publication supabase_realtime add table public.reservation_usage_records; end if;
  if not exists (
    select 1 from pg_publication_rel pr join pg_publication p on p.oid = pr.prpubid
    where p.pubname = 'supabase_realtime' and pr.prrelid = 'public.maintenance_windows'::regclass
  ) then alter publication supabase_realtime add table public.maintenance_windows; end if;
end;
$$;

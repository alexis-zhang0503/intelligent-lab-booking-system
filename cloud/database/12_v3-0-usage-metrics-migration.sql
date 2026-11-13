-- V3.0 additive, non-destructive usage-metrics migration.
-- Prerequisite: schema.sql, cloud-migration.sql, v2-0-migration.sql and v2-2-attendance-fix.sql.
-- This migration does not modify or delete historical reservations or attendance rows.
-- All slot instants are calculated in Asia/Shanghai; do not rely on the session TimeZone.

begin;

do $$
declare
  required_table text;
begin
  foreach required_table in array array[
    'public.reservations', 'public.reservation_usage_records',
    'public.lab_opening_rules', 'public.maintenance_windows', 'public.devices'
  ] loop
    if to_regclass(required_table) is null then
      raise exception 'V3 usage metrics precheck failed: required table % is missing', required_table;
    end if;
  end loop;
  if to_regprocedure('public.v2_check_in(uuid)') is null
     or to_regprocedure('public.v2_check_out(uuid)') is null then
    raise exception 'V3 usage metrics precheck failed: apply v2-2-attendance-fix.sql first';
  end if;
end;
$$;

-- Existing devices have no historical status timeline.  This baseline begins at
-- the next complete local day, so V3 never uses today's current status to invent
-- historical capacity.  The trigger below records all later status changes.
create table if not exists public.device_status_history (
  id uuid primary key default gen_random_uuid(),
  device_number smallint not null references public.devices(number) on delete cascade,
  status text not null check (status in ('available', 'faulty')),
  effective_at timestamptz not null,
  source text not null check (source in ('v3_baseline', 'device_status_change')),
  recorded_at timestamptz not null default now()
);

create index if not exists device_status_history_lookup
  on public.device_status_history (device_number, effective_at desc);

alter table public.device_status_history enable row level security;
drop policy if exists "administrators read device status history" on public.device_status_history;
create policy "administrators read device status history" on public.device_status_history
  for select to authenticated using (public.is_admin());

insert into public.device_status_history (device_number, status, effective_at, source)
select d.number,
       d.status,
       ((date_trunc('day', now() at time zone 'Asia/Shanghai') + interval '1 day') at time zone 'Asia/Shanghai'),
       'v3_baseline'
from public.devices d
where not exists (
  select 1 from public.device_status_history h where h.device_number = d.number
);

create or replace function public.v3_record_device_status_change()
returns trigger language plpgsql security definer set search_path = public
as $$
begin
  if new.status is distinct from old.status then
    -- Before the first complete-day baseline takes effect, keep that baseline
    -- aligned with the state that will exist at midnight instead of creating a
    -- misleading earlier event that the future baseline would override.
    update public.device_status_history
       set status = new.status, recorded_at = now()
     where device_number = new.number
       and source = 'v3_baseline'
       and effective_at > now();
    if not found then
      insert into public.device_status_history (device_number, status, effective_at, source)
      values (new.number, new.status, now(), 'device_status_change');
    end if;
  end if;
  return new;
end;
$$;

drop trigger if exists v3_record_device_status_history on public.devices;
create trigger v3_record_device_status_history
  after update of status on public.devices
  for each row execute function public.v3_record_device_status_change();

create or replace function public.v3_slot_start(p_date date, p_start_hour smallint)
returns timestamptz language sql stable set search_path = public
as $$
  select ((p_date::timestamp + make_interval(hours => p_start_hour)) at time zone 'Asia/Shanghai')
$$;

create or replace function public.v3_admin_usage_metrics(
  p_start_date date,
  p_end_date date,
  p_device_number smallint default null
)
returns jsonb language plpgsql security definer set search_path = public
as $$
declare
  v_capacity_complete boolean;
  v_known_capacity_minutes numeric;
  v_unknown_capacity_slots integer;
  v_booked_minutes numeric;
  v_actual_used_minutes numeric;
  v_provisional_used_minutes numeric;
  v_no_show_minutes numeric;
  v_cancelled_minutes numeric;
  v_expected_usage_records integer;
  v_complete_usage_records integer;
  v_checked_in_missing_checkout integer;
  v_missing_valid_check_in integer;
  v_timestamp_anomalies integer;
  v_device_utilization jsonb;
  v_slot_trends jsonb;
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if p_start_date is null or p_end_date is null or p_end_date < p_start_date then
    raise exception 'A valid date range is required';
  end if;
  if p_device_number is not null and not exists (select 1 from public.devices where number = p_device_number) then
    raise exception 'The requested device does not exist';
  end if;

  with open_slots as (
    select distinct day::date as reservation_date, slot_hour::smallint as start_hour
    from generate_series(p_start_date, p_end_date, interval '1 day') as days(day)
    join public.lab_opening_rules rule
      on rule.active and rule.weekday = extract(dow from day)::smallint
    cross join lateral generate_series(rule.start_hour, rule.end_hour - 1) as slot_hour
  ), capacity_slots as (
    select slots.reservation_date,
           slots.start_hour,
           device.number as device_number,
           history.status as historical_status,
           exists (
             select 1 from public.maintenance_windows maintenance
             where maintenance.reservation_date = slots.reservation_date
               and maintenance.status in ('scheduled', 'active')
               and maintenance.start_hour < slots.start_hour + 1
               and maintenance.end_hour > slots.start_hour
               and (maintenance.device_number is null or maintenance.device_number = device.number)
           ) as under_maintenance
    from open_slots slots
    cross join public.devices device
    left join lateral (
      select h.status
      from public.device_status_history h
      where h.device_number = device.number
        and h.effective_at <= public.v3_slot_start(slots.reservation_date, slots.start_hour)
      order by h.effective_at desc, h.recorded_at desc
      limit 1
    ) history on true
    where p_device_number is null or device.number = p_device_number
  ), capacity_summary as (
    select coalesce(sum(case when historical_status = 'available' and not under_maintenance then 60 else 0 end), 0)::numeric as known_capacity_minutes,
           count(*) filter (where historical_status is null)::integer as unknown_capacity_slots
    from capacity_slots
  )
  select known_capacity_minutes, unknown_capacity_slots
    into v_known_capacity_minutes, v_unknown_capacity_slots
  from capacity_summary;

  with reservation_minutes as (
    select r.id,
           r.device_number,
           r.reservation_date,
           r.start_hour,
           r.status,
           public.v3_slot_start(r.reservation_date, r.start_hour) as slot_start,
           public.v3_slot_start(r.reservation_date, (r.start_hour + 1)::smallint) as slot_end,
           u.checked_in_at,
           u.checked_out_at
    from public.reservations r
    left join public.reservation_usage_records u on u.reservation_id = r.id
    where r.reservation_date between p_start_date and p_end_date
      and (p_device_number is null or r.device_number = p_device_number)
  ), classified as (
    select *,
      case when checked_in_at is not null and checked_out_at is not null
                  and checked_out_at > checked_in_at
                  and checked_in_at < slot_end and checked_out_at > slot_start
           then greatest(0::numeric, extract(epoch from least(checked_out_at, slot_end) - greatest(checked_in_at, slot_start)) / 60.0)
           else 0::numeric end as actual_used_minutes,
      case when checked_in_at is not null and checked_out_at is null and now() > slot_start
                  and checked_in_at < slot_end
           then greatest(0::numeric, extract(epoch from least(now(), slot_end) - greatest(checked_in_at, slot_start)) / 60.0)
           else 0::numeric end as provisional_used_minutes,
      case when checked_out_at is not null and checked_in_at is null then true
           when checked_in_at is not null and checked_out_at is not null and checked_out_at <= checked_in_at then true
           when checked_in_at is not null and checked_in_at >= slot_end then true
           when checked_out_at is not null and checked_out_at <= slot_start then true
           else false end as timestamp_anomaly
    from reservation_minutes
  )
  select coalesce(sum(case when status in ('reserved', 'confirmed', 'in_use', 'completed', 'no_show') then 60 else 0 end), 0),
         coalesce(sum(actual_used_minutes), 0),
         coalesce(sum(provisional_used_minutes), 0),
         coalesce(sum(case when status = 'no_show' then 60 else 0 end), 0),
         coalesce(sum(case when status = 'cancelled' then 60 else 0 end), 0),
         count(*) filter (where status in ('in_use', 'completed')),
         count(*) filter (where status in ('in_use', 'completed') and actual_used_minutes > 0 and not timestamp_anomaly),
         count(*) filter (where checked_in_at is not null and checked_out_at is null),
         count(*) filter (where status in ('in_use', 'completed') and (checked_in_at is null or checked_in_at >= slot_end)),
         count(*) filter (where timestamp_anomaly)
    into v_booked_minutes, v_actual_used_minutes, v_provisional_used_minutes, v_no_show_minutes, v_cancelled_minutes,
         v_expected_usage_records, v_complete_usage_records, v_checked_in_missing_checkout,
         v_missing_valid_check_in, v_timestamp_anomalies
  from classified;

  v_capacity_complete := coalesce(v_unknown_capacity_slots, 0) = 0;

  with open_slots as (
    select distinct day::date as reservation_date, slot_hour::smallint as start_hour
    from generate_series(p_start_date, p_end_date, interval '1 day') as days(day)
    join public.lab_opening_rules rule on rule.active and rule.weekday = extract(dow from day)::smallint
    cross join lateral generate_series(rule.start_hour, rule.end_hour - 1) as slot_hour
  ), capacity_slots as (
    select slots.reservation_date, slots.start_hour, device.number as device_number, history.status,
      exists (select 1 from public.maintenance_windows maintenance
        where maintenance.reservation_date = slots.reservation_date and maintenance.status in ('scheduled', 'active')
          and maintenance.start_hour < slots.start_hour + 1 and maintenance.end_hour > slots.start_hour
          and (maintenance.device_number is null or maintenance.device_number = device.number)) as under_maintenance
    from open_slots slots cross join public.devices device
    left join lateral (select h.status from public.device_status_history h
      where h.device_number = device.number and h.effective_at <= public.v3_slot_start(slots.reservation_date, slots.start_hour)
      order by h.effective_at desc, h.recorded_at desc limit 1) history on true
    where p_device_number is null or device.number = p_device_number
  ), usage_by_device as (
    select r.device_number,
      coalesce(sum(case when u.checked_in_at is not null and u.checked_out_at is not null
                            and u.checked_out_at > u.checked_in_at
                            and u.checked_in_at < public.v3_slot_start(r.reservation_date, (r.start_hour + 1)::smallint)
                            and u.checked_out_at > public.v3_slot_start(r.reservation_date, r.start_hour)
                         then greatest(0::numeric, extract(epoch from least(u.checked_out_at, public.v3_slot_start(r.reservation_date, (r.start_hour + 1)::smallint)) - greatest(u.checked_in_at, public.v3_slot_start(r.reservation_date, r.start_hour))) / 60.0)
                         else 0 end), 0) as actual_used_minutes
    from public.reservations r left join public.reservation_usage_records u on u.reservation_id = r.id
    where r.reservation_date between p_start_date and p_end_date
      and (p_device_number is null or r.device_number = p_device_number)
    group by r.device_number
  ), capacity_by_device as (
    select device_number,
      coalesce(sum(case when status = 'available' and not under_maintenance then 60 else 0 end), 0)::numeric as known_capacity_minutes,
      count(*) filter (where status is null)::integer as unknown_capacity_slots
    from capacity_slots group by device_number
  )
  select coalesce(jsonb_agg(jsonb_build_object(
           'device_number', c.device_number,
           'known_capacity_minutes', c.known_capacity_minutes,
           'capacity_data_complete', c.unknown_capacity_slots = 0,
           'actual_used_minutes', coalesce(u.actual_used_minutes, 0),
           'actual_usage_rate', case when c.unknown_capacity_slots = 0 and c.known_capacity_minutes > 0
                                     then round(coalesce(u.actual_used_minutes, 0) / c.known_capacity_minutes, 4) else null end
         ) order by case when c.unknown_capacity_slots = 0 and c.known_capacity_minutes > 0
                              then coalesce(u.actual_used_minutes, 0) / c.known_capacity_minutes else null end desc nulls last,
                    c.device_number), '[]'::jsonb)
    into v_device_utilization
  from capacity_by_device c left join usage_by_device u using (device_number);

  with open_slots as (
    select distinct day::date as reservation_date, slot_hour::smallint as start_hour
    from generate_series(p_start_date, p_end_date, interval '1 day') as days(day)
    join public.lab_opening_rules rule on rule.active and rule.weekday = extract(dow from day)::smallint
    cross join lateral generate_series(rule.start_hour, rule.end_hour - 1) as slot_hour
  ), capacity_by_hour as (
    select slots.start_hour,
      coalesce(sum(case when history.status = 'available' and not exists (select 1 from public.maintenance_windows maintenance
        where maintenance.reservation_date = slots.reservation_date and maintenance.status in ('scheduled', 'active')
          and maintenance.start_hour < slots.start_hour + 1 and maintenance.end_hour > slots.start_hour
          and (maintenance.device_number is null or maintenance.device_number = device.number)) then 60 else 0 end), 0)::numeric as known_capacity_minutes,
      count(*) filter (where history.status is null)::integer as unknown_capacity_slots
    from open_slots slots cross join public.devices device
    left join lateral (select h.status from public.device_status_history h
      where h.device_number = device.number and h.effective_at <= public.v3_slot_start(slots.reservation_date, slots.start_hour)
      order by h.effective_at desc, h.recorded_at desc limit 1) history on true
    where p_device_number is null or device.number = p_device_number
    group by slots.start_hour
  ), usage_by_hour as (
    select r.start_hour, coalesce(sum(case when u.checked_in_at is not null and u.checked_out_at is not null
      and u.checked_out_at > u.checked_in_at
      and u.checked_in_at < public.v3_slot_start(r.reservation_date, (r.start_hour + 1)::smallint)
      and u.checked_out_at > public.v3_slot_start(r.reservation_date, r.start_hour)
      then greatest(0::numeric, extract(epoch from least(u.checked_out_at, public.v3_slot_start(r.reservation_date, (r.start_hour + 1)::smallint)) - greatest(u.checked_in_at, public.v3_slot_start(r.reservation_date, r.start_hour))) / 60.0) else 0 end), 0) as actual_used_minutes
    from public.reservations r left join public.reservation_usage_records u on u.reservation_id = r.id
    where r.reservation_date between p_start_date and p_end_date
      and (p_device_number is null or r.device_number = p_device_number)
    group by r.start_hour
  )
  select coalesce(jsonb_agg(jsonb_build_object(
           'start_hour', c.start_hour,
           'known_capacity_minutes', c.known_capacity_minutes,
           'capacity_data_complete', c.unknown_capacity_slots = 0,
           'actual_used_minutes', coalesce(u.actual_used_minutes, 0),
           'actual_usage_rate', case when c.unknown_capacity_slots = 0 and c.known_capacity_minutes > 0
                                     then round(coalesce(u.actual_used_minutes, 0) / c.known_capacity_minutes, 4) else null end
         ) order by c.start_hour), '[]'::jsonb)
    into v_slot_trends
  from capacity_by_hour c left join usage_by_hour u using (start_hour);

  return jsonb_build_object(
    'metric_version', 'v3_device_minutes',
    'time_zone', 'Asia/Shanghai',
    'range', jsonb_build_object('start_date', p_start_date, 'end_date', p_end_date, 'device_number', p_device_number),
    'capacity_data_complete', v_capacity_complete,
    'capacity_data_limitations', case when v_capacity_complete then 'none'
      else 'Historical device-status data predates the V3 baseline; rates are withheld rather than inferred from current device status.' end,
    'known_capacity_device_minutes', round(v_known_capacity_minutes, 2),
    'capacity_device_minutes', case when v_capacity_complete then round(v_known_capacity_minutes, 2) else null end,
    'booked_device_minutes', round(v_booked_minutes, 2),
    'actual_used_minutes', round(v_actual_used_minutes, 2),
    'provisional_used_minutes', round(v_provisional_used_minutes, 2),
    'reservation_rate', case when v_capacity_complete and v_known_capacity_minutes > 0 then round(v_booked_minutes / v_known_capacity_minutes, 4) else null end,
    'actual_usage_rate', case when v_capacity_complete and v_known_capacity_minutes > 0 then round(v_actual_used_minutes / v_known_capacity_minutes, 4) else null end,
    'usage_gap_rate', case when v_capacity_complete and v_known_capacity_minutes > 0 then round((v_booked_minutes - v_actual_used_minutes) / v_known_capacity_minutes, 4) else null end,
    'no_show_rate', case when v_booked_minutes > 0 then round(v_no_show_minutes / v_booked_minutes, 4) else null end,
    'cancellation_rate', case when v_booked_minutes + v_cancelled_minutes > 0 then round(v_cancelled_minutes / (v_booked_minutes + v_cancelled_minutes), 4) else null end,
    'data_completeness', jsonb_build_object(
      'checked_in_missing_checkout_count', v_checked_in_missing_checkout,
      'missing_valid_check_in_timestamp_count', v_missing_valid_check_in,
      'timestamp_anomaly_count', v_timestamp_anomalies,
      'complete_usage_record_count', v_complete_usage_records,
      'usage_expected_reservation_count', v_expected_usage_records,
      'complete_usage_record_coverage', case when v_expected_usage_records > 0 then round(v_complete_usage_records::numeric / v_expected_usage_records, 4) else null end,
      'final_actual_usage_note', 'Only complete check-in/check-out pairs contribute to actual_used_minutes; provisional_used_minutes is excluded.'
    ),
    'device_utilization', v_device_utilization,
    'slot_usage_trends', v_slot_trends,
    'measurement_note', 'Manual attendance is a system-recorded usage signal, not physical-device verification.'
  );
end;
$$;

revoke all on table public.device_status_history from public;
revoke all on function public.v3_slot_start(date, smallint) from public;
revoke all on function public.v3_admin_usage_metrics(date, date, smallint) from public;
grant select on public.device_status_history to authenticated;
grant execute on function public.v3_admin_usage_metrics(date, date, smallint) to authenticated;

notify pgrst, 'reload schema';
commit;

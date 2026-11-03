-- Run AFTER schema.sql, cloud-migration.sql and v2-0-migration.sql.
-- Fix attendance wall-clock conversion without changing historical migrations.
-- Safe to rerun; no reservation or usage rows are deleted.
begin;
create or replace function public.v2_check_in(p_reservation_id uuid)
returns void language plpgsql security definer set search_path = public
as $$
declare v_reservation public.reservations%rowtype;
begin
  if auth.uid() is null then raise exception 'Authentication required' using errcode = '42501'; end if;
  select * into v_reservation from public.reservations where id = p_reservation_id for update;
  if not found or (v_reservation.user_id <> auth.uid() and not public.is_admin()) then raise exception 'The reservation was not found'; end if;
  if v_reservation.status = 'in_use' then return; end if;
  if v_reservation.status not in ('reserved', 'confirmed', 'in_use') then raise exception 'Only a confirmed reservation can be checked in'; end if;
  if now() < ((v_reservation.reservation_date + make_interval(hours => v_reservation.start_hour)) at time zone 'Asia/Shanghai')
     or now() >= ((v_reservation.reservation_date + make_interval(hours => v_reservation.start_hour + 1)) at time zone 'Asia/Shanghai') then
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
  if auth.uid() is null then raise exception 'Authentication required' using errcode = '42501'; end if;
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

revoke all on function public.v2_check_in(uuid) from public;
revoke all on function public.v2_check_out(uuid) from public;
grant execute on function public.v2_check_in(uuid) to authenticated;
grant execute on function public.v2_check_out(uuid) to authenticated;
notify pgrst, 'reload schema';
commit;

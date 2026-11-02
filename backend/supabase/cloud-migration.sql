-- Apply after schema.sql for the GitHub Pages cloud client.
-- Safe to run more than once in the Supabase SQL Editor.

alter table public.profiles
  add column if not exists active boolean not null default true;

-- Table privileges expose only the operations that RLS policies subsequently filter.
grant usage on schema public to authenticated;
grant select on public.profiles, public.devices, public.reservations, public.issue_reports to authenticated;
grant insert, update on public.issue_reports to authenticated;
grant update on public.devices, public.reservations to authenticated;

-- A disabled administrator cannot retain any privileged policies.
create or replace function public.is_admin()
returns boolean language sql stable security definer set search_path = public
as $$
  select exists (
    select 1
    from public.profiles
    where id = auth.uid() and role = 'admin' and active
  )
$$;

-- A student may release only their own unstarted reservation.
drop policy if exists "students cancel their own reservations" on public.reservations;
create policy "students cancel their own reservations"
  on public.reservations for update to authenticated
  using (user_id = auth.uid() and status = 'reserved')
  with check (user_id = auth.uid() and status = 'cancelled');

-- Administrators deactivate student access without exposing Auth admin APIs to a browser.
create or replace function public.deactivate_student(target_user uuid)
returns void language plpgsql security definer set search_path = public
as $$
begin
  if not public.is_admin() then
    raise exception 'Administrator access is required';
  end if;

  if not exists (
    select 1 from public.profiles
    where id = target_user and role = 'student' and active
  ) then
    raise exception 'The selected active student was not found';
  end if;

  update public.profiles set active = false where id = target_user;
  update public.reservations
    set status = 'cancelled', cancelled_at = now()
    where user_id = target_user and status = 'reserved';
end;
$$;

grant execute on function public.deactivate_student(uuid) to authenticated;

-- A disabled student cannot create new reservations even with an old browser session.
create or replace function public.reserve_hourly_devices(
  requested_date date,
  requested_hour smallint,
  requested_devices smallint[]
)
returns table (reservation_id uuid, device_number smallint)
language plpgsql security definer set search_path = public
as $$
declare
  requested_device smallint;
  created_id uuid;
  created_device smallint;
begin
  if not exists (
    select 1 from public.profiles
    where id = auth.uid() and active
  ) then
    raise exception 'This account is inactive';
  end if;
  if requested_date < current_date then
    raise exception 'Reservation date must not be in the past';
  end if;
  if requested_date = current_date and requested_hour <= extract(hour from current_time) then
    raise exception 'Reservation hour has already started';
  end if;
  if requested_hour not between 8 and 16 then
    raise exception 'Reservations use the hourly range 08:00-17:00';
  end if;
  if coalesce(array_length(requested_devices, 1), 0) = 0 then
    raise exception 'Select at least one device';
  end if;

  foreach requested_device in array requested_devices loop
    if exists (
      select 1 from public.devices
      where number = requested_device and status = 'faulty'
    ) then
      raise exception 'Device % is faulty', requested_device;
    end if;
    insert into public.reservations (device_number, user_id, reservation_date, start_hour)
    values (requested_device, auth.uid(), requested_date, requested_hour)
    returning reservations.id, reservations.device_number into created_id, created_device;
    reservation_id := created_id;
    device_number := created_device;
    return next;
  end loop;
exception when unique_violation then
  raise exception 'A selected device was just reserved by another user';
end;
$$;

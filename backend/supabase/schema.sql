-- Shared data contract for the web and Qt clients.
-- Run this migration in a new Supabase project's SQL editor.

create extension if not exists pgcrypto;

-- Profiles are separate from auth.users so application roles remain auditable.
create table if not exists public.profiles (
  id uuid primary key references auth.users(id) on delete cascade,
  login_id text not null unique check (char_length(login_id) between 2 and 20),
  display_name text not null check (char_length(trim(display_name)) between 1 and 16),
  role text not null default 'student' check (role in ('student', 'admin')),
  created_at timestamptz not null default now()
);

-- Forty physical computers form the only bookable inventory.
create table if not exists public.devices (
  number smallint primary key check (number between 1 and 40),
  status text not null default 'available' check (status in ('available', 'faulty')),
  updated_at timestamptz not null default now(),
  updated_by uuid references public.profiles(id)
);

insert into public.devices (number)
select generate_series(1, 40)
on conflict (number) do nothing;

-- Each row is exactly one assignment-defined one-hour reservation.
create table if not exists public.reservations (
  id uuid primary key default gen_random_uuid(),
  device_number smallint not null references public.devices(number),
  user_id uuid not null references public.profiles(id) on delete cascade,
  reservation_date date not null,
  start_hour smallint not null check (start_hour between 8 and 16),
  status text not null default 'reserved' check (status in ('reserved', 'completed', 'cancelled')),
  created_at timestamptz not null default now(),
  cancelled_at timestamptz
);

-- This partial unique index is the authoritative concurrent-booking guard.
create unique index if not exists active_reservation_per_device_hour
  on public.reservations (device_number, reservation_date, start_hour)
  where status in ('reserved', 'completed');

create table if not exists public.issue_reports (
  id uuid primary key default gen_random_uuid(),
  device_number smallint not null references public.devices(number),
  reporter_id uuid not null references public.profiles(id),
  description text not null check (char_length(trim(description)) between 1 and 120),
  status text not null default 'reported' check (status in ('reported', 'faulty', 'resolved')),
  created_at timestamptz not null default now(),
  resolved_at timestamptz,
  resolved_by uuid references public.profiles(id),
  resolution_note text check (resolution_note is null or char_length(trim(resolution_note)) between 1 and 120)
);

-- Auth metadata creates a student profile automatically. Client code never chooses a role.
create or replace function public.handle_new_student()
returns trigger language plpgsql security definer set search_path = public
as $$
begin
  insert into public.profiles (id, login_id, display_name, role)
  values (
    new.id,
    coalesce(new.raw_user_meta_data ->> 'login_id', split_part(new.email, '@', 1)),
    coalesce(new.raw_user_meta_data ->> 'display_name', split_part(new.email, '@', 1)),
    'student'
  );
  return new;
end;
$$;

drop trigger if exists create_student_profile on auth.users;
create trigger create_student_profile
  after insert on auth.users
  for each row execute procedure public.handle_new_student();

alter table public.profiles enable row level security;
alter table public.devices enable row level security;
alter table public.reservations enable row level security;
alter table public.issue_reports enable row level security;

-- All signed-in users can inspect availability, but only administrators see identities.
create policy "authenticated users read devices"
  on public.devices for select to authenticated using (true);
create policy "authenticated users read reservation availability"
  on public.reservations for select to authenticated using (true);
create policy "users read their own profile"
  on public.profiles for select to authenticated using (id = auth.uid());
create policy "users read their own reports"
  on public.issue_reports for select to authenticated using (reporter_id = auth.uid());
create policy "students create their own reports"
  on public.issue_reports for insert to authenticated with check (reporter_id = auth.uid());

-- Admin-only policies use the persisted role, never a client-provided value.
create function public.is_admin()
returns boolean language sql stable security definer set search_path = public
as $$ select exists (select 1 from public.profiles where id = auth.uid() and role = 'admin') $$;

create policy "administrators read profiles"
  on public.profiles for select to authenticated using (public.is_admin());
create policy "administrators manage devices"
  on public.devices for all to authenticated using (public.is_admin()) with check (public.is_admin());
create policy "administrators manage reports"
  on public.issue_reports for all to authenticated using (public.is_admin()) with check (public.is_admin());
create policy "administrators manage reservations"
  on public.reservations for all to authenticated using (public.is_admin()) with check (public.is_admin());

-- The client may create student profiles only. Admin profiles are seeded or invited server-side.
-- An atomic all-or-nothing booking operation. A uniqueness failure means another client won the race.
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
    if exists (select 1 from public.devices where number = requested_device and status = 'faulty') then
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

grant execute on function public.reserve_hourly_devices(date, smallint, smallint[]) to authenticated;

-- Realtime updates allow both clients to refresh seats after a successful change.
alter publication supabase_realtime add table public.devices;
alter publication supabase_realtime add table public.reservations;
alter publication supabase_realtime add table public.issue_reports;

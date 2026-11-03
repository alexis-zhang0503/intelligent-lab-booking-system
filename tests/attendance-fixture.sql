-- Test-only schema for an EMPTY disposable PostgreSQL database; not a production migration.
create role authenticated;
create schema auth;
create function auth.uid() returns uuid language sql stable as $$
  select nullif(current_setting('request.jwt.claim.sub', true), '')::uuid
$$;
create function public.is_admin() returns boolean language sql stable as $$ select false $$;
create table public.reservations (
  id uuid primary key, user_id uuid not null, reservation_date date not null,
  start_hour integer not null, status text not null
);
create table public.reservation_usage_records (
  reservation_id uuid primary key references public.reservations(id),
  checked_in_at timestamptz, checked_in_by uuid,
  checked_out_at timestamptz, checked_out_by uuid, updated_at timestamptz default now()
);
create table public.reservation_status_events (
  reservation_id uuid, from_status text, to_status text, actor_id uuid, reason text
);

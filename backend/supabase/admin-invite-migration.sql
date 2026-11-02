-- Apply after schema.sql and cloud-migration.sql.
-- Creates one-time administrator invitations without exposing a secret in clients.

alter table public.profiles drop constraint if exists profiles_login_id_check;
alter table public.profiles add constraint profiles_login_id_check
  check (char_length(login_id) between 2 and 80);

create table if not exists public.admin_invites (
  id uuid primary key default gen_random_uuid(),
  code_hash text not null,
  expires_at timestamptz not null,
  used_at timestamptz,
  created_by uuid not null references public.profiles(id),
  used_by uuid references public.profiles(id),
  created_at timestamptz not null default now()
);

alter table public.admin_invites enable row level security;

-- Only an active administrator can generate a code. Its clear-text value is returned once.
create or replace function public.create_admin_invite(invite_expires_at timestamptz)
returns table (invite_code text, expires_at timestamptz)
language plpgsql security definer set search_path = public
as $$
declare
  generated_code text;
begin
  if not public.is_admin() then
    raise exception 'Administrator access is required';
  end if;
  if invite_expires_at <= now() or invite_expires_at > now() + interval '30 days' then
    raise exception 'Invitation expiry must be within the next 30 days';
  end if;

  generated_code := 'NJUPT-' || upper(encode(gen_random_bytes(6), 'hex'));
  insert into public.admin_invites (code_hash, expires_at, created_by)
    values (crypt(generated_code, gen_salt('bf')), invite_expires_at, auth.uid());
  invite_code := generated_code;
  expires_at := invite_expires_at;
  return next;
end;
$$;

grant execute on function public.create_admin_invite(timestamptz) to authenticated;

-- The Auth trigger creates every profile as a student unless it atomically consumes
-- a valid, unused invitation. The role is never supplied by browser code.
create or replace function public.handle_new_student()
returns trigger language plpgsql security definer set search_path = public
as $$
declare
  requested_code text := nullif(trim(new.raw_user_meta_data ->> 'admin_invite_code'), '');
  matching_invite uuid;
  assigned_role text := 'student';
begin
  if requested_code is not null then
    select id into matching_invite
      from public.admin_invites
      where used_at is null
        and expires_at > now()
        and code_hash = crypt(requested_code, code_hash)
      for update;
    if matching_invite is null then
      raise exception 'Administrator invitation is invalid or expired';
    end if;
    update public.admin_invites
      set used_at = now()
      where id = matching_invite;
    assigned_role := 'admin';
  end if;

  insert into public.profiles (id, login_id, display_name, role)
  values (
    new.id,
    coalesce(new.raw_user_meta_data ->> 'login_id', split_part(new.email, '@', 1)),
    coalesce(new.raw_user_meta_data ->> 'display_name', split_part(new.email, '@', 1)),
    assigned_role
  );
  if matching_invite is not null then
    update public.admin_invites set used_by = new.id where id = matching_invite;
  end if;
  return new;
end;
$$;

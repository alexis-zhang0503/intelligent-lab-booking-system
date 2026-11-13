-- Fix production functions whose search_path excludes the pgcrypto extension schema.
-- Does not create accounts, issue invitations or modify existing invitations.
begin;
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
  if invite_expires_at is null or not isfinite(invite_expires_at) or invite_expires_at <= now() or invite_expires_at > now() + interval '30 days' then
    raise exception 'Invitation expiry must be within the next 30 days';
  end if;

  -- gen_random_uuid is already used by this project schema and avoids relying
  -- on the optional byte-generator function namespace.
  generated_code := 'NJUPT-' || upper(substring(replace(gen_random_uuid()::text, '-', '') from 1 for 12));
  insert into public.admin_invites (code_hash, expires_at, created_by)
    values (extensions.crypt(generated_code, extensions.gen_salt('bf')), invite_expires_at, auth.uid());
  invite_code := generated_code;
  expires_at := invite_expires_at;
  return next;
end;
$$;

revoke all on function public.create_admin_invite(timestamptz) from public;
grant execute on function public.create_admin_invite(timestamptz) to authenticated;

create or replace function public.create_admin_invite_for_days(p_valid_days integer)
returns table(invite_code text, expires_at timestamptz)
language plpgsql security invoker set search_path = public
as $$
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if p_valid_days is null or p_valid_days not in (1,7,30) then
    raise exception 'Invitation validity must be 1, 7 or 30 days';
  end if;
  return query select * from public.create_admin_invite(now()+make_interval(days=>p_valid_days));
end;
$$;
revoke all on function public.create_admin_invite_for_days(integer) from public;
grant execute on function public.create_admin_invite_for_days(integer) to authenticated;

create or replace function public.handle_new_student()
returns trigger language plpgsql security definer set search_path = public
as $$
declare
  requested_code text := nullif(upper(trim(new.raw_user_meta_data ->> 'admin_invite_code')), '');
  matching_invite uuid;
  assigned_role text := 'student';
begin
  if requested_code is not null then
    select id into matching_invite
      from public.admin_invites
      where used_at is null
        and expires_at > now()
        and code_hash = extensions.crypt(requested_code, code_hash)
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

commit;

-- Apply after cloud-migration.sql and admin-invite-migration.sql.
-- Lets administrators edit names and student/staff IDs without exposing Auth admin APIs.

alter table public.profiles add column if not exists auth_login_id text;

create or replace function public.update_profile_identity(
  target_user uuid,
  next_display_name text,
  next_login_id text
)
returns void language plpgsql security definer set search_path = public
as $$
declare
  cleaned_name text := trim(next_display_name);
  cleaned_login_id text := trim(next_login_id);
  target_role text;
begin
  if not public.is_admin() then
    raise exception 'Administrator access is required';
  end if;
  if char_length(cleaned_name) not between 1 and 16 then
    raise exception 'Name must contain 1 to 16 characters';
  end if;
  select role into target_role from public.profiles where id = target_user;
  if target_role is null then
    raise exception 'The selected user was not found';
  end if;
  if target_role = 'student' and cleaned_login_id !~ '^[A-Za-z0-9._-]{2,20}$' then
    raise exception 'Student ID must use 2 to 20 letters, numbers, dots, hyphens, or underscores';
  end if;
  if target_role = 'admin' and char_length(cleaned_login_id) not between 2 and 80 then
    raise exception 'Staff ID must contain 2 to 80 characters';
  end if;

  update public.profiles
    set display_name = cleaned_name,
        login_id = cleaned_login_id,
        auth_login_id = case
          when target_role = 'student' then coalesce(auth_login_id, login_id)
          else auth_login_id
        end
    where id = target_user;
end;
$$;

grant execute on function public.update_profile_identity(uuid, text, text) to authenticated;

-- Student IDs are public application identifiers, not Auth emails. This resolver
-- lets the browser sign in with the latest edited ID while keeping Auth private.
create or replace function public.resolve_student_auth_email(entered_login_id text)
returns text language plpgsql security definer set search_path = public
as $$
declare
  stored_auth_login_id text;
begin
  select coalesce(auth_login_id, login_id) into stored_auth_login_id
    from public.profiles
    where role = 'student'
      and active
      and login_id = trim(entered_login_id);
  if stored_auth_login_id is null then
    return null;
  end if;
  return lower(stored_auth_login_id) || '@lab.njupt.local';
end;
$$;

grant execute on function public.resolve_student_auth_email(text) to anon, authenticated;

-- Administrator account operations for a Supabase project maintainer.
-- First create the user in Authentication -> Users. The schema trigger creates
-- that user's profile with role = 'student'. Replace the example email below.

-- 1. Inspect the account and confirm that exactly one profile is returned.
select
  users.id,
  users.email,
  profiles.login_id,
  profiles.display_name,
  profiles.role
from auth.users as users
join public.profiles as profiles on profiles.id = users.id
where lower(users.email) = lower('administrator@example.edu');

-- 2. Promote the verified account. Run only after step 1 returns the intended user.
update public.profiles
set role = 'admin'
where id = (
  select id
  from auth.users
  where lower(email) = lower('administrator@example.edu')
);

-- 3. Re-run step 1 to verify role = 'admin'.

-- 4. To revoke administrator permission but retain the account, run this instead.
-- update public.profiles
-- set role = 'student'
-- where id = (
--   select id
--   from auth.users
--   where lower(email) = lower('administrator@example.edu')
-- );

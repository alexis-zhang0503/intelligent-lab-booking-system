# Administrator account runbook

## Purpose

Students create their own accounts in the application. Administrator accounts are opened only by a project maintainer in the Supabase Dashboard. There is no public administrator registration option and no invitation secret stored in either client.

## Open an administrator account

1. In Supabase, open **Authentication → Users → Add user**.
2. Create the intended administrator's email/password account. Keep the password with that administrator; do not place it in source code, GitHub secrets visible to a client, or the Web page.
3. Wait for the `create_student_profile` trigger from `01_schema.sql` to create the matching `public.profiles` row.
4. Open **SQL Editor** and run the verification query in `admin-account.sql`, replacing the placeholder email.
5. Run the promotion query in the same file. It changes that one existing profile from `student` to `admin`.
6. Run the verification query again. The returned role must be `admin` before the person signs in.

## Boundaries

- Do not add an “Administrator” choice to public registration.
- Do not put a service-role key, administrator password, or invitation code in the Web client or Qt executable.
- Only a Supabase project maintainer with Dashboard/SQL Editor access performs promotion and revocation.
- In local demonstration mode, the Web client has no administrator account. The Qt demo administrator is created only when its maintainer supplies `LAB_DEMO_ADMIN_PASSWORD` at launch.

## Revoke access

Run the revocation query in `admin-account.sql`, then delete or suspend the user in **Authentication → Users** if that person must no longer sign in at all. Changing the profile back to `student` removes administrator permissions immediately on the next authenticated request.

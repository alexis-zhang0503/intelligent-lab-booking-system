-- Web fault reporting: run AFTER schema.sql and cloud-migration.sql.
-- V2.1 audit events are recorded when that optional migration is present.
-- Reports and device isolation commit together. Existing bookings are preserved.
begin;

create or replace function public.isolate_reported_device()
returns trigger language plpgsql security definer set search_path = public
as $$
begin
  -- Never trust a client-supplied initial status or repair metadata.
  if not exists (select 1 from public.profiles where id = auth.uid() and active) then
    raise exception 'An active account is required';
  end if;
  if new.reporter_id <> auth.uid() then
    raise exception 'Reports must belong to the signed-in user';
  end if;
  new.status := 'faulty';
  new.resolved_at := null;
  new.resolved_by := null;
  new.resolution_note := null;
  update public.devices
     set status = 'faulty', updated_by = auth.uid(), updated_at = now()
   where number = new.device_number;
  if not found then raise exception 'Unknown device number'; end if;
  return new;
end;
$$;
revoke all on function public.isolate_reported_device() from public;

drop trigger if exists isolate_reported_device on public.issue_reports;
create trigger isolate_reported_device before insert on public.issue_reports
for each row execute function public.isolate_reported_device();

-- Preserve the existing V2.1 isolation metrics/audit trail for automatic isolation.
create or replace function public.record_automatic_fault_isolation()
returns trigger language plpgsql security definer set search_path = public
as $$
begin
  if to_regclass('public.fault_response_events') is not null then
    insert into public.fault_response_events (issue_id, action, actor_id, note, payload)
    values (new.id, 'isolated', new.reporter_id, 'Automatically isolated on fault report',
            jsonb_build_object('automatic', true));
  end if;
  return new;
end;
$$;
revoke all on function public.record_automatic_fault_isolation() from public;
drop trigger if exists record_automatic_fault_isolation on public.issue_reports;
create trigger record_automatic_fault_isolation after insert on public.issue_reports
for each row execute function public.record_automatic_fault_isolation();

create or replace function public.report_device_fault(p_device_number integer, p_description text)
returns jsonb language plpgsql security invoker set search_path = public
as $$
declare v_issue public.issue_reports%rowtype;
begin
  if auth.uid() is null then raise exception 'Sign in before reporting an issue'; end if;
  if char_length(trim(coalesce(p_description, ''))) not between 4 and 120 then
    raise exception 'The issue description must contain 4 to 120 characters';
  end if;
  insert into public.issue_reports (device_number, reporter_id, description)
  values (p_device_number, auth.uid(), trim(p_description)) returning * into v_issue;
  return to_jsonb(v_issue);
end;
$$;
revoke all on function public.report_device_fault(integer, text) from public;
grant execute on function public.report_device_fault(integer, text) to authenticated;

-- Old clients / manual restore cannot make a device available while another
-- report remains open. V2.1 repair first resolves its report, then restores it.
create or replace function public.keep_open_issue_device_unavailable()
returns trigger language plpgsql security definer set search_path = public
as $$
begin
  if new.status = 'available' and exists (
    select 1 from public.issue_reports
    where device_number = new.number and status in ('reported', 'faulty')
  ) then new.status := 'faulty'; end if;
  return new;
end;
$$;
revoke all on function public.keep_open_issue_device_unavailable() from public;
drop trigger if exists keep_open_issue_device_unavailable on public.devices;
create trigger keep_open_issue_device_unavailable before update of status on public.devices
for each row execute function public.keep_open_issue_device_unavailable();

-- Fix reports submitted before this upgrade, including devices whose reports
-- are visible only to their reporter under RLS.
update public.issue_reports set status = 'faulty' where status = 'reported';
update public.devices d set status = 'faulty', updated_at = now()
where exists (select 1 from public.issue_reports i
              where i.device_number = d.number and i.status = 'faulty');
commit;

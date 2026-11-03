-- After V2.1 and immediate-fault-report-migration.sql. No records are removed.
begin;
alter table public.issue_reports drop constraint if exists issue_reports_description_check;
alter table public.issue_reports add constraint issue_reports_description_check
  check (char_length(trim(description)) <= 120);
alter table public.issue_reports alter column description set default '';
alter table public.issue_reports add column if not exists deleted_at timestamptz;
alter table public.issue_reports add column if not exists deleted_by uuid references public.profiles(id);

create or replace function public.report_device_fault(p_device_number integer, p_description text)
returns jsonb language plpgsql security invoker set search_path = public
as $$
declare v_issue public.issue_reports%rowtype;
begin
  if auth.uid() is null then raise exception 'Sign in before reporting an issue'; end if;
  if char_length(trim(coalesce(p_description, ''))) > 120 then
    raise exception 'The issue description must not exceed 120 characters';
  end if;
  insert into public.issue_reports (device_number, reporter_id, description)
  values (p_device_number, auth.uid(), trim(coalesce(p_description, ''))) returning * into v_issue;
  return to_jsonb(v_issue);
end;
$$;
revoke all on function public.report_device_fault(integer, text) from public;
grant execute on function public.report_device_fault(integer, text) to authenticated;

create or replace function public.v2_complete_fault_repair(p_issue_id uuid, p_note text)
returns void language plpgsql security definer set search_path = public
as $$
declare v_issue public.issue_reports%rowtype; v_note text := nullif(trim(coalesce(p_note,'')), '');
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if char_length(v_note) > 120 then raise exception 'The repair note must not exceed 120 characters'; end if;
  select * into v_issue from public.issue_reports where id = p_issue_id for update;
  if not found or v_issue.deleted_at is not null then raise exception 'The issue was not found'; end if;
  if v_issue.status = 'resolved' then return; end if;
  update public.issue_reports set status='resolved',resolved_at=now(),resolved_by=auth.uid(),resolution_note=v_note where id=v_issue.id;
  update public.devices set status='available',updated_by=auth.uid(),updated_at=now() where number=v_issue.device_number;
  insert into public.fault_response_events(issue_id,action,actor_id,note) values(v_issue.id,'repaired',auth.uid(),coalesce(v_note,'Repair completed without a note'));
end;
$$;
revoke all on function public.v2_complete_fault_repair(uuid, text) from public;
grant execute on function public.v2_complete_fault_repair(uuid, text) to authenticated;

-- Archive rather than destroy repair history and audit metrics.
create or replace function public.guard_issue_record_removal()
returns trigger language plpgsql security definer set search_path = public
as $$
begin
  if tg_op='INSERT' then new.deleted_at:=null; new.deleted_by:=null; return new; end if;
  if tg_op='DELETE' or new.deleted_at is distinct from old.deleted_at or new.deleted_by is distinct from old.deleted_by then
    if not public.is_admin() then raise exception 'Administrator access is required'; end if;
    if old.status <> 'resolved' or old.resolved_at is null or old.resolved_at > now()-interval '24 hours' then
      raise exception 'Issue records can be removed only 24 hours after repair';
    end if;
    if tg_op='DELETE' then return old; end if;
    if old.deleted_at is not null then raise exception 'The issue record is already removed'; end if;
    new.deleted_at:=now(); new.deleted_by:=auth.uid();
  end if;
  return new;
end;
$$;
revoke all on function public.guard_issue_record_removal() from public;
drop trigger if exists guard_issue_record_removal on public.issue_reports;
create trigger guard_issue_record_removal before insert or update or delete on public.issue_reports
for each row execute function public.guard_issue_record_removal();

create or replace function public.archive_resolved_issue(p_issue_id uuid)
returns void language plpgsql security definer set search_path = public
as $$
declare v_issue public.issue_reports%rowtype;
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  select * into v_issue from public.issue_reports where id=p_issue_id for update;
  if not found then raise exception 'The issue was not found'; end if;
  if v_issue.deleted_at is not null then return; end if;
  update public.issue_reports set deleted_at=now(),deleted_by=auth.uid() where id=p_issue_id;
end;
$$;
revoke all on function public.archive_resolved_issue(uuid) from public;
grant execute on function public.archive_resolved_issue(uuid) to authenticated;
commit;

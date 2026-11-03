-- Run only against the empty test database initialized by attendance-fixture.sql.
begin;
set time zone 'UTC';
select set_config('request.jwt.claim.sub','00000000-0000-0000-0000-000000000001',true);
do $$
declare
  v_id uuid := '00000000-0000-0000-0000-000000000010';
  v_early uuid := '00000000-0000-0000-0000-000000000011';
  v_late uuid := '00000000-0000-0000-0000-000000000012';
  v_now timestamp := now() at time zone 'Asia/Shanghai';
  v_in timestamptz;
  rejected boolean;
begin
  insert into public.reservations values
    (v_id,auth.uid(),v_now::date,extract(hour from v_now)::integer,'confirmed'),
    (v_early,auth.uid(),(v_now+interval '1 day')::date,8,'confirmed'),
    (v_late,auth.uid(),(v_now-interval '1 day')::date,8,'confirmed');
  rejected := false;
  begin perform public.v2_check_in(v_early); exception when others then rejected := true; end;
  if not rejected then raise exception 'Early check-in accepted'; end if;
  rejected := false;
  begin perform public.v2_check_in(v_late); exception when others then rejected := true; end;
  if not rejected then raise exception 'Expired check-in accepted'; end if;
  rejected := false;
  begin perform public.v2_check_out(v_id); exception when others then rejected := true; end;
  if not rejected then raise exception 'Checkout before check-in accepted'; end if;
  perform set_config('request.jwt.claim.sub','00000000-0000-0000-0000-000000000002',true);
  rejected := false;
  begin perform public.v2_check_in(v_id); exception when others then rejected := true; end;
  if not rejected then raise exception 'Other user check-in accepted'; end if;
  perform set_config('request.jwt.claim.sub','',true);
  rejected := false;
  begin perform public.v2_check_in(v_id); exception when insufficient_privilege then rejected := true; end;
  if not rejected then raise exception 'Anonymous check-in accepted'; end if;
  perform set_config('request.jwt.claim.sub','00000000-0000-0000-0000-000000000001',true);
  perform public.v2_check_in(v_id);
  if (select status from public.reservations where id=v_id) <> 'in_use' then raise exception 'Check-in did not update reservation'; end if;
  select checked_in_at into v_in from public.reservation_usage_records where reservation_id=v_id;
  if v_in is null then raise exception 'Check-in timestamp missing'; end if;
  perform set_config('TimeZone','America/New_York',true);
  perform public.v2_check_in(v_id);
  if (select checked_in_at from public.reservation_usage_records where reservation_id=v_id) <> v_in then raise exception 'Repeat check-in changed timestamp'; end if;
  perform public.v2_check_out(v_id);
  perform public.v2_check_out(v_id);
  if (select status from public.reservations where id=v_id) <> 'completed' then raise exception 'Check-out did not complete reservation'; end if;
  if (select count(*) from public.reservation_status_events where reservation_id=v_id) <> 2 then raise exception 'Repeated attendance created duplicate events'; end if;
  raise notice 'Attendance SQL tests passed';
end;
$$;
rollback;

// Disposable PostgreSQL integration test for the V3.1 proposal and conflict guards.
// Run with the same PGLITE_MODULE / PGCRYPTO_MODULE variables as usage-metrics-db.mjs.
const {PGlite}=await import(process.env.PGLITE_MODULE||'@electric-sql/pglite');
const {pgcrypto}=await import(process.env.PGCRYPTO_MODULE||'@electric-sql/pglite/contrib/pgcrypto');
import fs from 'node:fs/promises';
import assert from 'node:assert/strict';
import {runBookingAgent} from '../backend/supabase/functions/deepseek-booking-agent/booking-agent-tools.mjs';
import {prepareAdminAction} from '../backend/supabase/functions/deepseek-booking-agent/admin-agent-actions.mjs';

const root=new URL('../backend/supabase/',import.meta.url);
const student='00000000-0000-0000-0000-000000000011';
const other='00000000-0000-0000-0000-000000000012';
const admin='00000000-0000-0000-0000-000000000013';
const db=new PGlite({extensions:{pgcrypto}});
try{
  await db.exec(`create role anon;create role authenticated;create schema auth;create schema extensions;
    create table auth.users(id uuid primary key,email text unique,raw_user_meta_data jsonb default '{}');
    create function auth.uid() returns uuid language sql stable as $$select nullif(current_setting('request.jwt.claim.sub',true),'')::uuid$$;
    grant usage on schema auth to anon,authenticated;grant execute on function auth.uid() to anon,authenticated;
    create extension pgcrypto with schema extensions;create publication supabase_realtime;`);
  for(const file of ['schema.sql','cloud-migration.sql','v2-0-migration.sql','v2-1-fault-response-agent-migration.sql','v2-2-attendance-fix.sql','optional-fault-notes-migration.sql','v3-0-usage-metrics-migration.sql','v3-1-agent-layer-migration.sql','v3-2-agent-fault-note-compat.sql','v3-3-agent-session-clarification-fix.sql','v3-4-agent-privacy-hardening.sql']) await db.exec(await fs.readFile(new URL(file,root),'utf8'));
  for(const [id,name] of [[student,'Student'],[other,'Other'],[admin,'Admin']]) await db.query('insert into auth.users values($1,$2,$3)',[id,`${name}@lab.test`,{login_id:name,display_name:name}]);
  await db.query(`update public.profiles set role='admin' where id=$1`,[admin]);
  const asUser=async id=>{await db.exec('reset role');await db.query(`select set_config('request.jwt.claim.sub',$1,false)`,[id]);await db.exec('set role authenticated')};
  const date=(await db.query(`select ((now() at time zone 'Asia/Shanghai')::date + 1)::text as date`)).rows[0].date;
  await asUser(student);
  const session=(await db.query(`select * from public.v3_save_agent_session(null,'booking_planner',$1::jsonb)`,[{}])).rows[0];
  const mergedSession=(await db.query(`select * from public.v3_save_agent_session($1::uuid,'booking_planner',$2::jsonb)`,[session.session_id,{has_explicit_booking_selection:true}])).rows[0];
  assert.equal(mergedSession.constraints.has_explicit_booking_selection,true);
  const availability={date,startHours:[8],durationHours:1,requiredDeviceCount:2,preferredDevices:[1,2]};
  const plan=await runBookingAgent({
    today:date,message:'帮我找两台设备，用一个小时',history:[],language:'zh',
    complete:async messages=>({message:messages.at(-1).role==='tool'
      ? {role:'assistant',content:'找到可用设备，请选择方案。'}
      : {role:'assistant',tool_calls:[{id:'lookup',type:'function',function:{name:'search_availability',arguments:JSON.stringify(availability)}}]}}),
    plan:async request=>(await db.query(`select public.v3_plan_schedule($1::jsonb) as plan`,[request])).rows[0].plan,
  });
  assert.ok(plan.candidates.length>=1);
  const proposal=(await db.query(`select public.v3_create_booking_proposal($1::uuid,$2::jsonb) as proposal`,[session.session_id,plan.candidates[0]])).rows[0].proposal;
  assert.equal(proposal.state,'ready');
  await db.query(`select public.v3_log_agent_interaction($1::uuid,'booking_planner',$2::jsonb,$3::jsonb,'suggested',null)`,['10000000-0000-0000-0000-000000000010',{message:'Student private request'},{message:'Student private reply'}]);
  await asUser(other);
  assert.equal((await db.query(`select count(*)::int as count from public.agent_sessions`)).rows[0].count,0);
  assert.equal((await db.query(`select count(*)::int as count from public.agent_proposals`)).rows[0].count,0);
  assert.equal((await db.query(`select count(*)::int as count from public.agent_proposal_items`)).rows[0].count,0);
  assert.equal((await db.query(`select count(*)::int as count from public.agent_audit_logs`)).rows[0].count,0);
  await asUser(admin);
  assert.equal((await db.query(`select count(*)::int as count from public.agent_sessions`)).rows[0].count,0);
  assert.equal((await db.query(`select count(*)::int as count from public.agent_proposals`)).rows[0].count,0);
  assert.equal((await db.query(`select count(*)::int as count from public.agent_proposal_items`)).rows[0].count,0);
  assert.equal((await db.query(`select count(*)::int as count from public.agent_audit_logs`)).rows[0].count,0);
  await asUser(student);
  const key='10000000-0000-0000-0000-000000000001';
  const confirmed=(await db.query(`select public.v3_create_reservation_plan($1::uuid,$2::uuid) as result`,[proposal.proposalId,key])).rows[0].result;
  assert.equal(confirmed.reservationIds.length,2);
  const replay=(await db.query(`select public.v3_create_reservation_plan($1::uuid,$2::uuid) as result`,[proposal.proposalId,key])).rows[0].result;
  assert.deepEqual(replay.reservationIds,confirmed.reservationIds);
  await assert.rejects(db.query(`select * from public.v2_create_reservations($1::date,8::smallint,array[3]::smallint[],$2::uuid)`,[date,'10000000-0000-0000-0000-000000000002']),/already has a booking/);
  const legacy=(await db.query(`select * from public.v2_create_reservations($1::date,9::smallint,array[3,4]::smallint[],$2::uuid)`,[date,'10000000-0000-0000-0000-000000000003'])).rows;
  assert.equal(legacy.length,2);
  assert.equal((await db.query(`select count(distinct booking_block_id) as blocks from public.reservations where id=any($1::uuid[])`,[legacy.map(row=>row.reservation_id)])).rows[0].blocks,1);
  await assert.rejects(db.query(`select * from public.v2_reschedule_reservation($1::uuid,$2::date,8::smallint,5::smallint,$3::uuid)`,[legacy[0].reservation_id,date,'10000000-0000-0000-0000-000000000005']),/already has a booking/);
  await asUser(other);
  await assert.rejects(db.query(`select * from public.v2_create_reservations($1::date,8::smallint,array[1]::smallint[],$2::uuid)`,[date,'10000000-0000-0000-0000-000000000004']),/just reserved/);
  await asUser(admin);
  const issue=(await db.query(`insert into public.issue_reports(device_number,reporter_id,description) values(1,$1,'Test fault') returning id`,[admin])).rows[0];
  const faultSession=(await db.query(`select * from public.v3_save_agent_session(null,'fault_responder','{}'::jsonb)`)).rows[0];
  const faultProposal=(await db.query(`select public.v3_create_fault_proposal($1::uuid,'isolate_fault',$2::uuid,'Test isolation') as proposal`,[faultSession.session_id,issue.id])).rows[0].proposal;
  await db.query(`select public.v3_execute_agent_proposal($1::uuid,$2::uuid)`,[faultProposal.proposalId,'10000000-0000-0000-0000-000000000006']);
  assert.equal((await db.query(`select status from public.devices where number=1`)).rows[0].status,'faulty');
  const repairSession=(await db.query(`select * from public.v3_save_agent_session(null,'fault_responder','{}'::jsonb)`)).rows[0];
  const repair=await prepareAdminAction([{function:{name:'prepare_lab_action',arguments:'{"action":"repair_fault","deviceNumber":1}'}}],{
    findOpenFaults:async number=>(await db.query(`select id from public.issue_reports where device_number=$1 and status in ('reported','faulty') and deleted_at is null limit 2`,[number])).rows,
    prepareRepair:async issueId=>(await db.query(`select public.v3_create_fault_proposal($1::uuid,'complete_fault_repair',$2::uuid,'') as proposal`,[repairSession.session_id,issueId])).rows[0].proposal,
  });
  const repairKey='10000000-0000-0000-0000-000000000007';
  assert.equal(repair.kind,'repair');
  assert.equal((await db.query(`select status from public.devices where number=1`)).rows[0].status,'faulty');
  await asUser(other);
  await assert.rejects(db.query(`select public.v3_execute_agent_proposal($1::uuid,$2::uuid)`,[repair.proposal.proposalId,repairKey]),/proposal was not found/i);
  await asUser(admin);
  await db.query(`select public.v3_execute_agent_proposal($1::uuid,$2::uuid)`,[repair.proposal.proposalId,repairKey]);
  await db.query(`select public.v3_execute_agent_proposal($1::uuid,$2::uuid)`,[repair.proposal.proposalId,repairKey]);
  assert.equal((await db.query(`select count(*)::int as count from public.fault_response_events where issue_id=$1 and action='repaired'`,[issue.id])).rows[0].count,1);
  assert.equal((await db.query(`select status from public.devices where number=1`)).rows[0].status,'available');
  assert.equal((await db.query(`select status from public.reservations where id=$1`,[confirmed.reservationIds[0]])).rows[0].status,'confirmed');
  await asUser(student);
  const cancellation=(await db.query(`select public.v3_cancel_reservation_series($1::uuid) as result`,[confirmed.seriesId])).rows[0].result;
  assert.equal(Number(cancellation.cancelledCount),2);
  assert.equal((await db.query(`select count(*) as count from public.reservations where id=any($1::uuid[]) and status='cancelled'`,[confirmed.reservationIds])).rows[0].count,2);
  console.log('PASS: V3 proposal confirmation is idempotent; user-slot and cross-user device guards apply to V3 and V2 paths; fault isolation and empty-note repair preserve reservations; future series cancellation is grouped');
}finally{await db.close()}

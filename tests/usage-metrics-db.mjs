// Disposable PostgreSQL integration test. Requires @electric-sql/pglite and pgcrypto.
// Run: PGLITE_MODULE=/path/to/pglite PGCRYPTO_MODULE=/path/to/pgcrypto node tests/usage-metrics-db.mjs
const {PGlite}=await import(process.env.PGLITE_MODULE||'@electric-sql/pglite');
const {pgcrypto}=await import(process.env.PGCRYPTO_MODULE||'@electric-sql/pglite/contrib/pgcrypto');
import fs from 'node:fs/promises';
import assert from 'node:assert/strict';

const root=new URL('../backend/supabase/',import.meta.url);
const student='00000000-0000-0000-0000-000000000001';
const admin='00000000-0000-0000-0000-000000000002';
const db=new PGlite({extensions:{pgcrypto}});
try{
  await db.exec(`create role anon;create role authenticated;create schema auth;create schema extensions;
    create table auth.users(id uuid primary key,email text unique,raw_user_meta_data jsonb default '{}');
    create function auth.uid() returns uuid language sql stable as $$select nullif(current_setting('request.jwt.claim.sub',true),'')::uuid$$;
    grant usage on schema auth to anon,authenticated;grant execute on function auth.uid() to anon,authenticated;
    create extension pgcrypto with schema extensions;create publication supabase_realtime;`);
  for(const file of ['schema.sql','cloud-migration.sql','v2-0-migration.sql','v2-2-attendance-fix.sql','v3-0-usage-metrics-migration.sql','v3-1-agent-layer-migration.sql']){
    await db.exec(await fs.readFile(new URL(file,root),'utf8'));
  }
  for(const [id,name] of [[student,'Student'],[admin,'Admin']]){
    await db.query('insert into auth.users values($1,$2,$3)',[id,`${name}@lab.test`,{login_id:name,display_name:name}]);
  }
  await db.query(`update public.profiles set role='admin' where id=$1`,[admin]);
  const asUser=async id=>{await db.exec('reset role');await db.query(`select set_config('request.jwt.claim.sub',$1,false)`,[id]);await db.exec('set role authenticated')};
  const baseline=(await db.query(`select (min(effective_at) at time zone 'Asia/Shanghai')::date as date from public.device_status_history`)).rows[0].date;
  const addReservation=async(device,hour,status)=>{
    const row=(await db.query(`insert into public.reservations(device_number,user_id,reservation_date,start_hour,status) values($1,$2,$3,$4,$5) returning id`,[device,student,baseline,hour,status])).rows[0];
    return row.id;
  };
  const fifty=await addReservation(1,8,'completed');
  const ten=await addReservation(2,9,'completed');
  const noShow=await addReservation(3,10,'no_show');
  const incomplete=await addReservation(4,11,'in_use');
  const clipped=await addReservation(5,12,'completed');
  await db.query(`insert into public.reservation_usage_records(reservation_id,checked_in_at,checked_out_at,checked_in_by,checked_out_by)
    values($1,public.v3_slot_start($2::date,8::smallint)+interval '5 minutes',public.v3_slot_start($2::date,8::smallint)+interval '55 minutes',$3,$3),
          ($4,public.v3_slot_start($2::date,9::smallint)+interval '20 minutes',public.v3_slot_start($2::date,9::smallint)+interval '30 minutes',$3,$3),
          ($5,public.v3_slot_start($2::date,11::smallint)+interval '10 minutes',null,$3,null),
          ($6,public.v3_slot_start($2::date,12::smallint)-interval '10 minutes',public.v3_slot_start($2::date,13::smallint)+interval '10 minutes',$3,$3)`,[fifty,baseline,student,ten,incomplete,clipped]);
  await db.query(`insert into public.maintenance_windows(device_number,reservation_date,start_hour,end_hour,reason,status,created_by)
    values(6,$1,8,9,'Device maintenance','active',$2),(6,$1,8,9,'Overlapping maintenance','scheduled',$2),(null,$1,16,17,'Whole-lab maintenance','active',$2)`,[baseline,admin]);
  await asUser(admin);
  await db.query(`update public.devices set status='faulty' where number=7`);
  assert.equal((await db.query(`select status from public.device_status_history where device_number=7 order by effective_at desc limit 1`)).rows[0].status,'faulty');
  const metrics=(await db.query(`select public.v3_admin_usage_metrics($1::date,$1::date,null) as metrics`,[baseline])).rows[0].metrics;
  assert.equal(metrics.capacity_data_complete,true);
  assert.equal(Number(metrics.actual_used_minutes),120);
  assert.equal(Number(metrics.booked_device_minutes),300);
  assert.equal(Number(metrics.provisional_used_minutes),0);
  assert.equal(metrics.data_completeness.checked_in_missing_checkout_count,1);
  assert.equal(metrics.data_completeness.complete_usage_record_count,3);
  assert.equal(Number(metrics.no_show_rate),0.2);
  assert.equal(Number(metrics.capacity_device_minutes),18660); // Capacity reductions are a union: device 7 is already unavailable during the whole-lab maintenance hour.
  await asUser(student);
  await assert.rejects(db.query(`select public.v3_admin_usage_metrics($1::date,$1::date,null)`,[baseline]),/Administrator access is required/);
  console.log('PASS: V3 device-minute metrics, clipping, incomplete attendance, maintenance union, no-show and administrator boundary');
}finally{await db.close()}

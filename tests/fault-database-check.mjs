// Isolated PostgreSQL checks. Requires @electric-sql/pglite and its pgcrypto extension.
const {PGlite}=await import(process.env.PGLITE_MODULE||'@electric-sql/pglite');
const {pgcrypto}=await import(process.env.PGCRYPTO_MODULE||'@electric-sql/pglite/contrib/pgcrypto');
import fs from 'node:fs/promises';
import assert from 'node:assert/strict';
const root=new URL('../backend/supabase/',import.meta.url);
const a='00000000-0000-0000-0000-000000000001',b='00000000-0000-0000-0000-000000000002',admin='00000000-0000-0000-0000-000000000003';
for(const withV2 of [false,true]){
 const db=new PGlite({extensions:{pgcrypto}});
 try{
 await db.exec(`create role anon;create role authenticated;create schema auth;create schema extensions;
 create table auth.users(id uuid primary key,email text unique,raw_user_meta_data jsonb default '{}');
 create function auth.uid() returns uuid language sql stable as $$select nullif(current_setting('request.jwt.claim.sub',true),'')::uuid$$;
 grant usage on schema auth to anon,authenticated;grant execute on function auth.uid() to anon,authenticated;
 create extension pgcrypto with schema extensions;create publication supabase_realtime;`);
 const migrations=['schema.sql','cloud-migration.sql'];
 if(withV2)migrations.push('v2-0-migration.sql','v2-1-fault-response-agent-migration.sql');
 migrations.push('immediate-fault-report-migration.sql');
 for(const file of migrations)await db.exec(await fs.readFile(new URL(file,root),'utf8'));
 for(const [id,login]of [[a,'StudentA'],[b,'StudentB'],[admin,'Admin']])await db.query(`insert into auth.users values($1,$2,$3)`,[id,login+'@lab.test',{login_id:login,display_name:login}]);
 await db.query(`update public.profiles set role='admin' where id=$1`,[admin]);
 const asUser=async id=>{await db.exec('reset role');await db.query(`select set_config('request.jwt.claim.sub',$1,false)`,[id]);await db.exec('set role authenticated')};
 await asUser(a);
 const first=(await db.query(`insert into public.issue_reports(device_number,reporter_id,description,status) values(1,$1,'Monitor has no signal','reported') returning id,status`,[a])).rows[0];
 assert.equal(first.status,'faulty');
 assert.equal((await db.query(`select status from public.devices where number=1`)).rows[0].status,'faulty');
 await asUser(b);
 assert.equal((await db.query(`select status from public.devices where number=1`)).rows[0].status,'faulty');
 assert.equal((await db.query(`select count(*)::int n from public.issue_reports`)).rows[0].n,0);
 if(withV2)await assert.rejects(db.query(`select public.v2_create_reservations(current_date+1,8::smallint,ARRAY[1]::smallint[],gen_random_uuid())`),/unavailable/);
 else await assert.rejects(db.query(`select public.reserve_hourly_devices(current_date+1,8::smallint,ARRAY[1]::smallint[])`),/faulty/);
 const second=(await db.query(`insert into public.issue_reports(device_number,reporter_id,description,status) values(1,$1,'Another open report','resolved') returning id,status`,[b])).rows[0];
 assert.equal(second.status,'faulty');
 await asUser(admin);
 const repair=async id=>{
   if(withV2)await db.query(`select public.v2_complete_fault_repair($1,'Replaced monitor cable')`,[id]);
   else{await db.query(`update public.issue_reports set status='resolved' where id=$1`,[id]);await db.query(`update public.devices set status='available' where number=1`)}
 };
 await repair(first.id);assert.equal((await db.query(`select status from public.devices where number=1`)).rows[0].status,'faulty');
 await repair(second.id);assert.equal((await db.query(`select status from public.devices where number=1`)).rows[0].status,'available');
 if(withV2)assert.equal((await db.query(`select count(*)::int n from public.fault_response_events where action='isolated'`)).rows[0].n,2);
 // A second migration run must preserve resolved equipment and remain safe.
 await db.exec('reset role');await db.exec(await fs.readFile(new URL('immediate-fault-report-migration.sql',root),'utf8'));
 assert.equal((await db.query(`select status from public.devices where number=1`)).rows[0].status,'available');
 console.log('PASS:',withV2?'V2.1 backend':'legacy backend','atomic isolation, other-user visibility, server-side booking rejection, duplicate report guard, repair and idempotent migration');
 }finally{await db.close()}
}

// Requires @electric-sql/pglite. Run: node tests/admin-reservations-db.mjs
import fs from 'node:fs/promises';
import assert from 'node:assert/strict';
const {PGlite}=await import(process.env.PGLITE_MODULE||'@electric-sql/pglite');
const {pgcrypto}=await import(process.env.PGCRYPTO_MODULE||'@electric-sql/pglite/contrib/pgcrypto');
const db=new PGlite({extensions:{pgcrypto}});
const a='00000000-0000-0000-0000-000000000001',b='00000000-0000-0000-0000-000000000002',admin='00000000-0000-0000-0000-000000000003';
try{
 await db.exec(`create role anon;create role authenticated;create schema auth;create schema extensions;
 create table auth.users(id uuid primary key,email text unique,raw_user_meta_data jsonb default '{}');
 create function auth.uid() returns uuid language sql stable as $$select nullif(current_setting('request.jwt.claim.sub',true),'')::uuid$$;
 grant usage on schema auth to anon,authenticated;grant execute on function auth.uid() to anon,authenticated;
 create extension pgcrypto with schema extensions;create publication supabase_realtime;`);
 for(const file of ['01_schema.sql','02_cloud-migration.sql','06_v2-0-migration.sql'])await db.exec(await fs.readFile(new URL('../database/'+file,import.meta.url),'utf8'));
 for(const [id,login]of [[a,'StudentA'],[b,'StudentB'],[admin,'Admin']])await db.query(`insert into auth.users values($1,$2,$3)`,[id,login+'@lab.test',{login_id:login,display_name:login}]);
 await db.query(`update public.profiles set role='admin' where id=$1`,[admin]);
 const asUser=async id=>{await db.exec('reset role');await db.query(`select set_config('request.jwt.claim.sub',$1,false)`,[id]);await db.exec('set role authenticated')};
 await asUser(a);
 const reservation=(await db.query(`select public.v2_create_reservations(current_date+1,9::smallint,ARRAY[23]::smallint[],gen_random_uuid()) result`)).rows[0].result;
 const id=(await db.query(`select id from public.reservations where user_id=$1 and device_number=23`,[a])).rows[0].id;
 await asUser(b);await assert.rejects(db.query(`select public.v2_cancel_reservation($1)`,[id]),/not found/);
 await asUser(admin);
 const rows=(await db.query(`select r.device_number,r.start_hour,p.display_name,p.login_id from public.reservations r join public.profiles p on p.id=r.user_id where r.id=$1`,[id])).rows;
 assert.equal(rows[0].display_name,'StudentA');assert.equal(rows[0].start_hour,9);assert.equal(rows[0].device_number,23);
 await db.query(`select public.v2_cancel_reservation($1)`,[id]);
 assert.equal((await db.query(`select actor_id from public.reservation_status_events where reservation_id=$1 and to_status='cancelled'`,[id])).rows[0].actor_id,admin);
 await asUser(a);assert.equal((await db.query(`select status from public.reservations where id=$1`,[id])).rows[0].status,'cancelled');
 await asUser(b);await db.query(`select public.v2_create_reservations(current_date+1,9::smallint,ARRAY[23]::smallint[],gen_random_uuid())`);
 await db.exec('reset role');await db.query(`update public.reservations set status='in_use' where user_id=$1`,[b]);
 await asUser(admin);const started=(await db.query(`select id from public.reservations where user_id=$1`,[b])).rows[0].id;
 await assert.rejects(db.query(`select public.v2_cancel_reservation($1)`,[started]),/unstarted/);
 console.log('PASS: admin reads owner and slot, other student denied, admin cancellation audited, student sees cancellation, slot released, started reservation protected');
}finally{await db.close()}

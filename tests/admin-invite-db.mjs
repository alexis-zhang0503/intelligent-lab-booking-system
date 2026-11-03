// Requires @electric-sql/pglite. Run: node tests/admin-invite-db.mjs
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
 for(const file of ['schema.sql','cloud-migration.sql','admin-invite-migration.sql'])await db.exec(await fs.readFile(new URL('../backend/supabase/'+file,import.meta.url),'utf8'));
 for(const [id,login]of [[a,'StudentA'],[b,'StudentB'],[admin,'Admin']])await db.query(`insert into auth.users values($1,$2,$3)`,[id,login+'@lab.test',{login_id:login,display_name:login}]);
 await db.query(`update public.profiles set role='admin' where id=$1`,[admin]);
 const asUser=async id=>{await db.exec('reset role');await db.query(`select set_config('request.jwt.claim.sub',$1,false)`,[id]);await db.exec('set role authenticated')};
 // Reproduce the actual production definition with an extension outside search_path.
 let legacy=await fs.readFile(new URL('../backend/supabase/admin-invite-migration.sql',import.meta.url),'utf8');
 legacy=legacy.slice(legacy.indexOf('create or replace function public.create_admin_invite('),legacy.indexOf('-- The Auth trigger'));
 legacy=legacy.replace(/generated_code := .*;/,"generated_code := 'NJUPT-' || upper(encode(gen_random_bytes(6), 'hex'));").replaceAll('extensions.','');await db.exec(legacy);
 await asUser(admin);await assert.rejects(db.query(`select * from public.create_admin_invite(now()+interval '7 days')`),/gen_random_bytes/);
 await db.exec('reset role');await db.exec(await fs.readFile(new URL('../backend/supabase/admin-invite-fix-migration.sql',import.meta.url),'utf8'));
 await asUser(a);await assert.rejects(db.query(`select * from public.create_admin_invite_for_days(7)`),/Administrator/);
 await asUser(admin);await assert.rejects(db.query(`select * from public.create_admin_invite_for_days(null)`),/validity/);await assert.rejects(db.query(`select * from public.create_admin_invite_for_days(31)`),/validity/);
 await assert.rejects(db.query(`select * from public.create_admin_invite(null)`),/expiry/);
 const codes=[];
 for(const days of [1,7,30]){let result=(await db.query(`select *,extract(epoch from(expires_at-now()))::int ttl from public.create_admin_invite_for_days($1)`,[days])).rows[0];assert.match(result.invite_code,/^NJUPT-[A-F0-9]{12}$/);assert.equal(result.ttl,days*86400);codes.push(result.invite_code)}
 assert.equal(new Set(codes).size,3);
 await db.exec('reset role');let stored=(await db.query(`select code_hash,created_by from public.admin_invites`)).rows;assert.ok(stored.every(row=>!codes.includes(row.code_hash)&&row.created_by===admin));
 const newId='00000000-0000-0000-0000-000000000004';await db.query(`insert into auth.users values($1,'new-admin@lab.test',$2)`,[newId,{login_id:'NewAdmin',display_name:'New Admin',admin_invite_code:codes[0].toLowerCase()}]);
 assert.equal((await db.query(`select role from public.profiles where id=$1`,[newId])).rows[0].role,'admin');
 await assert.rejects(db.query(`insert into auth.users values('00000000-0000-0000-0000-000000000005','duplicate@lab.test',$1)`,[{admin_invite_code:codes[0]}]),/invalid or expired/);
 await db.query(`update public.admin_invites set expires_at=now()-interval '1 second' where code_hash=extensions.crypt($1,code_hash)`,[codes[1]]);
 await assert.rejects(db.query(`insert into auth.users values('00000000-0000-0000-0000-000000000006','expired@lab.test',$1)`,[{admin_invite_code:codes[1]}]),/invalid or expired/);
 await db.query(`update public.profiles set active=false where id=$1`,[admin]);await asUser(admin);await assert.rejects(db.query(`select * from public.create_admin_invite_for_days(7)`),/Administrator/);
 await db.exec('reset role');await db.exec(await fs.readFile(new URL('../backend/supabase/admin-invite-fix-migration.sql',import.meta.url),'utf8'));
 console.log('PASS: reproduced production crypto failure, fixed 1/7/30-day generation, hashed storage, one-time signup, case normalization, expiration and inactive/non-admin guards');
}finally{await db.close()}

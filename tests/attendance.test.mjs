import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const rulesSource=fs.readFileSync(new URL('../web/core/attendanceRules.js',import.meta.url),'utf8');
const {attendanceState,attendanceError,reservationStart}=await import('data:text/javascript;base64,'+Buffer.from(rulesSource).toString('base64'));
const booking={id:'r1',date:'2026-11-03',hour:10,status:'confirmed'};
test('CST schedule has an absolute instant regardless of computer timezone',()=>assert.equal(reservationStart(booking),Date.parse('2026-11-03T02:00:00Z')));
test('check-in accepts start, rejects before start and exact end',()=>{
 assert.equal(attendanceState(booking,Date.parse('2026-11-03T01:59:59Z')),'early');
 assert.equal(attendanceState(booking,Date.parse('2026-11-03T02:00:00Z')),'check_in');
 assert.equal(attendanceState(booking,Date.parse('2026-11-03T02:59:59Z')),'check_in');
 assert.equal(attendanceState(booking,Date.parse('2026-11-03T03:00:00Z')),'expired');
});
test('closed reservations cannot check in; in-use reservations can check out late',()=>{
 for(const status of ['completed','cancelled','no_show'])assert.equal(attendanceState({...booking,status}),'unavailable');
 assert.equal(attendanceState({...booking,status:'in_use'},Date.parse('2026-11-04T02:00:00Z')),'check_out');
});
test('backend failures have distinct actionable messages',()=>{
 assert.match(attendanceError({code:'PGRST202'}),/SQL/);
 assert.match(attendanceError({code:'42P01'}),/V2/);
 assert.match(attendanceError({message:'Manual check-in is only available during the booked hour'}),/北京时间/);
 assert.match(attendanceError({message:'Failed to fetch'}),/网络/);
});
function setup(){
 const nodes=new Map();const q=key=>{if(!nodes.has(key))nodes.set(key,{disabled:false,hidden:false,classList:{toggle(){}},querySelector(){return q('button')}});return nodes.get(key)};
 const context=vm.createContext({console,Date,Map,Set,Intl,Math,JSON,Error,Promise,localStorage:{getItem(){return 'zh'}},document:{querySelector:q,querySelectorAll(){return []},addEventListener(){}},setTimeout,clearTimeout,attendanceState,attendanceError,reservationStart});
const source=fs.readFileSync(new URL('../app.js',import.meta.url),'utf8').replace(/^playSiteEntry\(\);$/gm,'').replace(/^import .*;\r?$/gm,'').replace(/bootCloud\(\);\s*$/,'');
 vm.runInContext(source+`\nlet calls=0,refreshCalls=0;let pendingResolve;const supabase={rpc:async()=>{calls++;return new Promise(resolve=>pendingResolve=resolve)}};db={users:[{id:'u'}],session:'u',bookings:[{id:'r1',date:'2026-11-03',hour:10,status:'confirmed'}]};attendanceState=()=> 'check_in';me=()=>{};refreshCloud=async()=>{refreshCalls++;return true};toast=m=>globalThis.message=m;globalThis.api={saveAttendance,resolve:r=>pendingResolve(r),counts:()=>({calls,refreshCalls}),setRefresh:f=>refreshCloud=f,pending:()=>attendancePending.size};`,context);
 return context;
}
test('double click makes one request and clears busy state',async()=>{
 const c=setup();const p=c.api.saveAttendance('r1','check_in');await c.api.saveAttendance('r1','check_in');assert.equal(c.api.counts().calls,1);c.api.resolve({error:null});await p;assert.equal(c.api.pending(),0);assert.equal(c.api.counts().refreshCalls,1);
});
test('committed write followed by refresh failure is not shown as a failed write',async()=>{
 const c=setup();c.api.setRefresh(async()=>{throw Error('network')});const p=c.api.saveAttendance('r1','check_in');c.api.resolve({error:null});await p;assert.match(c.message,/已保存/);assert.equal(c.api.pending(),0);
});
test('failed RPC releases busy state and explains a missing migration',async()=>{
 const c=setup();const p=c.api.saveAttendance('r1','check_in');c.api.resolve({error:{code:'PGRST202'}});await p;assert.match(c.message,/SQL/);assert.equal(c.api.pending(),0);
});

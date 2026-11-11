import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const rules=fs.readFileSync(new URL('../web/core/attendanceRules.js',import.meta.url),'utf8');
const {attendanceState,attendanceError,reservationStart}=await import('data:text/javascript;base64,'+Buffer.from(rules).toString('base64'));
const instant=Date.parse('2026-11-04T09:30:00+08:00');
class Clock extends Date{constructor(...args){super(...(args.length?args:[instant]))}static now(){return instant}}
function setup(){
 const nodes=new Map();const node=s=>{if(!nodes.has(s))nodes.set(s,{value:'',innerHTML:'',dataset:{},classList:{add(){},remove(){},contains(){return false}},setAttribute(){}});return nodes.get(s)};
 const context=vm.createContext({console:{warn(){}},Date:Clock,Map,Set,Intl,Number,String,Math,JSON,crypto:{randomUUID:()=> 'key-'+(++context.uuid)},uuid:0,
 localStorage:{getItem:()=> 'zh'},document:{querySelector:node,querySelectorAll:()=>[],addEventListener(){},body:{dataset:{},classList:{contains:()=>false}}},window:{addEventListener(){}},setTimeout:()=>1,clearTimeout(){},setInterval:()=>1,
 DESKS:Array.from({length:40},(_,i)=>i+1),SLOTS:[8,9,10,11,12,13,14,15,16],supabase:{},hourlyRange:h=>`${h}:00–${h+1}:00`,hourToMinutes:h=>h*60,attendanceState,attendanceError,reservationStart});
 vm.runInContext(fs.readFileSync(new URL('../app.js',import.meta.url),'utf8').replace(/^import .*;\r?\n/gm,'').replace(/^playSiteEntry\(\);\r?$/gm,'').replace(/\r?\nbootCloud\(\);\s*$/,''),context);
 const run=code=>vm.runInContext(code,context);
 run(`db={users:[{id:'s',name:'Student',role:'student',loginId:'B001'}],session:'s',devices:[],faults:[],bookings:[]};date=today(1);page='dashboard';startHour=9;endHour=12;desks=new Set([23]);syncLanguage=()=>{};dashStudent=()=>{};toast=(text,type)=>{globalThis.notice={text,type}};refreshCloud=async()=>true;showBookingSuccess=minutes=>{globalThis.successMinutes=minutes};syncBookingRange()`);
 return {run,node};
}
test('point selectors offer only ends at least one hour later, including 17:00',()=>{
 const {run}=setup();const html=run('timePick()');assert.match(html,/>09:00<\/option>/);assert.doesNotMatch(html,/09:00–10:00/);
 const end=html.split('id="endTime"')[1];assert.doesNotMatch(end,/disabled/);assert.match(end,/value="10"/);assert.match(end,/value="17"/);assert.doesNotMatch(end,/value="9"/);
 assert.equal(run('timeRange()'),'09:00–12:00');assert.deepEqual(Array.from(run('[...hours]')),[9,10,11]);
});
test('endpoint changes preserve valid ends, correct invalid ends, and recompute conflicts',()=>{
 const {run,node}=setup();run('bindTimes()');node('#endTime').onchange({target:{value:'14'}});assert.equal(run('endHour'),14);assert.equal(run('hours.size'),5);
 node('#startTime').onchange({target:{value:'15'}});assert.equal(run('endHour'),16);assert.equal(run('hours.size'),1);
 run(`startHour=9;endHour=12;syncBookingRange();db.bookings=[{deskNo:23,date:date,hour:11,status:'confirmed'}]`);assert.equal(run('conflict(23)'),true);
 run(`endHour=11;syncBookingRange()`);assert.equal(run('conflict(23)'),false);
});
test('minimum duration, closing bound and Beijing same-day start are enforced',()=>{
 const {run}=setup();run(`date=today();startHour=9;endHour=10`);assert.equal(run('bookingWindow().valid'),false);assert.equal(run('bookingWindow().nextStart'),10);
 run('startHour=16;endHour=17');assert.equal(run('bookingWindow().valid'),true);run('endHour=18');assert.equal(run('bookingWindow().valid'),false);run('endHour=16');assert.equal(run('bookingWindow().valid'),false);
});
test('checked-in current-hour devices have a badge without blocking a free future interval',()=>{
 const {run}=setup();run(`date=today();startHour=10;endHour=12;syncBookingRange();db.bookings=[{id:'live',deskNo:23,date:date,hour:9,status:'in_use',usage:{checked_in_at:'2026-11-04T01:00:00Z',checked_out_at:null}}]`);
 assert.equal(run('deviceInUse(23)'),true);assert.equal(run('conflict(23)'),false);
 let html=run('map("student")');assert.match(html,/class="desk in-use selected" data-desk="23"/);assert.match(html,/23号设备，使用中，已选择/);assert.match(html,/>使用中<\/i>/);
 assert.match(run('map("admin")'),/in-use/);
 run(`db.bookings[0].usage.checked_out_at='2026-11-04T01:20:00Z'`);assert.equal(run('deviceInUse(23)'),false);
});
test('future, expired, cancelled and completed reservations are not live usage',()=>{
 const {run}=setup();run(`date=today();db.bookings=[{deskNo:23,date:date,hour:9,status:'confirmed'}]`);assert.equal(run('deviceInUse(23)'),false);
 for(const status of ['cancelled','completed','no_show']){run(`db.bookings[0].status=${JSON.stringify(status)}`);assert.equal(run('deviceInUse(23)'),false)}
 run(`db.bookings[0].status='in_use';db.bookings[0].hour=8`);assert.equal(run('deviceInUse(23)'),false);
 run(`db.bookings[0].hour=10`);assert.equal(run('deviceInUse(23)'),false);
 run(`date=today(1);db.bookings[0].date=date;db.bookings[0].hour=9`);assert.equal(run('deviceInUse(23)'),false);
});
test('multi-hour booking uses protected hourly service, distinct keys and actual duration',async()=>{
 const {run}=setup();run(`globalThis.calls=[];supabase.rpc=async(name,args)=>{calls.push({name,args});return {error:null}}`);await run('book()');
 const calls=run('calls');assert.deepEqual(Array.from(calls,x=>x.args.p_start_hour),[9,10,11]);assert.equal(new Set(calls.map(x=>x.args.p_idempotency_key)).size,3);assert.ok(calls.every(x=>x.name==='v2_create_reservations'&&x.args.p_device_numbers[0]===23));assert.equal(run('successMinutes'),180);assert.equal(run('bookingSubmitting'),false);
});
test('duplicate submissions are suppressed while a range is being booked',async()=>{
 const {run}=setup();run(`globalThis.calls=0;supabase.rpc=()=>{calls++;return new Promise(resolve=>{globalThis.finish=resolve})}`);const first=run('book()');await run('book()');assert.equal(run('calls'),1);
 run(`supabase.rpc=async()=>({error:null});finish({error:null})`);await first;assert.equal(run('successMinutes'),180);
});
test('mid-range rejection stops later hours and reports partial success without success overlay',async()=>{
 const {run}=setup();run(`globalThis.calls=[];supabase.rpc=async(name,args)=>{calls.push(args);return {error:args.p_start_hour===10?{message:'Conflict'}:null}}`);await run('book()');assert.equal(run('calls.length'),2);assert.equal(run('typeof successMinutes'),'undefined');assert.match(run('notice.text'),/1\/3/);assert.match(run('notice.text'),/我的/);assert.equal(run('bookingSubmitting'),false);
});
test('refresh failure after all commits is not reported as a failed range',async()=>{
 const {run}=setup();run(`supabase.rpc=async()=>({error:null});refreshCloud=async()=>{throw Error('offline')}`);await run('book()');assert.equal(run('successMinutes'),180);assert.equal(run('typeof notice'),'undefined');
});

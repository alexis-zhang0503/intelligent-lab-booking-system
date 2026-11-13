import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const rules=fs.readFileSync(new URL('../js/core/attendanceRules.js',import.meta.url),'utf8');
const {reservationStart,attendanceState}=await import('data:text/javascript;base64,'+Buffer.from(rules).toString('base64'));
const now=Date.parse('2026-11-04T10:30:00+08:00');
class Clock extends Date{constructor(...args){super(...(args.length?args:[now]))}static now(){return now}}
function setup(){
 const nodes=new Map();const node=s=>{if(!nodes.has(s))nodes.set(s,{innerHTML:'',scrollTop:0,dataset:{},classList:{add(){},remove(){}},setAttribute(){},focus(){this.focused=true}});return nodes.get(s)};
 const ctx=vm.createContext({console,Date:Clock,Map,Set,Intl,Number,String,Math,JSON,localStorage:{getItem:()=> 'zh'},document:{querySelector:node,querySelectorAll:()=>[],addEventListener(){},body:{dataset:{}},activeElement:null},window:{addEventListener(){}},setTimeout:()=>1,clearTimeout(){},setInterval:()=>1,supabase:{},DESKS:Array.from({length:40},(_,i)=>i+1),SLOTS:[8,9,10,11,12,13,14,15,16],hourlyRange:h=>`${h}:00–${h+1}:00`,hourToMinutes:h=>h*60,reservationStart,attendanceState:b=>attendanceState(b,now),playIntro(){}});
 vm.runInContext(fs.readFileSync(new URL('../app.js',import.meta.url),'utf8').replace(/^import .*;\r?\n/gm,'').replace(/\r?\nbootCloud\(\);\s*$/,''),ctx);
 const run=code=>vm.runInContext(code,ctx);
 run(`db={users:[{id:'s',name:'Student',role:'student',loginId:'B001'}],session:'s',devices:[],faults:[],bookings:[]};syncLanguage=()=>{};globalThis.make=(id,date,hour,status='confirmed',deskNo=23)=>({id,userId:'s',date,hour,status,deskNo});`);
 return {run,node};
}
test('both actionable states precede upcoming records, followed by recent history',()=>{
 const {run}=setup();run(`db.bookings=[make('old',today(-1),12,'completed'),make('later',today(1),9),make('current',today(),10),make('soon',today(),11),make('used',today(),10,'in_use'),make('recent',today(),9,'no_show')]`);
 assert.deepEqual(Array.from(run('personalReservationRecords(db.bookings).map(x=>x.id)')),['used','current','soon','later','recent','old']);
 assert.deepEqual(Array.from(run('db.bookings.map(x=>x.id)')),['old','later','current','soon','used','recent']);
});
test('end instant governs 24-hour removal and the exact boundary remains visible',()=>{
 const {run}=setup();run(`db.bookings=[make('older',today(-1),8,'completed'),make('within',today(-1),10,'completed'),make('future',today(2),8)];globalThis.boundary=Date.parse(today()+'T10:00:00+08:00')`);
 assert.deepEqual(Array.from(run('personalReservationRecords(db.bookings,boundary).map(x=>x.id)')),['future','within']);
 assert.deepEqual(Array.from(run('personalReservationRecords(db.bookings,boundary+3600000).map(x=>x.id)')),['future','within']);
 assert.deepEqual(Array.from(run('personalReservationRecords(db.bookings,boundary+3600001).map(x=>x.id)')),['future']);
});
test('overdue unchecked-out usage is retained; a checked-out old record is removed',()=>{
 const {run}=setup();run(`db.bookings=[make('active',today(-2),9,'in_use'),{...make('closed',today(-2),9,'in_use'),usage:{checked_out_at:'2026-11-02T02:00:00Z'}}]`);
 assert.deepEqual(Array.from(run('personalReservationRecords(db.bookings).map(x=>x.id)')),['active']);
});
test('cancelled and completed future records do not outrank upcoming valid reservations',()=>{
 const {run}=setup();run(`db.bookings=[make('cancelled',today(2),9,'cancelled'),make('valid',today(1),9),make('completed',today(),10,'completed')]`);
 assert.equal(run('personalReservationRecords(db.bookings)[0].id'),'valid');
});
test('equal priority and time use device number for deterministic order',()=>{
 const {run}=setup();run(`db.bookings=[make('b',today(),10,'confirmed',23),make('a',today(),10,'confirmed',2)]`);
 assert.deepEqual(Array.from(run('personalReservationRecords(db.bookings).map(x=>x.deskNo)')),[2,23]);
});
test('personal lists share the order and removal, while full history still feeds completed statistics',()=>{
 const {run,node}=setup();run(`db.bookings=[make('old',today(-2),9,'completed',40),make('future',today(1),9,'confirmed',8),make('current',today(),10,'confirmed',7)];me()`);
 const html=node('#content').innerHTML;
 for(const id of ['personalUsageList','personalBookingList']){
  const section=html.split('id="'+id+'"')[1].split('</section>')[0];
  assert.match(section,/class="personal-record-list" role="region"/);assert.match(section,/tabindex="0"/);
  assert.ok(section.indexOf('7号设备')<section.indexOf('8号设备'));assert.doesNotMatch(section,/40号设备/);
 }
 assert.match(html,/<span>已完成记录<\/span><b class="">1<\/b>/);assert.equal(run('db.bookings.length'),3);
});
test('automatic rerenders retain independent list scroll positions and keyboard focus',()=>{
 const {run,node}=setup();node('#personalUsageList').scrollTop=120;node('#personalBookingList').scrollTop=240;
 run(`document.activeElement={closest:()=>({id:'personalUsageList'})};me()`);
 assert.equal(node('#personalUsageList').scrollTop,120);assert.equal(node('#personalBookingList').scrollTop,240);assert.equal(node('#personalUsageList').focused,true);
});

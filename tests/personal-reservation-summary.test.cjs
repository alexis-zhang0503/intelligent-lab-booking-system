const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');

function setup(){
  const nodes=new Map();
  const node=selector=>{if(!nodes.has(selector))nodes.set(selector,{innerHTML:'',classList:{add(){},remove(){}},setAttribute(){}});return nodes.get(selector)};
  const context=vm.createContext({Date,Map,Set,Intl,Number,String,Math,JSON,
    localStorage:{getItem:()=> 'zh'},document:{body:{dataset:{}},querySelector:node,querySelectorAll:()=>[],addEventListener(){}},
    window:{addEventListener(){}},setTimeout:()=>1,clearTimeout(){},setInterval:()=>1,
    DESKS:Array.from({length:40},(_,i)=>i+1),SLOTS:[8,9,10,11,12,13,14,15,16],supabase:{},
    hourToMinutes:h=>h*60,hourlyRange:h=>`${h}:00–${h+1}:00`,playIntro(){},attendanceState:()=> "closed",reservationStart:b=>new Date(`${b.date}T00:00:00`).getTime()+b.hour*3600000
  });
  const source=fs.readFileSync(new URL('../web/app.js',`file://${__filename}`),'utf8')
    .replace(/^import .*;\r?\n/gm,'').replace(/\nbootCloud\(\);\s*$/,'');
  vm.runInContext(source,context);
  const run=code=>vm.runInContext(code,context);
  run(`db={users:[{id:'student',name:'学生',role:'student',loginId:'B001'}],session:'student',devices:[],faults:[],bookings:[]};syncLanguage=()=>{};toast=()=>{};`);
  return {run,node};
}

test('upcoming totals exclude cancelled and closed records but include legacy reserved status',()=>{
  const {run}=setup();
  run(`db.bookings=['confirmed','reserved','cancelled','completed','no_show','in_use'].map((status,i)=>({id:String(i),status,date:today(1),hour:9}));db.bookings.push({status:'confirmed',date:today(-1),hour:9})`);
  assert.equal(run('upcomingReservations(db.bookings).length'),2);
  const html=run('studentHoursModule(db.bookings)');
  assert.match(html,/<span>未来预约<\/span><b class="" data-stat-value="2">2<\/b>/);
  assert.match(html,/<span>已完成记录<\/span><b class="" data-stat-value="1">1<\/b>/);
});

test('cancelled-only history shows zero upcoming and completed without deleting history',()=>{
  const {run,node}=setup();
  run(`db.bookings=[{id:'cancelled',userId:'student',status:'cancelled',date:today(),hour:9,deskNo:23}];me()`);
  const html=node('#content').innerHTML;
  assert.match(html,/<span>未来预约<\/span><b class="" data-stat-value="0">0<\/b>/);
  assert.match(html,/<span>已完成记录<\/span><b class="" data-stat-value="0">0<\/b>/);
  assert.match(html,/<span>未来预约<\/span><b>0 个时段<\/b>/);
  assert.match(html,/今天还没有预约/);
  assert.match(html,/已取消/);
  assert.match(html,/常用设备<\/span><b>暂未形成/);
  assert.equal(run('db.bookings.length'),1);
});

test('successful cancellation and refresh update both personal-page counters',async()=>{
  const {run,node}=setup();
  run(`db.bookings=[{id:'r',userId:'student',status:'confirmed',date:today(1),hour:9,deskNo:23}];me();supabase.rpc=async(name,args)=>{if(name!=='v2_cancel_reservation'||args.p_reservation_id!=='r')throw Error('Wrong cancellation request');return {error:null}};refreshCloud=async()=>{db.bookings[0].status='cancelled';me()}`);
  assert.match(node('#content').innerHTML,/<span>未来预约<\/span><b class="" data-stat-value="1">1<\/b>/);
  await run(`cancelCloud('r')`);
  assert.match(node('#content').innerHTML,/<span>未来预约<\/span><b class="" data-stat-value="0">0<\/b>/);
  assert.match(node('#content').innerHTML,/<span>未来预约<\/span><b>0 个时段<\/b>/);
  assert.match(node('#content').innerHTML,/已取消/);
});

test('rejected cancellation leaves the confirmed reservation counted',async()=>{
  const {run,node}=setup();
  run(`db.bookings=[{id:'r',userId:'student',status:'confirmed',date:today(1),hour:9,deskNo:23}];me();cloudError=()=> 'Rejected';supabase.rpc=async()=>({error:{message:'Rejected'}});refreshCloud=async()=>{throw Error('Must not refresh failed cancellation')}`);
  await run(`cancelCloud('r')`);
  assert.equal(run('db.bookings[0].status'),'confirmed');
  assert.match(node('#content').innerHTML,/<span>未来预约<\/span><b class="" data-stat-value="1">1<\/b>/);
});

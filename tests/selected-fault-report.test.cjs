// Run from the repository root: node --test tests/selected-fault-report.test.cjs
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
function setup(){
  const nodes=new Map();
  const node=selector=>{if(!nodes.has(selector))nodes.set(selector,{value:'',innerHTML:'',dataset:{},style:{},classList:{add(){},remove(){},toggle(){}},setAttribute(){},showModal(){this.open=true},addEventListener(){},focus(){}});return nodes.get(selector)};
  const context=vm.createContext({console,Date,Map,Set,Intl,Number,String,Math,JSON,localStorage:{getItem:()=> 'zh',setItem(){}},document:{querySelector:node,querySelectorAll:()=>[],addEventListener(){},body:{dataset:{},classList:{contains:()=>false}}},window:{addEventListener(){}},setTimeout:()=>1,clearTimeout(){},DESKS:Array.from({length:40},(_,i)=>i+1),SLOTS:[8,9,10,11,12,13,14,15,16],supabase:{},hourlyRange:slot=>String(slot).padStart(2,'0')+':00–'+String(Number(slot)+1).padStart(2,'0')+':00'});
  const source=fs.readFileSync(path.resolve(__dirname,'../web/app.js'),'utf8').replace(/^playSiteEntry\(\);$/gm,'').replace(/^import .*;\r?\n/gm,'').replace(/\nbootCloud\(\);\s*$/,'');
  vm.runInContext(source,context);
  const run=code=>vm.runInContext(code,context);
  run(`db={users:[{id:'student',name:'Test',role:'student'}],session:'student',devices:[],bookings:[],faults:[]};syncLanguage=()=>{};bindFaultForm=()=>{};toast=(text,type)=>{globalThis.lastToast={text,type}};`);
  return {run,node};
}
test('single selected device is automatically selected in the fault form',()=>{
  const {run,node}=setup();run('desks.add(23);reportSelectedDevice()');
  assert.equal(node('#faultDesk').value,'23');assert.equal(node('#faultModal').open,true);
});
test('changing the selected device updates the fault form device number',()=>{
  const {run,node}=setup();run('desks.add(7);reportSelectedDevice();desks.clear();desks.add(18);reportSelectedDevice()');
  assert.equal(node('#faultDesk').value,'18');
});
test('multiple devices show the required prompt and do not open the form',()=>{
  const {run,node}=setup();run('desks.add(10);desks.add(29);reportSelectedDevice()');
  assert.equal(run('lastToast.text'),'请选择单台设备报告故障');assert.equal(node('#faultModal').open,undefined);assert.equal(run('desks.size'),2);
});
test('no selected device cannot accidentally report device 01',()=>{
  const {run,node}=setup();run('reportSelectedDevice()');
  assert.equal(node('#faultModal').open,undefined);assert.equal(run('lastToast.type'),'error');
});
test('assistant heading displays the generic name in Chinese and English',()=>{
  const {run}=setup();run(`globalThis.heading='';head=(kicker,title)=>heading=title;agentPage()`);
  assert.equal(run('heading'),'预约助手');run(`language='en'`);assert.equal(run('tr(heading)'),'Booking assistant');
});
test('assistant renders the model reply without raw database details',()=>{
  const {run,node}=setup();run(`language='en';agentResponse={message:'A recommendation based on live database facts is ready',facts:{schedule:{failures:[{date:'2026-11-04',hour:8,reason:'slot_has_started'}],timezone:'Asia/Shanghai',contract:'internal'}},proposals:[]};agentPage()`);
  assert.match(node('#content').innerHTML,/A recommendation based on live database facts is ready/);assert.doesNotMatch(node('#content').innerHTML,/slot_has_started|timezone|internal/);
});
test('assistant starts with task cards and a complete composer',()=>{
  const {run,node}=setup();run(`agentResponse=null;agentPage()`);
  assert.match(node('#content').innerHTML,/agent-starters/);assert.match(node('#content').innerHTML,/assistant-orbit/);assert.match(node('#content').innerHTML,/class="agent-send"/);assert.match(run('agentPage.toString()'),/event\.key==="Enter"/);
});
test('assistant shows the DeepSeek balance message returned by the API',async()=>{
  const {run,node}=setup();run(`crypto={randomUUID:()=> 'request'};supabase.functions={invoke:async()=>({data:null,error:{context:{clone(){return this},json:async()=>({errors:[{message:'DeepSeek API balance is insufficient. Top up to continue.'}]})}}})}`);
  await run('askAgent()');assert.match(node('#content').innerHTML,/balance is insufficient/);
});
test('confirmed assistant reservation reuses the booking-success animation',async()=>{
  const {run}=setup();run(`agentResponse={proposals:[{proposalId:'proposal',candidate:{items:[{reservation_date:'2026-11-08',start_hour:9,device_numbers:[3]},{reservation_date:'2026-11-08',start_hour:10,device_numbers:[3]}]}}]};crypto={randomUUID:()=> 'request'};supabase.functions={invoke:async()=>({data:{outcome:'confirmed'}})};refreshCloud=async()=>true;showBookingSuccess=minutes=>globalThis.successMinutes=minutes;agentPage=()=>{}`);
  await run(`askAgent({proposalId:'proposal',idempotencyKey:'key'})`);
  assert.equal(run('successMinutes'),120);
});
test('the dashboard report button calls the selection-aware handler',()=>{
  const {run}=setup();assert.match(run('dashStudent.toString()'),/q\("#report"\)\.onclick=reportSelectedDevice/);
});

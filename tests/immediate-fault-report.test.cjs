// Run from the repository root: node --test tests/selected-fault-report.test.cjs
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
function setup(){
  const nodes=new Map();
  const node=selector=>{if(!nodes.has(selector))nodes.set(selector,{value:'',innerHTML:'',dataset:{},style:{},classList:{add(){},remove(){},toggle(){}},setAttribute(){},showModal(){this.open=true},close(){this.open=false},reset(){},addEventListener(){},focus(){}});return nodes.get(selector)};
  const context=vm.createContext({console,Date,Map,Set,Intl,Number,String,Math,JSON,localStorage:{getItem:()=> 'zh',setItem(){}},document:{querySelector:node,querySelectorAll:()=>[],addEventListener(){},body:{dataset:{},classList:{contains:()=>false}}},window:{addEventListener(){}},setTimeout:()=>1,clearTimeout(){},setInterval:()=>1,DESKS:Array.from({length:40},(_,i)=>i+1),SLOTS:[8,9,10,11,12,13,14,15,16],supabase:{}});
  const source=fs.readFileSync(path.resolve(__dirname,'../web/app.js'),'utf8').replace(/^playSiteEntry\(\);$/gm,'').replace(/^import .*;\r?\n/gm,'').replace(/\nbootCloud\(\);\s*$/,'');
  vm.runInContext(source,context);
  const run=code=>vm.runInContext(code,context);
  run(`db={users:[{id:'student',name:'Test',role:'student'}],session:'student',devices:[],bookings:[],faults:[]};syncLanguage=()=>{};bindFaultForm=()=>{};toast=(text,type)=>{globalThis.lastToast={text,type}};`);
  return {run,node};
}

test('student report commits a faulty report and disables and deselects the device immediately',async()=>{
 const {run,node}=setup();node('#faultText').value='Monitor has no signal';node('#faultDesk').value='23';
 run(`db.devices=[{number:23,status:'available'}];desks.add(23);render=()=>{};refreshCloud=async()=>false;
 supabase.from=table=>({insert(payload){globalThis.sentReport=payload;return this},select(){return this},single:async()=>({data:{id:'issue',reporter_id:'student',description:'Monitor has no signal',status:'faulty',created_at:'2026-11-03T01:00:00Z'},error:null})})`);
 await run('submitCloudIssue({preventDefault(){}})');
 assert.equal(run('sentReport.status'),'faulty');assert.equal(run('db.devices[0].status'),'faulty');assert.equal(run('desks.has(23)'),false);
 assert.equal(run('db.faults[0].status'),'faulty');assert.equal(node('#faultModal').open,false);assert.equal(node('#submitFault').disabled,false);
 assert.match(run(`map('student')`),/data-desk="23"[^>]*disabled/);
});
test('failed writes keep the form open and do not pretend the device is faulty',async()=>{
 const {run,node}=setup();node('#faultText').value='Monitor has no signal';node('#faultDesk').value='23';node('#faultModal').open=true;
 run(`db.devices=[{number:23,status:'available'}];cloudError=error=>error.message;
 supabase.from=()=>({insert(){return this},select(){return this},single:async()=>({data:null,error:{message:'write rejected'}})})`);
 await run('submitCloudIssue({preventDefault(){}})');assert.equal(run('db.devices[0].status'),'available');assert.equal(run('db.faults.length'),0);assert.equal(node('#faultModal').open,true);
});
test('an unresolved legacy report blocks selection and multiple reports count as one device',()=>{
 const {run}=setup();run(`db.faults=[{deskNo:23,status:'reported'},{deskNo:23,status:'faulty'}]`);
 assert.equal(run('!!fault(23)'),true);assert.equal(run('faultyDeviceCount()'),1);assert.match(run(`map('student')`),/data-desk="23"[^>]*disabled/);
});
test('a follow-up network error preserves the successfully submitted fault',async()=>{
 const {run,node}=setup();node('#faultText').value='Monitor has no signal';node('#faultDesk').value='23';
 run(`db.devices=[{number:23,status:'available'}];render=()=>{};refreshCloud=async()=>{throw new Error('offline')};
 supabase.from=()=>({insert(){return this},select(){return this},single:async()=>({data:{id:'issue',reporter_id:'student',description:'Monitor has no signal',status:'faulty',created_at:'2026-11-03T01:00:00Z'},error:null})})`);
 await run('submitCloudIssue({preventDefault(){}})');assert.equal(run('db.devices[0].status'),'faulty');assert.equal(run('db.faults.length'),1);assert.match(run('lastToast.text'),/设备已停止预约/);
});

test('AI fault card opens an editable form and writes only when the student submits',async()=>{
 const {run,node}=setup();
 run(`db.devices=[{number:23,status:'available'}];render=()=>{};refreshCloud=async()=>false;
 agentResponse={facts:{faultReport:{deviceNumber:23,description:'显示器无信号'}}};
 let writes=0;
 supabase.from=table=>({insert(payload){writes++;globalThis.sentReport=payload;return this},select(){return this},single:async()=>({data:{id:'ai-report',reporter_id:'student',description:'屏幕偶尔黑屏',status:'faulty'},error:null})});`);
 assert.match(run('agentActionMarkup(agentResponse,false)'),/data-agent-fault-report/);
 assert.doesNotMatch(run('agentActionMarkup(agentResponse,true)'),/data-agent-fault-report/);
 run('reviewAgentFaultReport()');
 assert.equal(node('#faultDesk').value,'23');
 assert.equal(node('#faultText').value,'显示器无信号');
 assert.equal(run('writes'),0);
 node('#faultText').value='屏幕偶尔黑屏';
 await run('submitCloudIssue({preventDefault(){}})');
 assert.equal(run('writes'),1);
 assert.equal(run('sentReport.reporter_id'),'student');
 assert.equal(run('sentReport.description'),'屏幕偶尔黑屏');
 assert.equal(run('db.devices[0].status'),'faulty');
 assert.equal(run('agentResponse'),null);
 assert.match(run('agentConversation("booking_planner").at(-1).message'),/已上报 23 号设备/);
});

test('a fault draft from another account cannot be submitted',async()=>{
 const {run,node}=setup();
 node('#faultModal').dataset.agentOwner='previous-student';
 run(`let writes=0;supabase.from=()=>{writes++;throw new Error('unexpected write')};`);
 await run('submitCloudIssue({preventDefault(){}})');
 assert.equal(run('writes'),0);
});

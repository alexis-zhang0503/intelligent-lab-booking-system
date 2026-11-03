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
  const source=fs.readFileSync(path.resolve(__dirname,'../app.js'),'utf8').replace(/^import .*;\n/gm,'').replace(/\nbootCloud\(\);\s*$/,'');
  vm.runInContext(source,context);
  const run=code=>vm.runInContext(code,context);
  run(`db={users:[{id:'student',name:'Test',role:'student'}],session:'student',devices:[],bookings:[],faults:[]};syncLanguage=()=>{};bindFaultForm=()=>{};toast=(text,type)=>{globalThis.lastToast={text,type}};`);
  return {run,node};
}

test('fault and repair notes accept empty and short text but reject over 120 characters',()=>{
 const {run,node}=setup();
 for(const value of ['', ' ', '坏']){node('#faultText').value=value;node('#repairText').value=value;assert.equal(run('validateFaultDescription()'),true);assert.equal(run('validateRepairDescription()'),true)}
 node('#faultText').value='a'.repeat(121);node('#repairText').value='a'.repeat(121);assert.equal(run('validateFaultDescription()'),false);assert.equal(run('validateRepairDescription()'),false);
 const html=fs.readFileSync(path.resolve(__dirname,'../index.html'),'utf8');for(const id of ['faultText','repairText'])assert.doesNotMatch(html.match(new RegExp('<textarea id="'+id+'"[^>]*>'))[0],/required|minlength/);
});
test('student can submit a blank fault and the device is still isolated',async()=>{
 const {run,node}=setup();node('#faultText').value='';node('#faultDesk').value='23';run(`db.devices=[{number:23,status:'available'}];render=()=>{};refreshCloud=async()=>true;supabase.from=()=>({insert(payload){globalThis.payload=payload;return this},select(){return this},single:async()=>({data:{id:'new',description:'',device_number:23,status:'faulty',reporter_id:'student'},error:null})})`);
 await run('submitCloudIssue({preventDefault(){}})');assert.equal(run('payload.description'),'');assert.equal(run('db.devices[0].status'),'faulty');
});
test('administrator can complete a repair without entering a note',async()=>{
 const {run,node}=setup();run(`db.users[0].role='admin';db.faults=[{id:'repair',deskNo:23,status:'faulty'}];supabase.rpc=async(name,args)=>{globalThis.request={name,args};return {error:null}};refreshCloud=async()=>true;faults=()=>{}`);node('#repairModal').dataset.issueId='repair';node('#repairText').value='';await run('submitRepair({preventDefault(){}})');assert.equal(run('request.args.p_note'),'');assert.equal(run('request.name'),'v2_complete_fault_repair');assert.equal(node('#repairModal').open,false);
});
test('removal requires an administrator, resolved timestamp, and a full 24 hours',()=>{
 const {run}=setup();run(`db.users[0].role='admin';globalThis.issue={id:'old',deskNo:23,status:'resolved',resolvedAt:new Date(Date.now()-86400000).toISOString()}`);assert.equal(run('canDeleteIssue(issue)'),true);run(`issue.resolvedAt=new Date(Date.now()-86399000).toISOString()`);assert.equal(run('canDeleteIssue(issue)'),false);run(`issue.resolvedAt=null`);assert.equal(run('canDeleteIssue(issue)'),false);run(`issue.resolvedAt=new Date(Date.now()-86400001).toISOString();issue.status='faulty'`);assert.equal(run('canDeleteIssue(issue)'),false);run(`issue.status='resolved';db.users[0].role='student'`);assert.equal(run('canDeleteIssue(issue)'),false);
});
test('only resolved records expose a gated delete button and empty reasons have a readable fallback',()=>{
 const {run}=setup();run(`db.users[0].role='admin'`);assert.match(run(`faultList([{id:'r',deskNo:23,status:'resolved',description:'',resolvedAt:new Date().toISOString(),createdAt:new Date().toISOString()}])`),/data-delete-issue="r" disabled/);assert.match(run(`faultList([{id:'r',deskNo:23,status:'faulty',description:'',createdAt:new Date().toISOString()}])`),/未填写故障说明/);assert.doesNotMatch(run(`faultList([{id:'r',deskNo:23,status:'faulty',createdAt:new Date().toISOString()}])`),/data-delete-issue/);
});
test('declined deletion does nothing; confirmed removal is persisted and stays removed if refresh fails',async()=>{
 const {run}=setup();run(`db.users[0].role='admin';db.faults=[{id:'r',deskNo:23,status:'resolved',resolvedAt:new Date(Date.now()-86400001).toISOString()}];window.confirm=()=>false;supabase.rpc=()=>{throw new Error('must not call')}`);await run(`deleteIssueRecord('r')`);assert.equal(run('db.faults.length'),1);run(`window.confirm=()=>true;supabase.rpc=async(name,args)=>{globalThis.request={name,args};return {error:null}};refreshCloud=async()=>false;render=()=>{}`);await run(`deleteIssueRecord('r')`);assert.equal(run('request.name'),'archive_resolved_issue');assert.equal(run('db.faults.length'),0);assert.match(run('lastToast.text'),/刷新失败/);
});
test('server rejection keeps a record visible',async()=>{
 const {run}=setup();run(`db.users[0].role='admin';db.faults=[{id:'r',deskNo:23,status:'resolved',resolvedAt:new Date(Date.now()-86400001).toISOString()}];window.confirm=()=>true;supabase.rpc=async()=>({error:{message:'Too early'}});cloudError=e=>e.message`);await run(`deleteIssueRecord('r')`);assert.equal(run('db.faults.length'),1);assert.equal(run('lastToast.text'),'Too early');
});

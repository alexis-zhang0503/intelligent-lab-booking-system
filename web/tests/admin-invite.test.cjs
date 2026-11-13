// Run: node --test tests/admin-invite.test.cjs
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
function setup(){
  const nodes=new Map();
  const node=selector=>{if(!nodes.has(selector))nodes.set(selector,{value:'',innerHTML:'',dataset:{},style:{},classList:{add(){},remove(){},toggle(){}},setAttribute(){},showModal(){this.open=true},close(){this.open=false},reset(){},addEventListener(){},focus(){}});return nodes.get(selector)};
  const context=vm.createContext({console,Date,Map,Set,Intl,Number,String,Math,JSON,localStorage:{getItem:()=> 'zh',setItem(){}},document:{querySelector:node,querySelectorAll:()=>[],addEventListener(){},body:{dataset:{},classList:{contains:()=>false}}},window:{addEventListener(){}},setTimeout:()=>1,clearTimeout(){},setInterval:()=>1,DESKS:Array.from({length:40},(_,i)=>i+1),SLOTS:[8,9,10,11,12,13,14,15,16],supabase:{}});
  const source=fs.readFileSync(path.resolve(__dirname,'../app.js'),'utf8').replace(/^playSiteEntry\(\);$/gm,'').replace(/^import .*;\r?\n/gm,'').replace(/\nbootCloud\(\);\s*$/,'');
  vm.runInContext(source,context);
  const run=code=>vm.runInContext(code,context);
  run(`db={users:[{id:'student',name:'Test',role:'student'}],session:'student',devices:[],bookings:[],faults:[]};syncLanguage=()=>{};bindFaultForm=()=>{};toast=(text,type)=>{globalThis.lastToast={text,type}};`);
  return {run,node};
}

function adminSetup(){const ctx=setup();ctx.run(`db.users[0].role='admin';cloudError=e=>e.message;window.confirm=()=>true`);ctx.node('#inviteDays').value='7';ctx.node('#inviteForm button.primary').textContent='生成邀请码';return ctx}
function submit(run){return run(`createAdministratorInvite({preventDefault(){},target:{querySelector:()=>q('#inviteForm button.primary')}})`)}
test('invites use server-side validity and render code with its returned expiry',async()=>{
 const {run,node}=adminSetup();run(`supabase.rpc=async(name,args)=>{globalThis.sent={name,args};return {data:[{invite_code:'NJUPT-ABCDEF123456',expires_at:new Date(Date.now()+604800000).toISOString()}],error:null}}`);await submit(run);assert.equal(run('sent.name'),'create_admin_invite_for_days');assert.equal(run('sent.args.p_valid_days'),7);assert.equal(node('#inviteCodeOutput').value,'NJUPT-ABCDEF123456');assert.equal(node('#inviteResult').hidden,false);assert.match(node('#inviteExpiry').textContent,/到期时间/);assert.equal(node('#inviteForm button.primary').disabled,false);
});
test('double submission creates only one request and controls recover after network failure',async()=>{
 const {run,node}=adminSetup();run(`globalThis.calls=0;supabase.rpc=()=>{calls++;return new Promise(resolve=>{globalThis.finish=resolve})}`);const first=submit(run);await submit(run);assert.equal(run('calls'),1);assert.equal(node('#inviteForm button.primary').disabled,true);run(`finish({data:null,error:{message:'Network failed'}})`);await first;assert.equal(node('#inviteForm button.primary').disabled,false);assert.equal(node('#inviteDays').disabled,false);assert.equal(run('inviteGenerationPending'),false);
});
test('empty/malformed responses and thrown errors produce errors instead of leaving generation stuck',async()=>{
 for(const result of [`({data:[],error:null})`,`({data:[{invite_code:'bad',expires_at:'bad'}],error:null})`]){const {run,node}=adminSetup();run(`supabase.rpc=async()=>${result}`);await submit(run);assert.match(run('lastToast.text'),/结果异常/);assert.equal(node('#inviteForm button.primary').disabled,false)}
 const {run,node}=adminSetup();run(`supabase.rpc=async()=>{throw new Error('offline')}`);await submit(run);assert.equal(run('lastToast.text'),'offline');assert.equal(node('#inviteForm button.primary').disabled,false);
});
test('invalid validity and non-admin calls never invoke generation',async()=>{
 const {run,node}=adminSetup();run(`supabase.rpc=()=>{throw new Error('must not call')}`);node('#inviteDays').value='999';await submit(run);assert.match(run('lastToast.text'),/有效期/);node('#inviteDays').value='7';run(`db.users[0].role='student'`);await submit(run);assert.equal(node('#inviteCodeOutput').value,'');
});
test('reopening preserves the visible invitation and replacing requires confirmation',async()=>{
 const {run,node}=adminSetup();node('#inviteCodeOutput').value='NJUPT-ABCDEF123456';run('openInviteDialog()');assert.equal(node('#inviteResult').hidden,false);assert.equal(node('#inviteCodeOutput').value,'NJUPT-ABCDEF123456');run(`window.confirm=()=>false;supabase.rpc=()=>{throw new Error('must not call')}`);await submit(run);assert.equal(node('#inviteCodeOutput').value,'NJUPT-ABCDEF123456');
});
test('a response after logout cannot expose an invitation in another session',async()=>{
 const {run,node}=adminSetup();run(`supabase.rpc=()=>new Promise(resolve=>{globalThis.finish=resolve})`);const first=submit(run);run(`db.session=null;finish({data:[{invite_code:'NJUPT-ABCDEF123456',expires_at:new Date().toISOString()}],error:null})`);await first;assert.equal(node('#inviteCodeOutput').value,'');
});

test('cached older HTML without the new expiry element can still display a generated code',async()=>{
 const {run,node}=adminSetup();run(`globalThis.originalQuery=document.querySelector;document.querySelector=selector=>selector==='#inviteExpiry'?null:originalQuery(selector);supabase.rpc=async()=>({data:[{invite_code:'NJUPT-ABCDEF123456',expires_at:new Date().toISOString()}],error:null})`);await submit(run);assert.equal(node('#inviteCodeOutput').value,'NJUPT-ABCDEF123456');assert.equal(node('#inviteResult').hidden,false);
});

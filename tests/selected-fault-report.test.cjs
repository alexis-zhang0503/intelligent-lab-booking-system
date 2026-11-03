// Run from the repository root: node --test tests/selected-fault-report.test.cjs
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
function setup(){
  const nodes=new Map();
  const node=selector=>{if(!nodes.has(selector))nodes.set(selector,{value:'',innerHTML:'',dataset:{},style:{},classList:{add(){},remove(){},toggle(){}},setAttribute(){},showModal(){this.open=true},addEventListener(){},focus(){}});return nodes.get(selector)};
  const context=vm.createContext({console,Date,Map,Set,Intl,Number,String,Math,JSON,localStorage:{getItem:()=> 'zh',setItem(){}},document:{querySelector:node,querySelectorAll:()=>[],addEventListener(){},body:{dataset:{},classList:{contains:()=>false}}},window:{addEventListener(){}},setTimeout:()=>1,clearTimeout(){},DESKS:Array.from({length:40},(_,i)=>i+1),SLOTS:[8,9,10,11,12,13,14,15,16],supabase:{}});
  const source=fs.readFileSync(path.resolve(__dirname,'../app.js'),'utf8').replace(/^playSiteEntry\(\);$/gm,'').replace(/^import .*;\n/gm,'').replace(/\nbootCloud\(\);\s*$/,'');
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
  assert.equal(run('heading'),'智能助手');run(`language='en'`);assert.equal(run('tr(heading)'),'AI Assistant');
});
test('the dashboard report button calls the selection-aware handler',()=>{
  const {run}=setup();assert.match(run('dashStudent.toString()'),/q\("#report"\)\.onclick=reportSelectedDevice/);
});

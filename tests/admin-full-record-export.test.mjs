import assert from 'node:assert/strict';
import fs from 'node:fs';
import {test} from 'node:test';
import vm from 'node:vm';

function setup(){
  const source=fs.readFileSync(new URL('../web/app.js',import.meta.url),'utf8')
    .replace(/^import .*;\r?\n/gm,'')
    .replace(/^playSiteEntry\(\);\r?\n/gm,'')
    .replace(/\r?\nbootCloud\(\);\s*$/,'');
  const nodes=new Map();
  const query=selector=>{
    if(!nodes.has(selector))nodes.set(selector,{innerHTML:'',classList:{},dataset:{}});
    return nodes.get(selector);
  };
  const context=vm.createContext({
    console:{warn(){}}, Date, Map, Set, Intl, Number, String, Math, JSON,
    DESKS:[1,2], hourlyRange:hour=>`${hour}:00–${hour+1}:00`,
    localStorage:{getItem:()=> 'zh'},
    document:{querySelector:query,querySelectorAll(){return []},addEventListener(){},body:{dataset:{}}},
    setTimeout(){return 1}, clearTimeout(){}, setInterval(){return 1}
  });
  vm.runInContext(`${source}
    globalThis.exportApi={
      assistant:administrator=>{
        language='zh';
        db={users:[{id:'test',role:administrator?'admin':'student'}],session:'test'};
        nav();
        const card=agentActionMarkup({proposals:[{actionType:'complete_fault_repair',proposalId:'repair',deviceNumber:1}],facts:{exports:['reservations','issues','invalid']}},administrator);
        return q('#nav').innerHTML+card;
      },
      assistantDownload:async(administrator,changeAccount=false)=>{
        db={users:[{id:'test',role:administrator?'admin':'student'}],session:'test',bookings:[],faults:[]};
        refreshCloud=async()=>{if(changeAccount)db.session='other';return true};
        download=(name,text)=>globalThis.downloaded={name,text};
        await downloadAgentExport('issues');
        return globalThis.downloaded;
      },
      dashboard:()=>{
        language='zh';page='dashboard';adminMetrics={capacity_data_complete:true,device_utilization:[],slot_usage_trends:[]};
        db={users:[{id:'admin',role:'admin'}],devices:[],bookings:[],faults:[],session:'admin'};
        syncLanguage=()=>{};dashAdmin();return q('#content').innerHTML;
      },
      reservations:()=>{
        language='zh';db={bookings:[
          {id:'old',date:'2026-10-01',hour:8,deskNo:1,userName:'A',status:'completed'},
          {id:'new',date:'2026-11-05',hour:9,deskNo:2,userName:'B',status:'confirmed'}
        ]};download=(name,text)=>globalThis.downloaded={name,text};reservationFile();return globalThis.downloaded;
      },
      issues:()=>{
        language='zh';db={faults:[{deskNo:2,status:'resolved',reporter:'A',description:'网络异常',createdAt:'2026-11-01T00:00:00Z',resolvedAt:'2026-11-02T00:00:00Z',resolver:'B',repairNote:'已更换网线'}]};
        download=(name,text)=>globalThis.downloaded={name,text};errorFile();return globalThis.downloaded;
      }
    };`,context);
  return context.exportApi;
}

test('overview keeps usage metrics full width and places exports in their record modules',()=>{
  const api=setup();
  const html=api.dashboard();
  assert.doesNotMatch(html,/admin-export/);
  assert.match(html,/data-export-reservations/);
  assert.match(html,/data-export-issues/);
  assert.doesNotMatch(html,/module-summary"><div><h2>[^<]+<\/h2><p class="sub">/);
});

test('reservation export includes records outside today',()=>{
  const api=setup();
  const file=api.reservations();
  assert.equal(file.name,'reservations.txt');
  assert.match(file.text,/2026-10-01/);
  assert.match(file.text,/2026-11-05/);
});

test('issue export includes its resolution history',()=>{
  const api=setup();
  const file=api.issues();
  assert.match(file.text,/已更换网线/);
  assert.match(file.text,/修复时间：/);
  assert.match(file.text,/处理人：B/);
});

test('AI Assistant shows repair confirmation and both downloads only to administrators',()=>{
  const api=setup();
  const html=api.assistant(true);
  assert.match(html,/AI 助手/);
  assert.doesNotMatch(html,/AI 决策/);
  assert.match(html,/data-confirm-proposal="repair"/);
  assert.match(html,/确认修复/);
  assert.match(html,/data-agent-export="reservations"/);
  assert.match(html,/data-agent-export="issues"/);
  assert.doesNotMatch(html,/invalid/);
  assert.doesNotMatch(api.assistant(false),/data-agent-export|data-confirm-proposal/);
});

test('the assistant downloads the existing TXT and refuses a role or account change',async()=>{
  assert.equal((await setup().assistantDownload(true)).name,'issues.txt');
  assert.equal(await setup().assistantDownload(false),undefined);
  assert.equal(await setup().assistantDownload(true,true),undefined);
});

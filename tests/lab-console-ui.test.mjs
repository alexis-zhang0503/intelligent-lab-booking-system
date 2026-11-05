import assert from 'node:assert/strict';
import fs from 'node:fs';
import {test} from 'node:test';
import vm from 'node:vm';

const html=fs.readFileSync(new URL('../index.html',import.meta.url),'utf8');
const css=fs.readFileSync(new URL('../lab-console.css',import.meta.url),'utf8');

test('light laboratory console is loaded after the existing interface layers',()=>{
  assert.match(html,/agent-assistant\.css[\s\S]*lab-console\.css/);
  assert.doesNotMatch(css,/color-scheme\s*:\s*dark/);
});

test('secondary engineering copy is visually removed without suppressing controls',()=>{
  assert.match(css,/#kicker,.eyebrow,.sub,.helper/);
  assert.match(css,/\.agent-starters button>small/);
  assert.match(css,/\.primary,.secondary,.outline/);
});

test('user profiles split administrators and students into independent scroll panels',()=>{
  const source=fs.readFileSync(new URL('../app.js',import.meta.url),'utf8').replace(/^import .*;\r?\n/gm,'').replace(/^playSiteEntry\(\);\r?\n/gm,'').replace(/\r?\nbootCloud\(\);\s*$/,'');
  const nodes=new Map(),query=selector=>{
    if(!nodes.has(selector))nodes.set(selector,{innerHTML:'',textContent:'',dataset:{}});
    return nodes.get(selector);
  };
  const context=vm.createContext({Date,Map,Set,Intl,Number,String,Math,JSON,DESKS:[],SLOTS:[],hourlyRange:hour=>`${hour}:00–${hour+1}:00`,localStorage:{getItem:()=> 'zh',setItem(){}},document:{querySelector:query,querySelectorAll(){return []},addEventListener(){},body:{dataset:{}}},setTimeout(){return 1},clearTimeout(){},setInterval(){return 1}});
  vm.runInContext(source,context);
  vm.runInContext(`db={session:'admin',devices:[],bookings:[],faults:[],users:[{id:'admin',name:'Admin',loginId:'admin@example.com',role:'admin'},{id:'student-a',name:'Student A',loginId:'B1',role:'student'},{id:'student-b',name:'Student B',loginId:'B2',role:'student'}]};syncLanguage=()=>{};users();`,context);
  const rendered=query('#content').innerHTML;
  assert.match(rendered,/user-directory-admin/);
  assert.match(rendered,/user-directory-student/);
  assert.equal((rendered.match(/user-directory-scroll/g)||[]).length,2);
  assert.match(rendered,/data-user-id="admin"/);
  assert.match(rendered,/data-user-id="student-a"/);
  assert.equal((rendered.match(/data-remove-user=/g)||[]).length,2);
  assert.match(vm.runInContext('beginProfileIdentityEdit.toString()',context),/row\.dataset\.userId/);
  assert.match(css,/\.user-directory-scroll\{height:/);
});

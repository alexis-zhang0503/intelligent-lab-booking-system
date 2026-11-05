import assert from 'node:assert/strict';
import fs from 'node:fs';
import {test} from 'node:test';
import vm from 'node:vm';

function loadI18n(){
  const source=fs.readFileSync(new URL('../app.js',import.meta.url),'utf8')
    .replace(/^import .*;\r?\n/gm,'')
    .replace(/^playSiteEntry\(\);\r?\n/gm,'')
    .replace(/\r?\nbootCloud\(\);\s*$/,'');
  const nodes=new Map();
  const query=selector=>{
    if(!nodes.has(selector))nodes.set(selector,{innerHTML:''});
    return nodes.get(selector);
  };
  const context=vm.createContext({
    console:{warn(){}}, Date, Map, Set, Intl, Number, String, Math, JSON, hourlyRange:hour=>`${hour}:00–${hour+1}:00`,
    localStorage:{getItem:()=> 'en'},
    document:{querySelector:query,querySelectorAll(){return []},addEventListener(){},body:{dataset:{}}},
    setTimeout(){return 1}, clearTimeout(){}, setInterval(){return 1}
  });
  vm.runInContext(`${source}\nglobalThis.i18n={translate:s=>{language='en';return tr(s)},minutes,candidate:items=>{language='en';return candidateMarkup(items)},loading:async()=>{language='en';let resolve,calls=0;supabase={rpc:async()=>calls++?{data:[],error:null}:new Promise(done=>resolve=done)};let target=q('#scheduleSuggestions');target.innerHTML='';let pending=loadScheduleCandidates(),markup=target.innerHTML;resolve({data:[],error:null});await pending;return markup}};`,context);
  return context.i18n;
}

test('administrator usage dashboard labels have English translations',()=>{
  const i18n=loadI18n();
  const labels=[
    '实际使用率','预约与使用率差异','爽约率','取消率','记录完整率',
    '实验室使用情况','按时间范围和设备查看使用情况。','刷新数据',
    '开始日期','结束日期','全部设备','查询','文件导出','下载运营记录',
    '导出今日预约和故障记录。','今日预约统计','按时段汇总预约情况',
    '故障记录','导出全部故障处理记录','预约与故障记录','查看完整记录并处理需要关注的事项。',
    '正在加载使用情况…','暂无设备数据','暂无时段数据','每台设备的利用',
    '使用时长、可用时长与利用率','使用时长','可用时长','利用率',
    '每个时间段的利用','按预约时段汇总','时间段','未解决故障',
    '导出完整预约记录','导出完整故障记录','完整故障记录',
    '导出全部预约状态与提交时间。','导出故障现象、处理状态与维修说明。'
  ];
  for(const label of labels){
    const translated=i18n.translate(label);
    assert.notEqual(translated,label,`${label} is missing an English translation`);
    assert.doesNotMatch(translated,/\p{Script=Han}/u,`${label} still contains Chinese`);
  }
});

test('English duration values do not append the Chinese minute unit',()=>{
  const i18n=loadI18n();
  assert.equal(i18n.minutes(12),'12.0 minutes');
});

test('asynchronously inserted recommendations render in English',async()=>{
  const i18n=loadI18n();
  assert.equal(await i18n.loading(),'<p class="sub">Calculating live options…</p>');
  assert.doesNotMatch(i18n.candidate(),/\p{Script=Han}/u);
  assert.doesNotMatch(i18n.candidate([{start_hour:8,device_numbers:[1],candidate_rank:1}]),/\p{Script=Han}/u);
});

const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');

const app=fs.readFileSync(path.resolve(__dirname,'../app.js'),'utf8');
const css=fs.readFileSync(path.resolve(__dirname,'../diorama-ui.css'),'utf8');

test('administrator metrics use their own responsive layout instead of booking controls',()=>{
  const dashboard=app.match(/function dashAdmin\(\)\{([\s\S]*?)\n\}/)?.[1]||'';
  assert.match(dashboard,/admin-metrics-layout/);
  assert.match(dashboard,/admin-metrics-filters/);
  assert.doesNotMatch(dashboard,/side-select/);
  assert.doesNotMatch(dashboard,/设备分钟|刷新 V3/);
  assert.match(app,/admin-metric-tables/);
  assert.match(app,/每台设备的利用/);
  assert.match(app,/每个时间段的利用/);
  assert.doesNotMatch(app.match(/function usageMetricDetails\([\s\S]*?\n\}/)?.[0]||'',/slice\(0,5\)/);
  assert.match(css,/\.admin-metrics-layout\{/);
  assert.match(css,/\.admin-metric-tables\{/);
  assert.match(css,/\.admin-export\{display:grid/);
});

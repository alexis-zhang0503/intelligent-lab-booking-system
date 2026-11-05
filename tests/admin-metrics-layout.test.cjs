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
  assert.match(app,/admin-metric-tables/);
  assert.match(css,/\.admin-metrics-layout\{/);
  assert.match(css,/\.admin-metric-tables\{/);
});

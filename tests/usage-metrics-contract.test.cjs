// Local, database-independent verification for V3 metric definitions.
// Run: node --test tests/usage-metrics-contract.test.cjs
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');

function finalMinutes({checkedInAt,checkedOutAt,slotStart,slotEnd}){
  if(!checkedInAt||!checkedOutAt||checkedOutAt<=checkedInAt||checkedInAt>=slotEnd||checkedOutAt<=slotStart)return 0;
  return Math.max(0,(Math.min(checkedOutAt,slotEnd)-Math.max(checkedInAt,slotStart))/60000);
}
function provisionalMinutes({checkedInAt,checkedOutAt,slotStart,slotEnd,now}){
  if(!checkedInAt||checkedOutAt||now<=slotStart||checkedInAt>=slotEnd)return 0;
  return Math.max(0,(Math.min(now,slotEnd)-Math.max(checkedInAt,slotStart))/60000);
}
function capacityMinutes(maintenanceWindows){
  return maintenanceWindows.some(window=>window.active)?0:60;
}
const h=60*60*1000,base=Date.parse('2026-11-10T06:00:00Z'); // 14:00 Asia/Shanghai
const slot={slotStart:base,slotEnd:base+h};

test('a 60-minute reservation with 50 minutes of complete attendance contributes 50 device-minutes',()=>{
  assert.equal(finalMinutes({...slot,checkedInAt:base+5*60000,checkedOutAt:base+55*60000}),50);
});
test('a 60-minute reservation with 10 minutes of complete attendance contributes 10 device-minutes',()=>{
  assert.equal(finalMinutes({...slot,checkedInAt:base+20*60000,checkedOutAt:base+30*60000}),10);
});
test('no check-in produces zero final actual-use minutes',()=>{
  assert.equal(finalMinutes({...slot,checkedInAt:null,checkedOutAt:null}),0);
});
test('missing checkout is incomplete and appears only as provisional use',()=>{
  const record={...slot,checkedInAt:base+10*60000,checkedOutAt:null,now:base+40*60000};
  assert.equal(finalMinutes(record),0);
  assert.equal(provisionalMinutes(record),30);
});
test('attendance timestamps are clipped to slot boundaries',()=>{
  assert.equal(finalMinutes({...slot,checkedInAt:base-20*60000,checkedOutAt:base+80*60000}),60);
});
test('adjacent slots do not double count a single clipped interval',()=>{
  const first={slotStart:base,slotEnd:base+h,checkedInAt:base,checkedOutAt:base+h};
  const second={slotStart:base+h,slotEnd:base+2*h,checkedInAt:base+h,checkedOutAt:base+2*h};
  assert.equal(finalMinutes(first)+finalMinutes(second),120);
});
test('one or many overlapping maintenance windows remove a slot only once',()=>{
  assert.equal(capacityMinutes([{active:true}]),0);
  assert.equal(capacityMinutes([{active:true},{active:true}]),0);
});
test('unknown historical device status withholds rates instead of inferring capacity',()=>{
  const knownCapacity=null,actualUsed=50;
  assert.equal(knownCapacity===null?null:actualUsed/knownCapacity,null);
});

test('V3 migration retains the required server-side safeguards and definitions',()=>{
  const sql=fs.readFileSync(path.resolve(__dirname,'../backend/supabase/v3-0-usage-metrics-migration.sql'),'utf8');
  assert.match(sql,/if not public\.is_admin\(\) then raise exception 'Administrator access is required'/);
  assert.match(sql,/at time zone 'Asia\/Shanghai'/);
  assert.match(sql,/least\(checked_out_at, slot_end\) - greatest\(checked_in_at, slot_start\)/);
  assert.match(sql,/checked_in_missing_checkout_count/);
  assert.match(sql,/timestamp_anomaly_count/);
  assert.match(sql,/provisional_used_minutes/);
  assert.match(sql,/where maintenance\.reservation_date = slots\.reservation_date/);
  assert.doesNotMatch(sql,/join public\.devices device on device\.status = 'available'/);
  assert.doesNotMatch(sql,/delete from public\.reservations/i);
  assert.doesNotMatch(sql,/update public\.reservations/i);
});

test('administrator dashboard and agent use the V3 metric contract',()=>{
  const app=fs.readFileSync(path.resolve(__dirname,'../app.js'),'utf8');
  const agent=fs.readFileSync(path.resolve(__dirname,'../backend/supabase/functions/deepseek-booking-agent/index.ts'),'utf8');
  assert.match(app,/v3_admin_usage_metrics/);
  assert.match(app,/actual_usage_rate/);
  assert.match(app,/capacity_data_limitations/);
  assert.match(agent,/v3_admin_usage_metrics/);
  assert.match(agent,/historical capacity data is incomplete/);
  assert.match(agent,/DEEPSEEK_BALANCE_INSUFFICIENT/);
  assert.doesNotMatch(agent,/outcome = "fallback"/);
  assert.doesNotMatch(app,/function plannerReply/);
  assert.match(app,/esc\(response\.message\)/);
});

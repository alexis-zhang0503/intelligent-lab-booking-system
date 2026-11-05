const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
function setup(){
 const nodes=new Map();const node=s=>{if(!nodes.has(s))nodes.set(s,{value:'',innerHTML:'',dataset:{},classList:{add(){},remove(){}},setAttribute(){}});return nodes.get(s)};
 const context=vm.createContext({console,Date,Map,Set,Intl,Number,String,Math,JSON,localStorage:{getItem:()=> 'zh'},document:{querySelector:node,querySelectorAll:()=>[],addEventListener(){},body:{dataset:{},classList:{contains:()=>false}}},window:{addEventListener(){},confirm:()=>true},setTimeout:()=>1,clearTimeout(){},setInterval:()=>1,DESKS:Array.from({length:40},(_,i)=>i+1),SLOTS:[8,9,10,11,12,13,14,15,16],supabase:{},hourlyRange:h=>`${String(h).padStart(2,'0')}:00–${String(h+1).padStart(2,'0')}:00`});
 vm.runInContext(fs.readFileSync(new URL('../app.js',`file://${__filename}`),'utf8').replace(/^playSiteEntry\(\);$/gm,'').replace(/^import .*;\r?\n/gm,'').replace(/\nbootCloud\(\);\s*$/,''),context);
 const run=code=>vm.runInContext(code,context);
 run(`db={users:[{id:'a',name:'Admin',role:'admin',loginId:'admin'},{id:'s',name:'Student <A>',role:'student',loginId:'B2501'}],session:'a',devices:[],faults:[],bookings:[{id:'r',userId:'s',deskNo:23,date:today(1),hour:9,userName:'Student <A>',status:'confirmed',createdAt:'2026-11-03T00:00:00Z'}]};syncLanguage=()=>{};toast=(text,type)=>{globalThis.lastToast={text,type}};render=()=>{globalThis.rendered=true};refreshCloud=async()=>true;cloudError=e=>e.message;`);
 return {run,node};
}
test('admin rows contain escaped owner, student ID, exact date, hour and cancellation action',()=>{
 const {run}=setup();let html=run('adminReservationRows()');assert.match(html,/Student &lt;A&gt;/);assert.match(html,/B2501/);assert.match(html,/23号设备/);assert.ok(html.includes(run('today(1)')));assert.match(html,/09:00–10:00/);assert.match(html,/data-admin-cancel="r"/);
});
test('date, status and owner/device searches combine and preserve legacy reserved rows',()=>{
 const {run}=setup();run(`db.bookings.push({...db.bookings[0],id:'old',status:'cancelled',date:today(-1)});adminBookingFilters={date:today(1),status:'confirmed',query:'b2501'};db.bookings[0].status='reserved'`);assert.equal(run('filteredAdminBookings().length'),1);run(`adminBookingFilters.query='23'`);assert.equal(run('filteredAdminBookings().length'),1);run(`adminBookingFilters.query='unmatched'`);assert.equal(run('filteredAdminBookings().length'),0);
});
test('device page limits reservation management to today while overview retains all records',()=>{
 const {run}=setup();run(`db.bookings=[{...db.bookings[0],id:'today',date:today()},{...db.bookings[0],id:'older',date:today(-1)}];adminBookingFilters={date:'',status:'',query:''};adminBookingScope='today'`);assert.deepEqual([...run('filteredAdminBookings()').map(item=>item.id)],['today']);run(`adminBookingScope='all'`);assert.equal(run('filteredAdminBookings().length'),2);
});
test('students cannot access actions; started, completed and admin-owned bookings have no force action',()=>{
 const {run}=setup();run(`db.session='s'`);assert.equal(run('canAdminCancel(db.bookings[0])'),false);run(`db.session='a';db.bookings[0].date=today(-1)`);assert.equal(run('canAdminCancel(db.bookings[0])'),false);run(`db.bookings[0].date=today(1);db.bookings[0].status='in_use'`);assert.equal(run('canAdminCancel(db.bookings[0])'),false);run(`db.bookings[0].status='completed'`);assert.equal(run('canAdminCancel(db.bookings[0])'),false);run(`db.bookings[0].status='confirmed';db.bookings[0].userId='a'`);assert.equal(run('canAdminCancel(db.bookings[0])'),false);
});
test('declining confirmation never calls reservation service',async()=>{
 const {run}=setup();run(`window.confirm=()=>false;supabase.rpc=()=>{throw new Error('must not call')}`);await run(`forceCancelStudentReservation('r')`);assert.equal(run('db.bookings[0].status'),'confirmed');
});
test('confirmed admin cancellation uses protected service and refreshes shared state',async()=>{
 const {run,node}=setup();run(`supabase.rpc=async(name,args)=>{globalThis.sent={name,args};return {error:null}}`);await run(`forceCancelStudentReservation('r',q('#button'))`);assert.equal(run('sent.name'),'v2_cancel_reservation');assert.equal(run('sent.args.p_reservation_id'),'r');assert.equal(run('db.bookings[0].status'),'cancelled');assert.equal(run('rendered'),true);assert.equal(node('#button').disabled,false);
});
test('rejected cancellation keeps original status; refresh failure preserves successful cancellation',async()=>{
 const {run}=setup();run(`supabase.rpc=async()=>({error:{message:'Permission denied'}})`);await run(`forceCancelStudentReservation('r')`);assert.equal(run('db.bookings[0].status'),'confirmed');assert.equal(run('lastToast.text'),'Permission denied');run(`supabase.rpc=async()=>({error:null});refreshCloud=async()=>false`);await run(`forceCancelStudentReservation('r')`);assert.equal(run('db.bookings[0].status'),'cancelled');assert.match(run('lastToast.text'),/刷新失败/);
});

import { DEVICE_NUMBERS as DESKS, HOURLY_SLOTS as SLOTS, hourToMinutes, hourlyRange } from "./web/core/hourlyRules.js";
import { supabase } from "./web/core/supabaseClient.js";

// Browser storage holds language preference only; reservation data stays in Supabase.
const LANGKEY="lab-booking-language";
const EN={"实验室预约":"Lab Reservation","把时间留给探索":"Make time for discovery","选定设备，约好时间。把注意力留给实验本身。":"Choose a device and time. Leave your attention for the work.","登录":"Sign in","注册":"Create account","开始预约":"Start a reservation","登录后，设备、时段和你的记录，都在这里。":"Your devices, times, and records are all here.","学号 / 工号":"Student / staff ID","密码":"Password","登录并继续":"Sign in and continue","创建账号":"Create your account","管理员和学生均通过注册获得账号。":"Students and administrators both register here.","姓名":"Name","身份":"Role","学生":"Student","管理员":"Administrator","注册并进入":"Create account and enter","退出":"Sign out","实验室管理":"Lab Operations","实验室总览":"Lab Overview","总览":"Overview","设备":"Devices","用户":"Users","统计":"Reports","故障":"Issues","预约":"Reserve","时长":"Hours","我的":"Me","预约流程":"Reservation flow","三步完成":"Three steps","选择设备":"Choose a device","设置时间":"Set the time","确认预约":"Confirm reservation","台实验设备":"lab devices","我的预约":"My reservations","实验室平面图":"Lab floor plan","点击空闲设备，可同时选择多个位置":"Select any available device. Multiple devices are supported.","预约日期":"Reservation date","空闲":"Available","已预约":"Reserved","故障设备":"Faulty devices","已选择":"Selected","预约时间":"Reservation time","设置开始与结束时间":"Set a start and end time","开始":"Start","结束":"End","至":"to","尚未选择设备":"No device selected","确认预约 →":"Confirm reservation →","实验室状态":"Lab status","台设备可预约":"devices available","报告设备故障":"Report a device issue","近期预约":"Recent reservations","完整预约记录已合并至“我的”":"Your complete reservation history is in Me.","在我的中查看":"View in Me","暂无记录。":"No records yet.","我的":"Me","今天的预约":"Today's reservations","今天还没有预约":"Nothing reserved for today","选好设备和起止时间后，会显示在这里。":"Choose a device and time, and it will appear here.","个预约时段":"reserved hours","去预约":"Reserve now","预约偏好":"Reservation preferences","根据你的实际预约自动汇总":"Automatically summarised from your reservations.","常用设备":"Most used device","常用时段":"Most used time","未来预约":"Upcoming reservations","取消后设备时段会立即释放。":"Cancelling releases the device and time immediately.","取消":"Cancel","注册用户":"Registered users","当前实验人数":"Current lab users","今日预约时段":"Reserved hours today","今日设备使用":"Today's device use","设备管理":"Manage devices","待处理故障":"Open issues","学生上报后由管理员确认":"Student reports are confirmed by an administrator.","故障维护":"Issue maintenance","本周使用趋势":"Weekly usage","管理员可在“统计与导出”中下载管理文件。":"Administrative files are available in Reports.","设备状态维护":"Device status","40 台设备平面图":"40-device floor plan","点击设备可切换正常 / 故障；故障设备无法被学生预约。":"Select a device to mark it available or faulty. Faulty devices cannot be reserved.","维护说明":"Maintenance notes","红色 ! 是故障设备。管理员可直接标记或恢复设备，学生上报会在故障维护中保留。":"A red ! marks a faulty device. Administrators can mark or restore devices here.","录入故障记录":"Add an issue record","用户档案":"User profiles","已注册用户":"Registered users","注册角色、账号和累计预约时长。":"Role, account, and total reserved hours.","累计时长":"Total hours","统计与导出":"Reports and exports","实验累计时间":"Lab hours","累计实验时长":"Total lab hours","已完成记录":"Completed records","时长明细":"Hours detail","每个已预约的一小时设备时段累计为一小时。":"Each reserved device hour counts as one hour.","累计使用时段":"Total reserved hours","本周使用时段":"Reserved hours this week","可用设备":"Available devices","一周使用情况":"Weekly usage","按预约时段统计":"By reserved hour","管理员权限":"Administrator access","管理文件导出":"Export management files","导出每日设备使用情况和故障维护记录。":"Export daily device use and issue-maintenance records.","下载 count.txt":"Download count.txt","下载 error.txt":"Download error.txt","设备故障上报":"Device issue report","全部故障记录":"All issue records","我的故障记录":"My issue reports","处理后设备会恢复可预约状态。":"When resolved, devices become available again.","提交后请等待管理员维护设备状态。":"After submitting, please wait for an administrator to update the device.","上报故障":"Report an issue","已处理":"Resolved","待处理":"Open","故障中":"Faulty","恢复":"Restore","电脑编号":"Device number","故障描述":"Issue description","例如：显示器无信号，无法开机":"For example: the monitor has no signal or the device will not start.","提交故障记录":"Submit issue report","关闭":"Close","正在进入实验室":"Entering the lab","预约已确认":"Reservation confirmed","设备与时间已为你保留":"Your device and time are reserved"};
Object.assign(EN,{"强制刷新状态":"Force refresh status","设备状态已更新。":"Device status updated.","设备状态已是最新。":"Device status is already current.","请至少选择一台设备。":"Select at least one device.","选择中存在故障或已预约设备。":"Your selection includes a faulty or reserved device.","NJUPT · 实验室预约":"NJUPT · Lab Reservation","NJUPT · 实验室管理":"NJUPT · Lab Operations","选择设备与起止时间　剩下的留给实验本身":"Choose a device and time. Leave the rest to the experiment.","暂无故障记录。":"No issue records yet.","预约时间已开始或今日预约已结束。":"This time has already started, or booking for today has closed.","管理员标记为故障。":"Marked faulty by an administrator.","管理员":"Administrator","故障已处理。":"Issue resolved.","设备已处于故障状态。":"This device is already marked as faulty.","故障已上报。":"Issue report submitted.","注册成功。":"Account created.","账号或密码不正确。":"Incorrect ID or password.","该账号已注册，请直接登录。":"This ID is already registered. Please sign in.","已下载 ":"Downloaded ","设备":"Device","设备总数：40 台":"Total devices: 40","开放时段：08:00-17:00（每 30 分钟）":"Hours: 08:00–17:00 (30-minute intervals)","智能实验室预约系统 - 每日使用情况统计":"Intelligent Lab Reservation System - Daily Usage","智能实验室预约系统 - 设备故障记录":"Intelligent Lab Reservation System - Device Issues","状态：":"Status: ","上报人：":"Reported by: ","描述：":"Description: ","账号操作":"Account action","注销账号":"Deactivate account","账号已注销。":"Account deactivated.","确认注销该学生账号吗？其预约将被删除，此操作不可恢复。":"Deactivate this student account? Their reservations will be deleted. This cannot be undone.","暂未形成":"Not established yet","预约已取消。":"Reservation cancelled.","今日预约已结束":"Booking closed for today","请选择未来日期继续预约":"Choose a future date to continue","智能实验室预约系统":"Intelligent Lab Reservation System","该设备已处于故障状态。":"This device is already marked as faulty.","每个已预约的半小时设备时段累计为半小时。":"Each reserved 30-minute device slot counts as half an hour."});
Object.assign(EN,{"开放时段：08:00-17:00（每小时）":"Hours: 08:00–17:00 (hourly)","每个已预约的一小时设备时段累计为一小时。":"Each reserved device hour counts as one hour.","创建学生账号":"Create a student account","学生可直接注册。管理员账号由项目维护者在后台开通。":"Students can register directly. Administrator accounts are provisioned by the project maintainer.","学号":"Student ID","学号 / 管理员邮箱":"Student ID / administrator email"});
Object.assign(EN,{"学生可直接注册。管理员账号需使用邀请码。":"Students can register directly. Administrator accounts require an invitation code.","使用邀请码注册管理员":"Register an administrator with an invitation","管理员注册":"Administrator registration","输入管理员邀请码后即可创建管理员账号":"Enter an administrator invitation code to create an administrator account","管理员邮箱":"Administrator email","管理员邀请码":"Administrator invitation code","注册管理员并进入":"Create administrator account and enter","返回学生注册":"Back to student registration","生成管理员邀请码":"Generate administrator invitation","邀请码仅显示一次。请通过可信方式发送给新管理员":"The code is shown once. Send it to the new administrator through a trusted channel.","有效期":"Valid for","1 天":"1 day","7 天":"7 days","30 天":"30 days","生成邀请码":"Generate invitation","邀请码":"Invitation code","复制邀请码":"Copy invitation code","邀请码已复制。":"Invitation code copied.","名称已更新。":"Name updated."});
Object.assign(EN,{"故障现象或原因":"Fault symptom or cause","例如：显示器无信号、无法开机或网络无法连接":"For example: no monitor signal, device will not start, or network unavailable"});
Object.assign(EN,{"请描述你实际看到的故障现象，或你认为可能的原因":"Describe what you observed or the cause you suspect","写下设备表现，管理员会据此定位和处理问题":"Describe the device behaviour so an administrator can diagnose and resolve it","常见故障现象":"Common symptoms","显示器无信号":"No monitor signal","无法开机":"Device will not start","网络无法连接":"Network unavailable","故障说明至少需要 8 个字符。":"Describe the issue in at least 8 characters.","故障说明":"Issue description","上报人":"Reported by","上报时间":"Reported","维修收件箱":"Maintenance inbox","需要关注的设备":"Devices needing attention","已处理":"Resolved","待处理":"Open","故障中":"Faulty","移动到故障设备上可快速查看故障现象或原因":"Hover a faulty device to preview its symptom or cause","设备状态和维修记录在这里同步":"Device state and maintenance records stay in sync here","全部故障记录":"All issue records","上报设备故障":"Report a device issue","故障设备":"Faulty devices","故障现象或原因":"Fault symptom or cause","选择设备并填写故障现象，再将它标记为故障":"Choose a device, describe the symptom, then mark it faulty","设备状态":"Device status","当前故障":"Current issues","选择可用设备以创建故障记录；选择红色设备即可恢复":"Choose an available device to create an issue record; select a red device to restore it","标记设备故障":"Mark device as faulty","设备已恢复可预约":"Device restored and available","没有待处理的故障记录":"No open issue records","设备状态和维修记录会在这里同步":"Device status and maintenance records stay in sync here","最近故障记录":"Recent issue records","恢复可预约":"Restore availability","点击可用设备开始填写故障说明":"Select an available device to describe the issue","点击红色设备恢复可预约":"Select a red device to restore availability"});
const ZH=Object.fromEntries(Object.entries(EN).map(([zh,en])=>[en,zh]));
let language=localStorage.getItem(LANGKEY)||"en", db={users:[],devices:[],bookings:[],faults:[],session:null}, page="dashboard", date=today(), hours=new Set([8]), desks=new Set(), startHour=8, endHour=9;
const q=s=>document.querySelector(s), qa=s=>[...document.querySelectorAll(s)];
function today(n=0){let d=new Date();d.setDate(d.getDate()+n);return d.toLocaleDateString("en-CA")}
function user(){return db.users.find(x=>x.id===db.session)} function admin(){return user()?.role==="admin"}
function id(p){return p+"-"+Date.now()+"-"+Math.random().toString(16).slice(2)}
function esc(x){return String(x).replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#039;"}[c]))}
function tr(s){return language==="en"?(EN[s]||s):(ZH[s]||s)}
function fd(d){return new Intl.DateTimeFormat(language==="en"?"en-US":"zh-CN",{month:"short",day:"numeric",weekday:"short"}).format(new Date(d+"T00:00:00"))}
/** Converts an hourly start value to minutes after midnight for Date comparisons. */
function slotStart(value){return hourToMinutes(value)}
function clockAt(minutes){return String(Math.floor(minutes/60)).padStart(2,"0")+":"+String(minutes%60).padStart(2,"0")}
/** Formats one assignment-defined booking slot. */
function fh(slot){return hourlyRange(slot)}
/** Formats the currently selected one-hour interval. */
function timeRange(){return hourlyRange(startHour)}
/** Counts only finished, non-cancelled reservations toward accumulated experiment time. */
function durationMinutes(list){return list.reduce((sum,booking)=>sum+(booking.status!=="cancelled"&&bookingEnd(booking)<=Date.now()?60:0),0)}
function durationLabel(list){let hours=durationMinutes(list)/60;return `${Number.isInteger(hours)?hours:hours.toFixed(1)}h`}
function locale(){return language==="en"?"en-US":"zh-CN"}
function timesLabel(count){return language==="en"?`${count} time${count===1?"":"s"}`:`${count} 次`}
function slotsLabel(count){return language==="en"?`${count} reservation slot${count===1?"":"s"}`:`${count} 个时段`}
function deviceLabel(number){return language==="en"?`Device ${String(number).padStart(2,"0")}`:`#${String(number).padStart(2,"0")} 设备`}
function systemFaultText(text){return tr(text)}
function toast(m,type=""){let e=q("#toast");e.textContent=tr(m);e.className="show "+type;clearTimeout(toast.t);toast.t=setTimeout(()=>e.className="",2500)}
function translateNode(node){let raw=node.nodeValue,trim=raw.trim();if(!trim)return;let value=tr(trim);if(value!==trim)node.nodeValue=raw.replace(trim,value)}
function syncLanguage(){document.documentElement.lang=language==="en"?"en":"zh-CN";document.title=tr("智能实验室预约系统");let walker=document.createTreeWalker(document.body,NodeFilter.SHOW_TEXT);for(let node;node=walker.nextNode();)if(!node.parentElement?.closest("script,style"))translateNode(node);qa("[placeholder],[aria-label]").forEach(el=>{for(let name of ["placeholder","aria-label"]){let value=el.getAttribute(name);if(value)el.setAttribute(name,tr(value))}});qa("[data-language]").forEach(button=>button.classList.toggle("active",button.dataset.language===language))}
function setLanguage(next){language=next;localStorage.setItem(LANGKEY,language);render()}
/** Returns the exact local end instant for a one-hour reservation. */
function bookingEnd(booking){let start=slotStart(booking.hour);return new Date(`${booking.date}T00:00:00`).getTime()+(start+60)*60000}
function refreshBookingStates(){let now=Date.now(),changed=false;db.bookings.forEach(booking=>{if(bookingEnd(booking)<=now&&booking.status!=="cancelled"&&booking.status!=="completed"){booking.status="completed";changed=true}});return changed}
function scheduleStatusRefresh(){clearTimeout(scheduleStatusRefresh.timer);let now=Date.now(),next=Math.min(...db.bookings.filter(booking=>booking.status!=="completed"&&bookingEnd(booking)>now).map(booking=>bookingEnd(booking)));if(Number.isFinite(next))scheduleStatusRefresh.timer=setTimeout(()=>{let changed=refreshBookingStates();if(changed&&user())render();else scheduleStatusRefresh()},Math.max(250,next-Date.now()+80))}
function fault(n){return db.devices?.find(x=>x.number===n&&x.status==="faulty")||db.faults.find(x=>x.deskNo===n&&x.status==="faulty")}
function at(n,d=date){return db.bookings.filter(x=>x.deskNo===n&&x.date===d)}
/** Returns the single hourly value occupied by this reservation. */
function bookingSlots(booking){return [+booking.hour]}
function conflict(n){refreshBookingStates();return [...hours].some(slot=>db.bookings.some(x=>x.deskNo===n&&x.date===date&&bookingSlots(x).includes(slot)&&x.status!=="completed"))}
/** Calculates the first bookable full hour for the selected date. */
function bookingWindow(){let now=new Date(),sameDay=date===today(),nextStart=sameDay?Math.max(8,now.getHours()+(now.getMinutes()||now.getSeconds()?1:0)):8,closed=nextStart>16;return {sameDay,nextStart,closed,valid:!closed&&startHour>=nextStart&&startHour<=16&&endHour===startHour+1,now}}
/** Keeps the UI state constrained to one whole assignment-defined hour. */
function syncBookingRange(){let state=bookingWindow();if(state.closed){hours.clear();return state}if(startHour<state.nextStart||startHour>16)startHour=state.nextStart;endHour=startHour+1;hours=new Set([startHour]);return bookingWindow()}
function liveClock(d=new Date()){return new Intl.DateTimeFormat(language==="en"?"en-US":"zh-CN",{hour:"2-digit",minute:"2-digit",hour12:false}).format(d)}
function scheduleLiveTimeCheck(){clearTimeout(scheduleLiveTimeCheck.timer);if(admin()||page!=="dashboard"||date!==today())return;let delay=60000-Date.now()%60000+80;scheduleLiveTimeCheck.timer=setTimeout(()=>{if(document.body.classList.contains("booking-success")){scheduleLiveTimeCheck();return}dashStudent()},delay)}
function render(keepAuth=false){refreshBookingStates();if(!user()){q("#auth").hidden=false;q("#app").hidden=true;syncLanguage();return}q("#auth").hidden=!keepAuth;q("#app").hidden=false;q("#profile").innerHTML=`<div class="avatar">${esc(user().name[0])}</div><div><b>${esc(user().name)}</b><span>${tr(admin()?"管理员":"学生")} · ${esc(user().loginId)}</span></div>`;nav();({dashboard:admin()?dashAdmin:dashStudent,lab,users,reports,faults,me})[page]?.();let content=q("#content");content.classList.remove("page-enter");void content.offsetWidth;content.classList.add("page-enter");syncLanguage();scheduleStatusRefresh()}
function startSession(){render(true);let reduce=matchMedia("(prefers-reduced-motion: reduce)").matches,overlay=q("#entryOverlay");if(reduce){q("#auth").hidden=true;return}overlay.hidden=false;overlay.setAttribute("aria-hidden","false");document.body.classList.add("launching");requestAnimationFrame(()=>document.body.classList.add("launching-ready"));setTimeout(()=>{q("#auth").hidden=true;overlay.hidden=true;overlay.setAttribute("aria-hidden","true");document.body.classList.remove("launching","launching-ready")},2000)}
function nav(){let list=admin()?[["dashboard","01","总览"],["lab","02","设备"],["users","03","用户"],["reports","04","统计"],["faults","05","故障"]]:[["dashboard","01","预约"],["reports","02","时长"],["faults","03","故障"],["me","04","我的"]];q("#nav").innerHTML=list.map(x=>`<button class="nav-btn ${page===x[0]?"active":""}" data-p="${x[0]}"><i class="nav-code">${x[1]}</i>${tr(x[2])}</button>`).join("");qa("[data-p]").forEach(b=>b.onclick=()=>{page=b.dataset.p;render()})}
function head(k,t){document.body.dataset.role=admin()?"admin":"student";q("#kicker").textContent=tr(admin()?"NJUPT · 实验室管理":"NJUPT · 实验室预约");q("#title").textContent=tr(t)}
function stats(a){return `<div class="stats">${a.map(x=>`<article class="stat"><span>${tr(x[0])}</span><b class="${x[2]||""}">${x[1]}</b></article>`).join("")}</div>`}
/** Renders forty devices with explicit available (*), reserved (#), and faulty (!) symbols. */
function map(mode){let groups=Array.from({length:20},(_,i)=>[i*2+1,i*2+2]);return `<div class="lab">${groups.map(g=>`<div class="group">${g.map(n=>{let f=fault(n),c=conflict(n),blocked=mode==="student"&&(f||c),s=desks.has(n)&&!blocked,cl=["desk",blocked?"unavailable":"",f?"faulty":"",!f&&c?"occupied":"",s?"selected":""].filter(Boolean).join(" "),status=f?"故障":c?"已预约":s?"已选择":"空闲",symbol=f?"!":c?"#":s?"●":"*",label=language==="en"?`Device ${n}, ${tr(status)}`:`${n}号设备，${status}`;return `<button class="${cl}" data-desk="${n}" aria-label="${label}" ${blocked?"disabled":""}><b>${String(n).padStart(2,"0")}</b><i aria-hidden="true">${symbol}</i></button>`}).join("")}</div>`).join("")}</div>`}
/** Renders the fixed nine one-hour choices from 08:00 through 17:00. */
function timePick(){let state=bookingWindow(),isEn=language==="en",label=isEn?"Reservation time":"预约时间";if(state.closed)return `<div class="time-picker"><h3>${label}</h3><div class="time-check blocked"><div><b>${isEn?"Booking unavailable today":"今日无法预约"}</b><span>${isEn?"Choose another date to continue":"请选择其他日期继续预约"}</span></div></div></div>`;return `<div class="time-picker"><h3>${label}</h3><div class="time-range"><label><span>${isEn?"Start":"开始"}</span><select id="startTime">${SLOTS.map(slot=>`<option value="${slot}" ${slot===startHour?"selected":""} ${slot<state.nextStart?"disabled":""}>${hourlyRange(slot)}</option>`).join("")}</select></label><i>${isEn?"to":"至"}</i><label><span>${isEn?"End":"结束"}</span><select id="endTime" disabled><option>${hourlyRange(startHour)}</option></select></label></div></div>`}
function selection(){let ds=[...desks].sort((a,b)=>a-b);return ds.length?`${ds.map(n=>"#"+String(n).padStart(2,"0")).join(" ")} · ${timeRange()}`:tr("尚未选择设备")}
function pruneSelections(){for(let n of desks)if(fault(n)||conflict(n))desks.delete(n)}
/** Renders the student flow, including a legend that never hides fault status. */
function dashStudent(){syncBookingRange();pruneSelections();head("STUDENT / RESERVATION","实验室预约");let mine=db.bookings.filter(x=>x.userId===user().id),available=DESKS.filter(n=>!fault(n)&&!conflict(n)).length,faulty=db.faults.filter(x=>x.status==="faulty").length,canBook=bookingWindow().valid;q("#content").innerHTML=`<section class="booking-intro"><div class="booking-copy"><p class="eyebrow">${fd(date)}</p><h2>把时间留给探索</h2><p>选择设备与起止时间　剩下的留给实验本身</p><div class="booking-summary"><span><b>40</b> 台实验设备</span><span><b>${mine.length}</b> 我的预约</span></div></div><section class="reservation-guide"><p class="eyebrow">预约流程</p><h3>三步完成</h3><ol><li><b>01</b>选择设备</li><li><b>02</b>设置时间</li><li><b>03</b>确认预约</li></ol></section></section><div class="booking"><section class="panel lab-panel"><div class="panel-head"><div><h2>实验室平面图</h2><p class="sub">点击空闲设备，可同时选择多个位置</p></div><label class="booking-date"><span>预约日期</span><input id="date" class="date" type="date" min="${today()}" value="${date}"></label></div><div class="legend"><span><i></i>* 空闲</span><span><i class="occ"></i># 已预约</span><span><i class="bad"></i>! 故障</span><span><i class="pick"></i>已选择</span></div>${map("student")}</section><aside class="side-select"><div class="panel booking-flow">${timePick()}<div class="selection compact-selection">${selection()}</div><button id="book" class="primary" ${canBook?"":"disabled"}>确认预约 <span>→</span></button></div><section class="panel lab-status"><p class="eyebrow">实验室状态</p><b>${available}<span> / 40</span></b><p>台设备可预约</p><div><span>故障设备</span><strong>${faulty}${language==="en"?" devices":" 台"}</strong></div></section><button id="report" class="secondary">报告设备故障</button></aside></div><section class="panel recent-panel schedule-panel"><div class="section-head"><div><h2>近期预约</h2><p class="sub">完整预约记录已合并至“我的”</p></div><button data-go="me" class="secondary small-btn">在我的中查看</button></div>${records(mine.slice(-3).reverse(),true)}</section>`;q("#date").onchange=e=>{date=e.target.value;desks.clear();dashStudent()};bindTimes();bindDesks("student");q("#book").onclick=book;q("#report").onclick=openModal;go();syncLanguage();scheduleLiveTimeCheck()}
/** Binds the single start selector; the end value is always exactly one hour later. */
function bindTimes(){let start=q("#startTime");if(!start)return;start.onchange=e=>{startHour=+e.target.value;syncBookingRange();dashStudent()}}
/** Handles administrator fault toggles and student multi-device selection. */
function bindDesks(mode){qa("[data-desk]").forEach(e=>e.onclick=()=>{let n=+e.dataset.desk;if(mode==="admin"){if(fault(n))restoreDevice(n);else openModal(n);return}desks.has(n)?desks.delete(n):desks.add(n);let selected=desks.has(n);e.classList.toggle("selected",selected);e.querySelector("i").textContent=selected?"●":"*";e.setAttribute("aria-label",language==="en"?`Device ${n}, ${selected?"Selected":"Available"}`:`${n}号设备，${selected?"已选择":"空闲"}`);q(".compact-selection").textContent=selection()})}
/** Shows the fixed two-second confirmation after a successful hourly booking. */
function showBookingSuccess(durationMinutes){let overlay=q("#bookingOverlay"),reduce=matchMedia("(prefers-reduced-motion: reduce)").matches;q("#bookingOverlayCopy").textContent=language==="en"?`Your ${durationMinutes}-minute reservation is ready`:`已为你保留 ${durationMinutes} 分钟`;if(reduce){dashStudent();toast("Reservation confirmed");return}overlay.hidden=false;overlay.setAttribute("aria-hidden","false");document.body.classList.add("booking-success");void overlay.offsetWidth;document.body.classList.add("booking-success-ready");setTimeout(()=>{overlay.hidden=true;overlay.setAttribute("aria-hidden","true");document.body.classList.remove("booking-success","booking-success-ready");dashStudent();toast("Reservation confirmed")},2000)}
/** Validates every selected device immediately before storing the one-hour reservation. */
function records(a,cancel=false){return a.length?`<div class="records">${a.map(x=>`<div class="record"><div><b>#${String(x.deskNo).padStart(2,"0")} · ${fh(x.hour)}</b><small>${fd(x.date)}${x.userName?" · "+esc(x.userName):""}</small></div>${cancel?`<button class="secondary small-btn" data-cancel="${x.id}">${tr("取消")}</button>`:`<span class="tag">${tr("已预约")}</span>`}</div>`).join("")}</div>`:`<p class="empty">${tr("暂无记录。")}</p>`}
function favorite(a,key,label){let counts={};a.forEach(x=>counts[x[key]]=(counts[x[key]]||0)+1);let top=Object.entries(counts).sort((a,b)=>b[1]-a[1])[0];return top?label(top[0],top[1]):tr("暂未形成")}
function me(){head("MY PROFILE","我的");let mine=db.bookings.filter(x=>x.userId===user().id),todayMine=mine.filter(x=>x.date===today()).sort((a,b)=>a.hour-b.hour),future=mine.filter(x=>x.date>=today()).sort((a,b)=>(a.date+a.hour).localeCompare(b.date+b.hour)),favoriteDesk=favorite(mine,"deskNo",(n,c)=>"#"+String(n).padStart(2,"0")+" · "+timesLabel(c)),favoriteHour=favorite(mine,"hour",(slot,c)=>fh(slot).split("–")[0]+" · "+timesLabel(c));q("#content").innerHTML=`<section class="profile-hero"><div class="profile-mark">${esc(user().name[0])}</div><div><h2>${esc(user().name)}</h2><p>${tr(admin()?"管理员":"学生")} · ${esc(user().loginId)}</p></div></section><section class="today-spotlight"><div><p class="eyebrow">${fd(today())}</p><h2>${todayMine.length?"今天的预约":"今天还没有预约"}</h2><p>${todayMine.length?todayMine.map(x=>"#"+String(x.deskNo).padStart(2,"0")+" · "+fh(x.hour)).join("　"):"选好设备和起止时间后，会显示在这里。"}</p></div><div class="today-number"><b>${todayMine.length}</b><span>${slotsLabel(todayMine.length)}</span></div>${todayMine.length?"":`<button data-go="dashboard" class="secondary small-btn">去预约</button>`}</section><div class="profile-grid"><section class="panel preference-panel"><h2>预约偏好</h2><p class="sub">根据你的实际预约自动汇总</p><div class="preference-list"><div><span>常用设备</span><b>${favoriteDesk}</b></div><div><span>常用时段</span><b>${favoriteHour}</b></div><div><span>未来预约</span><b>${slotsLabel(future.length)}</b></div></div></section><section class="panel my-bookings"><h2>我的预约</h2><p class="sub">取消后设备时段会立即释放。</p>${records(mine.slice().sort((a,b)=>(a.date+a.hour).localeCompare(b.date+b.hour)),true)}</section></div>`;go()}
/** Renders administrator metrics using the actual current full-hour slot. */
function dashAdmin(){head("ADMIN / OPERATIONS","实验室总览");let t=db.bookings.filter(x=>x.date===today()),now=new Date(),nowSlot=now.getHours();q("#content").innerHTML=stats([["注册用户",db.users.length],["当前实验人数",t.filter(x=>bookingSlots(x).includes(nowSlot)&&x.status==="reserved").length,"cyan"],["今日预约时段",t.length],["故障设备",db.faults.filter(x=>x.status==="faulty").length,"warn"]])+`<div class="grid2"><section class="panel"><div class="section-head"><div><h2>今日设备使用</h2><p class="sub">${fd(today())}</p></div><button data-go="lab" class="secondary small-btn">设备管理</button></div>${records(t.slice(-8).reverse())}</section><section class="panel"><div class="section-head"><div><h2>待处理故障</h2><p class="sub">学生上报后由管理员确认</p></div><button data-go="faults" class="secondary small-btn">故障维护</button></div>${faultList(db.faults.filter(x=>x.status==="reported"))}</section></div><section class="panel" style="margin-top:1rem"><h2>本周使用趋势</h2><p class="sub">管理员可在“统计与导出”中下载管理文件。</p>${chart()}</section>`;go()}
function lab(){let active=db.faults.filter(issue=>issue.status!=="resolved").slice().reverse(),faulty=db.devices.filter(device=>device.status==="faulty").length;head("ADMIN / DEVICE STATUS","设备状态维护");q("#content").innerHTML=`<section class="device-overview"><div><p class="eyebrow">DEVICE OPERATIONS</p><h2>选择设备并填写故障现象，再将它标记为故障</h2><p>设备状态和维修记录会在这里同步</p></div><div class="device-overview-metric"><b>${faulty}</b><span>当前故障</span></div></section><div class="device-layout"><section class="panel device-plan"><div class="section-head"><div><p class="eyebrow">LIVE FLOOR PLAN</p><h2>40 台设备平面图</h2><p class="sub">点击可用设备开始填写故障说明</p></div><div class="device-legend"><span><i></i>空闲</span><span><i class="bad"></i>故障</span></div></div>${map("admin")}</section><aside class="device-console"><section class="panel device-actions"><p class="eyebrow">MAINTENANCE ACTION</p><h2>设备状态</h2><p>选择可用设备以创建故障记录；选择红色设备即可恢复</p><button id="report" class="primary wide">标记设备故障 <span>→</span></button><button id="refreshStatus" class="secondary wide">强制刷新状态</button></section><section class="panel device-current"><div class="section-head"><div><p class="eyebrow">OPEN RECORDS</p><h2>最近故障记录</h2></div><span class="tag ${active.length?"bad":"ok"}">${active.length?active.length:"0"}</span></div>${faultList(active.slice(0,3))}</section></aside></div>`;bindDesks("admin");q("#refreshStatus").onclick=forceRefresh;q("#report").onclick=()=>openModal();qa("[data-fix]").forEach(button=>button.onclick=()=>fix(button.dataset.fix));syncLanguage()}
function users(){head("ADMIN / USERS","用户档案");q("#content").innerHTML=`<section class="panel"><h2>已注册用户</h2><p class="sub">注册角色、账号和累计预约时长。</p><div class="table-wrap"><table class="table"><thead><tr><th>姓名</th><th>身份</th><th>学号 / 工号</th><th>累计时长</th><th>账号操作</th></tr></thead><tbody>${db.users.map(x=>`<tr><td>${esc(x.name)}</td><td><span class="tag">${x.role==="admin"?"管理员":"学生"}</span></td><td>${esc(x.loginId)}</td><td>${durationLabel(db.bookings.filter(b=>b.userId===x.id))}</td><td>${x.role==="student"?`<button class="secondary small-btn danger-btn" data-remove-user="${x.id}">注销账号</button>`:"—"}</td></tr>`).join("")}</tbody></table></div></section>`;qa("[data-remove-user]").forEach(button=>button.onclick=()=>removeStudent(button.dataset.removeUser));syncLanguage()}
function reports(){head(admin()?"ADMIN / REPORTS":"STUDENT / HOURS",admin()?"统计与导出":"实验累计时间");if(!admin()){let a=db.bookings.filter(x=>x.userId===user().id),future=a.filter(x=>x.date>=today()).length;q("#content").innerHTML=stats([["累计实验时长",durationLabel(a),"cyan"],["未来预约",future],["已完成记录",Math.max(0,a.length-future)]]);return}q("#content").innerHTML=stats([["累计使用时段",db.bookings.length,"cyan"],["本周使用时段",week().reduce((s,x)=>s+x.count,0)],["可用设备",40-db.faults.filter(x=>x.status==="faulty").length],["故障设备",db.faults.filter(x=>x.status==="faulty").length,"warn"]])+`<div class="grid2"><section class="panel"><h2>一周使用情况</h2><p class="sub">按预约时段统计</p>${chart()}</section><section class="panel admin-export"><p class="eyebrow">管理员权限</p><h2>管理文件导出</h2><p class="helper">导出每日设备使用情况和故障维护记录。</p><button id="c2" class="secondary wide">下载 count.txt</button><button id="e2" class="secondary wide" style="margin-top:.6rem">下载 error.txt</button></section></div>`;q("#c2").onclick=countFile;q("#e2").onclick=errorFile}
/** Returns the current Monday-through-Sunday week instead of seven future days. */
function week(){let anchor=new Date(`${today()}T00:00:00`),offset=(anchor.getDay()+6)%7;anchor.setDate(anchor.getDate()-offset);return Array.from({length:7},(_,i)=>{let d=new Date(anchor);d.setDate(anchor.getDate()+i);let key=d.toLocaleDateString("en-CA");return {date:key,count:db.bookings.filter(x=>x.date===key&&x.status!=="cancelled").length}})}
function chart(){let a=week(),m=Math.max(1,...a.map(x=>x.count));return `<div class="bars">${a.map(x=>`<div class="bar-col"><b>${x.count}</b><i class="bar" style="height:${Math.max(5,x.count/m*112)}px"></i><span>${fd(x.date).replace("日","")}</span></div>`).join("")}</div>`}
function faults(){
  let manager=admin(),issues=(manager?db.faults:db.faults.filter(item=>item.reporterId===user().id)).slice().reverse();
  head(manager?"ADMIN / MAINTENANCE":"STUDENT / DEVICE REPORT",manager?"故障维护":"设备故障上报");
  if(manager){
    let open=issues.filter(item=>item.status!=="resolved"),faulty=db.devices.filter(item=>item.status==="faulty").length;
    q("#content").innerHTML=`<section class="maintenance-hero"><div><p class="eyebrow">MAINTENANCE INBOX</p><h2>需要关注的设备</h2><p>每条记录都保留故障现象、上报人和处理状态</p></div><div class="maintenance-metrics"><div><b>${open.length}</b><span>待处理</span></div><div><b>${faulty}</b><span>故障设备</span></div></div></section><div class="maintenance-layout"><section class="panel issue-inbox"><div class="section-head"><div><p class="eyebrow">LIVE RECORDS</p><h2>维修收件箱</h2></div><button id="report" class="secondary small-btn">上报设备故障</button></div>${faultList(issues)}</section><aside class="panel issue-context"><p class="eyebrow">DEVICE CONTEXT</p><h2>先看现象，再处理设备</h2><p>移动到设备管理中的红色 <b>!</b> 方格，即可快速查看该设备最新的故障说明</p><div><span>记录包含</span><b>设备 · 现象或原因 · 上报人 · 时间</b></div></aside></div>`;
  }else{
    q("#content").innerHTML=`<section class="panel issue-inbox student-issues"><div class="section-head"><div><p class="eyebrow">DEVICE REPORT</p><h2>我的故障记录</h2><p class="sub">提交后可在这里查看故障现象和处理进度</p></div><button id="report" class="secondary small-btn">上报设备故障</button></div>${faultList(issues)}</section>`;
  }
  q("#report").onclick=openModal;if(manager)qa("[data-fix]").forEach(button=>button.onclick=()=>fix(button.dataset.fix));syncLanguage()
}
function issueState(issue){return issue.status==="resolved"?'<span class="tag ok">已处理</span>':issue.status==="reported"?'<span class="tag">待处理</span>':'<span class="tag bad">故障中</span>'}
function faultList(issues){return issues.length?`<div class="issue-list">${issues.map(issue=>`<article class="issue-card"><div class="issue-signal" aria-hidden="true">!</div><div class="issue-body"><header><div><p class="eyebrow">${deviceLabel(issue.deskNo)}</p><h3>故障现象或原因</h3></div>${issueState(issue)}</header><p class="issue-description">${esc(systemFaultText(issue.description))}</p><footer><span><b>上报人</b>${esc(issue.reporter||tr("管理员"))}</span><span><b>上报时间</b>${new Date(issue.createdAt).toLocaleString(locale(),{hour12:false})}</span></footer></div>${admin()&&issue.status!=="resolved"?`<button class="secondary small-btn issue-resolve" data-fix="${issue.id}">恢复</button>`:""}</article>`).join("")}</div>`:'<div class="issue-empty"><b>暂无故障记录。</b><span>设备状态正常时，这里会保持安静</span></div>'}
function openModal(selectedDesk){q("#faultDesk").innerHTML=DESKS.map(n=>`<option value="${n}">${deviceLabel(n)}</option>`).join("");if(Number.isInteger(selectedDesk))q("#faultDesk").value=String(selectedDesk);q("#faultModal").showModal();bindFaultForm();syncLanguage()}
function go(){qa("[data-go]").forEach(b=>b.onclick=()=>{page=b.dataset.go;render()});cancelBind()}
function download(name,text){let a=document.createElement("a");a.href=URL.createObjectURL(new Blob([text],{type:"text/plain;charset=utf-8"}));a.download=name;a.click();URL.revokeObjectURL(a.href);toast(language==="en"?`Downloaded ${name}`:`已下载 ${name}`)}
/** Downloads one daily count.txt report using nine assignment-defined hourly sections. */
function countFile(){let en=language==="en",out=[en?"Intelligent Lab Reservation System - Daily Usage":"智能实验室预约系统 - 每日使用情况统计",en?"Total devices: 40":"设备总数：40 台",en?"Hours: 08:00–17:00 (hourly)":"开放时段：08:00-17:00（每小时）",""];let x={date:today(),count:db.bookings.filter(b=>b.date===today()&&b.status!=="cancelled").length};out.push(en?`${x.date} (${fd(x.date)}): ${x.count} reservations`:`${x.date}（${fd(x.date)}）：${x.count} 人次`);SLOTS.forEach(slot=>{let a=db.bookings.filter(b=>b.date===x.date&&bookingSlots(b).includes(slot)&&b.status!=="cancelled"),names=a.map(b=>`#${String(b.deskNo).padStart(2,"0")} ${b.userName}`).join(en?"; ":"；");out.push(en?`  ${fh(slot)}: ${a.length} reservation${a.length===1?"":"s"}${a.length?` (${names})`:""}`:`  ${fh(slot)}：${a.length} 人次${a.length?`（${names}）`:""}`)});download("count.txt",out.join("\\n"))}
function errorFile(){let en=language==="en",out=[en?"Intelligent Lab Reservation System - Device Issues":"智能实验室预约系统 - 设备故障记录",""];if(!db.faults.length)out.push(en?"No issue records yet.":"暂无故障记录。");db.faults.forEach((x,i)=>{let state=x.status==="resolved"?tr("已处理"):x.status==="reported"?tr("待处理"):tr("故障中");out.push(en?`${i+1}. Device #${String(x.deskNo).padStart(2,"0")} | Status: ${state} | Reported by: ${x.reporter||tr("管理员")}\n   Description: ${systemFaultText(x.description)}`:`${i+1}. 设备 #${String(x.deskNo).padStart(2,"0")} | 状态：${state} | 上报人：${x.reporter||tr("管理员")}\n   描述：${x.description}`)});download("error.txt",out.join("\\n"))}
qa("[data-tab]").forEach(b=>b.onclick=()=>{let r=b.dataset.tab==="register";qa("[data-tab]").forEach(x=>x.classList.toggle("active",x===b));q("#loginForm").hidden=r;q("#registerForm").hidden=!r;q("#adminRegisterForm").hidden=true});
qa("[data-language]").forEach(button=>button.onclick=()=>setLanguage(button.dataset.language));
/** Public registration always creates a student; administrator roles never come from this form. */
q("#registerForm").onsubmit=e=>{e.preventDefault();let f=new FormData(e.target),loginId=f.get("loginId").trim(),name=f.get("name").trim();if(!name){toast("请输入姓名。","error");return}if(db.users.some(x=>x.loginId===loginId)){toast("该账号已注册，请直接登录。","error");return}let u={id:id("user"),name,loginId,password:f.get("password"),role:"student"};db.users.push(u);db.session=u.id;save();page="dashboard";toast("注册成功。");startSession()};
q("#loginForm").onsubmit=e=>{e.preventDefault();let f=new FormData(e.target),u=db.users.find(x=>x.loginId===f.get("loginId").trim()&&x.password===f.get("password"));if(!u){toast("账号或密码不正确。","error");return}db.session=u.id;save();page="dashboard";startSession()};
q("#faultForm").onsubmit=e=>{e.preventDefault();let n=+q("#faultDesk").value;if(fault(n)){toast("该设备已处于故障状态。","error");return}db.faults.push({id:id("fault"),deskNo:n,description:q("#faultText").value.trim(),reporter:user().name,reporterId:user().id,status:admin()?"faulty":"reported",createdAt:new Date().toISOString()});save();e.target.reset();q("#faultModal").close();toast(admin()?"设备已标记故障。":"故障已上报。");render()};
q("#closeModal").onclick=()=>q("#faultModal").close();q("#logout").onclick=()=>{db.session=null;save();desks.clear();render()};

/** Maps a public student ID to an internal Auth email without exposing it in the UI. */
function studentEmail(loginId){
  let normalized=loginId.trim().toLowerCase();
  if(!/^[a-z0-9._-]{2,20}$/.test(normalized))throw new Error("Student ID must use 2–20 letters, numbers, dots, hyphens, or underscores.");
  return `${normalized}@lab.njupt.local`;
}

/** Normalises Supabase tables into the existing screen model. */
function toCloudState(profile,profiles,devices,reservations,reports){
  let users=profiles.map(item=>({id:item.id,name:item.display_name,loginId:item.login_id,role:item.role,active:item.active}));
  let names=new Map(users.map(item=>[item.id,item.name]));
  return {
    users,
    devices:devices.map(item=>({number:item.number,status:item.status})),
    bookings:reservations.map(item=>({id:item.id,deskNo:item.device_number,date:item.reservation_date,hour:item.start_hour,userId:item.user_id,userName:names.get(item.user_id)||"Student",status:item.status})),
    faults:reports.map(item=>({id:item.id,deskNo:item.device_number,description:item.description,reporter:names.get(item.reporter_id)||"Student",reporterId:item.reporter_id,status:item.status,createdAt:item.created_at,resolvedAt:item.resolved_at})),
    session:profile.id
  };
}

/** Loads the signed-in user's permitted cloud data; RLS controls every response. */
async function refreshCloud(renderAfter=true){
  let {data:{session}}=await supabase.auth.getSession();
  if(!session){db={users:[],devices:[],bookings:[],faults:[],session:null};if(renderAfter)render();return false}
  let {data:profile,error:profileError}=await supabase.from("profiles").select("id,login_id,display_name,role,active").eq("id",session.user.id).maybeSingle();
  if(profileError||!profile){toast("Account profile is not ready. Please try again.","error");return false}
  if(!profile.active){await supabase.auth.signOut();db={users:[],devices:[],bookings:[],faults:[],session:null};toast("This account has been deactivated.","error");if(renderAfter)render();return false}
  let [devicesResult,reservationsResult,reportsResult,profilesResult]=await Promise.all([
    supabase.from("devices").select("number,status").order("number"),
    supabase.from("reservations").select("id,device_number,user_id,reservation_date,start_hour,status,created_at,cancelled_at"),
    supabase.from("issue_reports").select("id,device_number,reporter_id,description,status,created_at,resolved_at"),
    profile.role==="admin"?supabase.from("profiles").select("id,login_id,display_name,role,active").order("created_at"):Promise.resolve({data:[profile],error:null})
  ]);
  let failed=[devicesResult,reservationsResult,reportsResult,profilesResult].find(result=>result.error);
  if(failed){toast(failed.error.message,"error");return false}
  db=toCloudState(profile,profilesResult.data,devicesResult.data,reservationsResult.data,reportsResult.data);
  refreshBookingStates();
  if(renderAfter)render();
  return true;
}

function cloudError(error){return error?.message||"The request could not be completed. Please try again."}

/** Calls the atomic server function, which rejects a reservation race safely. */
async function book(){
  if(!syncBookingRange().valid){toast("预约时间已开始或今日预约已结束。","error");dashStudent();return}
  if(!desks.size){toast("请至少选择一台设备。","error");return}
  let button=q("#book");button.disabled=true;
  let {error}=await supabase.rpc("reserve_hourly_devices",{requested_date:date,requested_hour:startHour,requested_devices:[...desks]});
  if(error){button.disabled=false;toast(cloudError(error),"error");await refreshCloud();return}
  desks.clear();
  await refreshCloud(false);
  showBookingSuccess(60);
}

async function cancelCloud(reservationId){
  let {error}=await supabase.from("reservations").update({status:"cancelled",cancelled_at:new Date().toISOString()}).eq("id",reservationId);
  if(error){toast(cloudError(error),"error");return}
  toast("预约已取消。");
  await refreshCloud();
}

function cancelBind(){qa("[data-cancel]").forEach(button=>button.onclick=()=>cancelCloud(button.dataset.cancel))}

/** Restores a faulty device and closes every active report attached to it. */
async function restoreDevice(number){
  let {error:issueError}=await supabase.from("issue_reports").update({status:"resolved",resolved_at:new Date().toISOString(),resolved_by:user().id}).eq("device_number",number).in("status",["reported","faulty"]);
  if(issueError){toast(cloudError(issueError),"error");return}
  let {error:deviceError}=await supabase.from("devices").update({status:"available",updated_by:user().id,updated_at:new Date().toISOString()}).eq("number",number);
  if(deviceError){toast(cloudError(deviceError),"error");return}
  toast("设备已恢复可预约");await refreshCloud();lab();
}

async function fix(issueId){
  let issue=db.faults.find(item=>item.id===issueId);if(!issue)return;
  let {error:issueError}=await supabase.from("issue_reports").update({status:"resolved",resolved_at:new Date().toISOString(),resolved_by:user().id}).eq("id",issueId);
  if(issueError){toast(cloudError(issueError),"error");return}
  let {error:deviceError}=await supabase.from("devices").update({status:"available",updated_by:user().id,updated_at:new Date().toISOString()}).eq("number",issue.deskNo);
  if(deviceError){toast(cloudError(deviceError),"error");return}
  toast("故障已处理。");await refreshCloud();render();
}

async function forceRefresh(){await refreshCloud();toast("设备状态已更新。");lab()}

async function removeStudent(userId){
  let student=db.users.find(item=>item.id===userId);if(!student||student.role!=="student")return;
  if(!window.confirm(tr("确认注销该学生账号吗？其预约将被删除，此操作不可恢复。")))return;
  let {error}=await supabase.rpc("deactivate_student",{target_user:userId});
  if(error){toast(cloudError(error),"error");return}
  toast("账号已注销。");await refreshCloud();users();
}

async function submitCloudIssue(event){
  event.preventDefault();
  let description=q("#faultText").value.trim(),button=q("#submitFault");
  if(!validateFaultDescription()){q("#faultText").focus();return}
  button.disabled=true;
  let deviceNumber=+q("#faultDesk").value;
  if(admin()&&fault(deviceNumber)){button.disabled=false;toast("该设备已处于故障状态。","error");return}
  let {error}=await supabase.from("issue_reports").insert({device_number:deviceNumber,reporter_id:user().id,description,status:admin()?"faulty":"reported"});
  if(!error&&admin()){
    let {error:deviceError}=await supabase.from("devices").update({status:"faulty",updated_by:user().id,updated_at:new Date().toISOString()}).eq("number",deviceNumber);
    error=deviceError;
  }
  button.disabled=false;
  if(error){toast(cloudError(error),"error");return}
  q("#faultForm").reset();q("#faultModal").close();toast(admin()?"设备已标记故障。":"故障已上报。");await refreshCloud();render();
}

/** Gives the short maintenance form local feedback before it reaches the database. */
function validateFaultDescription(){
  let field=q("#faultText"),error=q("#faultError"),valid=field.value.trim().length>=4;
  error.hidden=valid;if(!valid)error.textContent=language==="en"?"Describe the issue in at least 4 characters.":"故障说明至少需要 4 个字符。";
  field.setAttribute("aria-invalid",String(!valid));return valid;
}
function bindFaultForm(){
  let field=q("#faultText"),count=q("#faultCount");
  let refresh=()=>count.textContent=`${field.value.length} / 120`;
  refresh();field.oninput=refresh;field.onblur=validateFaultDescription;
  qa("[data-symptom]").forEach(button=>button.onclick=()=>{field.value=field.value?`${field.value.trim()}；${button.dataset.symptom}`:button.dataset.symptom;refresh();field.focus()});
}

async function registerCloud(event){
  event.preventDefault();
  let form=new FormData(event.target),loginId=form.get("loginId").trim(),name=form.get("name").trim(),password=form.get("password");
  if(!name){toast("请输入姓名。","error");return}
  let email;
  try{email=studentEmail(loginId)}catch(error){toast(error.message,"error");return}
  let {data,error}=await supabase.auth.signUp({email,password,options:{data:{login_id:loginId,display_name:name}}});
  if(error){toast(cloudError(error),"error");return}
  if(!data.session){toast("Disable Confirm email in Supabase before student testing.","error");return}
  await new Promise(resolve=>setTimeout(resolve,250));
  if(await refreshCloud(false)){page="dashboard";toast("注册成功。");startSession()}
}

/** Switches between the ordinary student and invitation-only administrator forms. */
function showAuthForm(form){
  q("#loginForm").hidden=form!=="login";
  q("#registerForm").hidden=form!=="student";
  q("#adminRegisterForm").hidden=form!=="admin";
  qa("[data-tab]").forEach(button=>button.classList.toggle("active",button.dataset.tab===(form==="login"?"login":"register")));
  syncLanguage();
}

/** Signs up an administrator only when the database trigger accepts the supplied one-time code. */
async function registerAdministrator(event){
  event.preventDefault();
  let form=new FormData(event.target),name=form.get("name").trim(),email=form.get("email").trim().toLowerCase(),password=form.get("password"),inviteCode=form.get("inviteCode").trim().toUpperCase();
  if(!name||!email||!inviteCode){toast("Complete every field to continue.","error");return}
  let {data,error}=await supabase.auth.signUp({email,password,options:{data:{login_id:email,display_name:name,admin_invite_code:inviteCode}}});
  if(error){toast(cloudError(error),"error");return}
  if(!data.session){toast("Disable Confirm email in Supabase before testing.","error");return}
  await new Promise(resolve=>setTimeout(resolve,250));
  if(await refreshCloud(false)){
    if(!admin()){await supabase.auth.signOut();toast("The invitation was not accepted.","error");return}
    page="dashboard";toast("Administrator account created.");startSession();
  }
}

/** Generates one time-limited code through the administrator-only database function. */
async function createAdministratorInvite(event){
  event.preventDefault();
  let days=Number(q("#inviteDays").value),expiry=new Date();expiry.setDate(expiry.getDate()+days);
  let {data,error}=await supabase.rpc("create_admin_invite",{invite_expires_at:expiry.toISOString()});
  if(error){toast(cloudError(error),"error");return}
  q("#inviteCodeOutput").value=data[0].invite_code;
  q("#inviteResult").hidden=false;
}

function openInviteDialog(){q("#inviteResult").hidden=true;q("#inviteModal").showModal()}
async function copyInvitationCode(){
  try{await navigator.clipboard.writeText(q("#inviteCodeOutput").value);toast("邀请码已复制。")}catch{q("#inviteCodeOutput").select()}
}

/** Saves one editable profile value without ever exposing an Auth administrator key. */
async function updateProfileIdentity(target,input,field,originalValue){
  let nextValue=input.value.trim();
  if(nextValue===originalValue){input.parentElement.textContent=originalValue;return}
  let nextName=field==="name"?nextValue:target.name;
  let nextLoginId=field==="loginId"?nextValue:target.loginId;
  let {error}=await supabase.rpc("update_profile_identity",{target_user:target.id,next_display_name:nextName,next_login_id:nextLoginId});
  if(error){toast(cloudError(error),"error");input.parentElement.textContent=originalValue;return}
  toast("名称已更新。");await refreshCloud();
}

/** Converts an administrator-table name or student/staff ID cell to an inline field. */
function beginProfileIdentityEdit(cell,field){
  if(cell.querySelector("input"))return;
  let row=cell.parentElement,rowIndex=[...row.parentElement.children].indexOf(row),target=db.users[rowIndex];
  if(!target)return;
  let originalValue=target[field],input=document.createElement("input"),saved=false;
  input.value=originalValue;input.maxLength=field==="name"?16:(target.role==="student"?20:80);input.setAttribute("aria-label",field==="name"?(language==="en"?"Display name":"显示名称"):(language==="en"?"Student or staff ID":"学号或工号"));
  cell.replaceChildren(input);input.focus();input.select();
  let save=()=>{if(saved)return;saved=true;updateProfileIdentity(target,input,field,originalValue)};
  input.addEventListener("blur",save);
  input.addEventListener("keydown",event=>{
    if(event.key==="Enter"){event.preventDefault();save()}
    if(event.key==="Escape"){saved=true;cell.textContent=originalValue}
  });
}

document.addEventListener("click",event=>{
  let cell=event.target.closest("#content .table tbody tr td"),row=cell?.parentElement;
  if(!cell||!row||!admin()||page!=="users")return;
  let column=[...row.children].indexOf(cell);
  if(column===0)beginProfileIdentityEdit(cell,"name");
  if(column===2)beginProfileIdentityEdit(cell,"loginId");
});

/** Presents the latest fault record near its red device square without hiding it in a native title. */
function latestOpenFault(number){return db.faults.filter(item=>item.deskNo===number&&item.status!=="resolved").sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)))[0]}
function hideFaultTooltip(){let tip=q("#faultTooltip");if(tip)tip.hidden=true}
function placeFaultTooltip(event){let tip=q("#faultTooltip");if(!tip||tip.hidden)return;let x=Math.min(window.innerWidth-tip.offsetWidth-16,event.clientX+16),y=Math.min(window.innerHeight-tip.offsetHeight-16,event.clientY+16);tip.style.transform=`translate(${Math.max(16,x)}px,${Math.max(16,y)}px)`}
function showFaultTooltip(desk,event){let latest=latestOpenFault(Number(desk.dataset.desk)),tip=q("#faultTooltip");if(!latest||!tip){hideFaultTooltip();return}q("#faultTooltipDevice").textContent=deviceLabel(latest.deskNo);q("#faultTooltipDescription").textContent=latest.description;q("#faultTooltipMeta").textContent=new Date(latest.createdAt).toLocaleString(locale(),{hour12:false});tip.hidden=false;placeFaultTooltip(event)}
document.addEventListener("pointerover",event=>{let desk=event.target.closest("#content [data-desk]");if(!desk||!admin()||page!=="lab"||!fault(Number(desk.dataset.desk)))return;desk.removeAttribute("title");showFaultTooltip(desk,event)});
document.addEventListener("pointermove",event=>{if(event.target.closest("#content [data-desk]"))placeFaultTooltip(event)});
document.addEventListener("pointerout",event=>{if(event.target.closest("#content [data-desk]"))hideFaultTooltip()});

async function loginCloud(event){
  event.preventDefault();
  let form=new FormData(event.target),identity=form.get("loginId").trim(),password=form.get("password"),email;
  try{
    if(identity.includes("@"))email=identity;
    else{
      let {data,error}=await supabase.rpc("resolve_student_auth_email",{entered_login_id:identity});
      if(error)throw error;
      if(!data)throw new Error("Incorrect ID or password.");
      email=data;
    }
  }catch(error){toast(error.message,"error");return}
  let {error}=await supabase.auth.signInWithPassword({email,password});
  if(error){toast("账号或密码不正确。","error");return}
  if(await refreshCloud(false)){page="dashboard";startSession()}
}

async function logoutCloud(){await supabase.auth.signOut();desks.clear();await refreshCloud()}

function subscribeCloud(){
  let timer;
  supabase.channel("lab-reservation-live")
    .on("postgres_changes",{event:"*",schema:"public",table:"devices"},queue)
    .on("postgres_changes",{event:"*",schema:"public",table:"reservations"},queue)
    .on("postgres_changes",{event:"*",schema:"public",table:"issue_reports"},queue)
    .subscribe();
  function queue(){clearTimeout(timer);timer=setTimeout(()=>{if(!document.body.classList.contains("booking-success"))refreshCloud()},150)}
}

async function bootCloud(){
  q("#registerForm").onsubmit=registerCloud;
  q("#adminRegisterForm").onsubmit=registerAdministrator;
  q("#loginForm").onsubmit=loginCloud;
  q("#faultForm").onsubmit=submitCloudIssue;
  q("#logout").onclick=logoutCloud;
  q("#showAdminRegister").onclick=()=>showAuthForm("admin");
  q("#showStudentRegister").onclick=()=>showAuthForm("student");
  q("#openInvite").onclick=openInviteDialog;
  q("#closeInviteModal").onclick=()=>q("#inviteModal").close();
  q("#inviteForm").onsubmit=createAdministratorInvite;
  q("#copyInviteCode").onclick=copyInvitationCode;
  subscribeCloud();
  await refreshCloud();
}

bootCloud();

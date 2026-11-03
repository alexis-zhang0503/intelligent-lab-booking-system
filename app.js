import { registrationError, authErrorMessage } from "./web/core/authRules.js";
import { DEVICE_NUMBERS as DESKS, HOURLY_SLOTS as SLOTS, hourToMinutes, hourlyRange } from "./web/core/hourlyRules.js";
import { supabase } from "./web/core/supabaseClient.js";
import { playIntro } from "./web/ui/introMotion.js";

// Browser storage holds language preference only; reservation data stays in Supabase.
const LANGKEY="lab-booking-language";
const EN={"实验室预约":"Lab Reservation","把时间留给探索":"Make time for discovery","选定设备，约好时间。把注意力留给实验本身。":"Choose a device and time. Leave your attention for the work.","登录":"Sign in","注册":"Create account","开始预约":"Start a reservation","登录后，设备、时段和你的记录，都在这里。":"Your devices, times, and records are all here.","学号 / 工号":"Student / staff ID","密码":"Password","登录并继续":"Sign in and continue","创建账号":"Create your account","管理员和学生均通过注册获得账号。":"Students and administrators both register here.","姓名":"Name","身份":"Role","学生":"Student","管理员":"Administrator","注册并进入":"Create account and enter","退出":"Sign out","实验室管理":"Lab Operations","实验室总览":"Lab Overview","总览":"Overview","设备":"Devices","用户":"Users","统计":"Reports","故障":"Issues","预约":"Reserve","时长":"Hours","我的":"Me","预约流程":"Reservation flow","三步完成":"Three steps","选择设备":"Choose a device","设置时间":"Set the time","确认预约":"Confirm reservation","台实验设备":"lab devices","我的预约":"My reservations","实验室平面图":"Lab floor plan","点击空闲设备，可同时选择多个位置":"Select any available device. Multiple devices are supported.","预约日期":"Reservation date","空闲":"Available","已预约":"Reserved","故障设备":"Faulty devices","已选择":"Selected","预约时间":"Reservation time","设置开始与结束时间":"Set a start and end time","开始":"Start","结束":"End","至":"to","尚未选择设备":"No device selected","确认预约 →":"Confirm reservation →","实验室状态":"Lab status","台设备可预约":"devices available","报告设备故障":"Report a device issue","近期预约":"Recent reservations","完整预约记录已合并至“我的”":"Your complete reservation history is in Me.","在我的中查看":"View in Me","暂无记录。":"No records yet.","我的":"Me","今天的预约":"Today's reservations","今天还没有预约":"Nothing reserved for today","选好设备和起止时间后，会显示在这里。":"Choose a device and time, and it will appear here.","个预约时段":"reserved hours","去预约":"Reserve now","预约偏好":"Reservation preferences","根据你的实际预约自动汇总":"Automatically summarised from your reservations.","常用设备":"Most used device","常用时段":"Most used time","未来预约":"Upcoming reservations","取消后设备时段会立即释放。":"Cancelling releases the device and time immediately.","取消":"Cancel","注册用户":"Registered users","当前实验人数":"Current lab users","今日预约时段":"Reserved hours today","今日设备使用":"Today's device use","设备管理":"Manage devices","待处理故障":"Open issues","学生上报后由管理员确认":"Student reports are confirmed by an administrator.","故障维护":"Issue maintenance","本周使用趋势":"Weekly usage","管理员可在“统计与导出”中下载管理文件。":"Administrative files are available in Reports.","设备状态维护":"Device status","40 台设备平面图":"40-device floor plan","点击设备可切换正常 / 故障；故障设备无法被学生预约。":"Select a device to mark it available or faulty. Faulty devices cannot be reserved.","维护说明":"Maintenance notes","红色 ! 是故障设备。管理员可直接标记或恢复设备，学生上报会在故障维护中保留。":"A red ! marks a faulty device. Administrators can mark or restore devices here.","录入故障记录":"Add an issue record","用户档案":"User profiles","已注册用户":"Registered users","注册角色、账号和累计预约时长。":"Role, account, and total reserved hours.","累计时长":"Total hours","统计与导出":"Reports and exports","实验累计时间":"Lab hours","累计实验时长":"Total lab hours","已完成记录":"Completed records","时长明细":"Hours detail","每个已预约的一小时设备时段累计为一小时。":"Each reserved device hour counts as one hour.","累计使用时段":"Total reserved hours","本周使用时段":"Reserved hours this week","可用设备":"Available devices","一周使用情况":"Weekly usage","按预约时段统计":"By reserved hour","管理员权限":"Administrator access","管理文件导出":"Export management files","导出每日设备使用情况和故障维护记录。":"Export daily device use and issue-maintenance records.","下载 count.txt":"Download count.txt","下载 error.txt":"Download error.txt","设备故障上报":"Device issue report","全部故障记录":"All issue records","我的故障记录":"My issue reports","处理后设备会恢复可预约状态。":"When resolved, devices become available again.","提交后请等待管理员维护设备状态。":"After submitting, please wait for an administrator to update the device.","上报故障":"Report an issue","已处理":"Resolved","待处理":"Open","故障中":"Faulty","恢复":"Restore","电脑编号":"Device number","故障描述":"Issue description","例如：显示器无信号，无法开机":"For example: the monitor has no signal or the device will not start.","提交故障记录":"Submit issue report","关闭":"Close","正在进入实验室":"Entering the lab","预约已确认":"Reservation confirmed","设备与时间已为你保留":"Your device and time are reserved"};
Object.assign(EN,{"强制刷新状态":"Force refresh status","设备状态已更新。":"Device status updated.","设备状态已是最新。":"Device status is already current.","请至少选择一台设备。":"Select at least one device.","选择中存在故障或已预约设备。":"Your selection includes a faulty or reserved device.","NJUPT · 实验室预约":"NJUPT · Lab Reservation","NJUPT · 实验室管理":"NJUPT · Lab Operations","选择设备与起止时间　剩下的留给实验本身":"Choose a device and time. Leave the rest to the experiment.","暂无故障记录。":"No issue records yet.","预约时间已开始或今日预约已结束。":"This time has already started, or booking for today has closed.","管理员标记为故障。":"Marked faulty by an administrator.","管理员":"Administrator","故障已处理。":"Issue resolved.","设备已处于故障状态。":"This device is already marked as faulty.","故障已上报。":"Issue report submitted.","注册成功。":"Account created.","账号或密码不正确。":"Incorrect ID or password.","该账号已注册，请直接登录。":"This ID is already registered. Please sign in.","已下载 ":"Downloaded ","设备":"Device","设备总数：40 台":"Total devices: 40","开放时段：08:00-17:00（每 30 分钟）":"Hours: 08:00–17:00 (30-minute intervals)","智能实验室预约系统 - 每日使用情况统计":"Intelligent Lab Reservation System - Daily Usage","智能实验室预约系统 - 设备故障记录":"Intelligent Lab Reservation System - Device Issues","状态：":"Status: ","上报人：":"Reported by: ","描述：":"Description: ","账号操作":"Account action","注销账号":"Deactivate account","账号已注销。":"Account deactivated.","确认注销该学生账号吗？其预约将被删除，此操作不可恢复。":"Deactivate this student account? Their reservations will be deleted. This cannot be undone.","暂未形成":"Not established yet","预约已取消。":"Reservation cancelled.","今日预约已结束":"Booking closed for today","请选择未来日期继续预约":"Choose a future date to continue","智能实验室预约系统":"Intelligent Lab Reservation System","该设备已处于故障状态。":"This device is already marked as faulty.","每个已预约的半小时设备时段累计为半小时。":"Each reserved 30-minute device slot counts as half an hour."});
Object.assign(EN,{"开放时段：08:00-17:00（每小时）":"Hours: 08:00–17:00 (hourly)","每个已预约的一小时设备时段累计为一小时。":"Each reserved device hour counts as one hour.","创建学生账号":"Create a student account","学生可直接注册。管理员账号由项目维护者在后台开通。":"Students can register directly. Administrator accounts are provisioned by the project maintainer.","学号":"Student ID","学号 / 管理员邮箱":"Student ID / administrator email"});
Object.assign(EN,{"学生可直接注册。管理员账号需使用邀请码。":"Students can register directly. Administrator accounts require an invitation code.","使用邀请码注册管理员":"Register an administrator with an invitation","管理员注册":"Administrator registration","输入管理员邀请码后即可创建管理员账号":"Enter an administrator invitation code to create an administrator account","管理员邮箱":"Administrator email","管理员邀请码":"Administrator invitation code","注册管理员并进入":"Create administrator account and enter","返回学生注册":"Back to student registration","生成管理员邀请码":"Generate administrator invitation","邀请码仅显示一次。请通过可信方式发送给新管理员":"The code is shown once. Send it to the new administrator through a trusted channel.","有效期":"Valid for","1 天":"1 day","7 天":"7 days","30 天":"30 days","生成邀请码":"Generate invitation","邀请码":"Invitation code","复制邀请码":"Copy invitation code","邀请码已复制。":"Invitation code copied.","名称已更新。":"Name updated."});
Object.assign(EN,{"故障现象或原因":"Fault symptom or cause","例如：显示器无信号、无法开机或网络无法连接":"For example: no monitor signal, device will not start, or network unavailable"});
Object.assign(EN,{"请描述你实际看到的故障现象，或你认为可能的原因":"Describe what you observed or the cause you suspect","写下设备表现，管理员会据此定位和处理问题":"Describe the device behaviour so an administrator can diagnose and resolve it","常见故障现象":"Common symptoms","显示器无信号":"No monitor signal","无法开机":"Device will not start","网络无法连接":"Network unavailable","故障说明至少需要 8 个字符。":"Describe the issue in at least 8 characters.","故障说明":"Issue description","上报人":"Reported by","上报时间":"Reported","维修收件箱":"Maintenance inbox","需要关注的设备":"Devices needing attention","已处理":"Resolved","待处理":"Open","故障中":"Faulty","移动到故障设备上可快速查看故障现象或原因":"Hover a faulty device to preview its symptom or cause","设备状态和维修记录在这里同步":"Device state and maintenance records stay in sync here","全部故障记录":"All issue records","上报设备故障":"Report a device issue","故障设备":"Faulty devices","故障现象或原因":"Fault symptom or cause","选择设备并填写故障现象，再将它标记为故障":"Choose a device, describe the symptom, then mark it faulty","设备状态":"Device status","当前故障":"Current issues","选择可用设备以创建故障记录；选择红色设备即可恢复":"Choose an available device to create an issue record; select a red device to restore it","标记设备故障":"Mark device as faulty","设备已恢复可预约":"Device restored and available","没有待处理的故障记录":"No open issue records","设备状态和维修记录会在这里同步":"Device status and maintenance records stay in sync here","最近故障记录":"Recent issue records","恢复可预约":"Restore availability","点击可用设备开始填写故障说明":"Select an available device to describe the issue","点击红色设备恢复可预约":"Select a red device to restore availability"});
Object.assign(EN,{"查看详情":"View details","记录维修处理":"Record repair outcome","处理说明":"Repair outcome","这条说明会保存在该设备的故障档案中":"This note is saved to the device maintenance record","完成处理并恢复设备":"Complete repair and restore device","故障维修档案":"Device maintenance record","上报":"Reported","标记故障":"Marked faulty","完成修复":"Resolved","修复时间":"Resolved","修复人":"Resolved by","暂无处理说明":"No repair note was recorded","处理并恢复":"Resolve and restore","已更换网线并确认网络恢复":"For example: replaced the network cable and confirmed connectivity","处理说明至少需要 4 个字符。":"Describe the repair outcome in at least 4 characters.","系统需要升级故障追踪数据结构":"The issue-tracking database update is required"});
Object.assign(EN,{"例如：已更换网线并确认网络恢复":"For example: replaced the network cable and confirmed connectivity"});
Object.assign(EN,{"提交后可在这里查看故障现象和处理进度":"Track your report and its progress here","设备状态正常时，这里会保持安静":"No device issues to show","每条记录都保留故障现象、上报人和处理状态":"Each record retains the symptom, reporter, and status","设备维护":"Device maintenance","标记故障或恢复设备":"Mark a fault or restore a device","选择可用设备创建故障记录；红色设备可恢复":"Select an available device to create a record; restore a red device","点击可用设备填写故障说明":"Select an available device to add a fault note"});
Object.assign(EN,{"设备操作":"DEVICE OPERATIONS","实时平面图":"LIVE FLOOR PLAN","维护操作":"MAINTENANCE ACTION","待处理记录":"OPEN RECORDS","维修收件箱":"MAINTENANCE INBOX","实时记录":"LIVE RECORDS","设备上报":"DEVICE REPORT","维修处理记录":"RESOLUTION RECORD","管理员 / 设备状态":"ADMIN / DEVICE STATUS","管理员 / 维护":"ADMIN / MAINTENANCE","学生 / 设备上报":"STUDENT / DEVICE REPORT","管理员 / 总览":"ADMIN / OPERATIONS","管理员 / 用户":"ADMIN / USERS","管理员 / 报表":"ADMIN / REPORTS","学生 / 时长":"STUDENT / HOURS","我的资料":"MY PROFILE","学生 / 预约":"STUDENT / RESERVATION","请输入姓名。":"Enter your name.","设备状态已更新。":"Device status updated."});
Object.assign(EN,{"选定设备，约好时间，把注意力留给实验本身":"Choose a device and time, leave your attention for the work"});
Object.assign(EN,{"收到故障说明":"Fault details received","等待管理员处理":"Waiting for administrator action","这条记录仍在等待管理员确认":"This record is waiting for administrator confirmation","设备已停止预约":"Device is unavailable for reservations","设备状态已同步为故障，学生无法预约":"The device is marked faulty and cannot be reserved by students","设备已恢复可预约":"Device is available for reservations again","等待维修完成":"Awaiting repair completion","修复后会在这里记录处理说明":"The repair outcome will be recorded here once work is complete"});
Object.assign(EN,{"智能助手":"AI Assistant","AI 决策":"AI Insights","智能推荐":"Smart suggestions","没有符合全部约束的候选方案。":"No candidate satisfies every constraint.","已确认":"Confirmed","使用中":"In use","已完成":"Completed","已取消":"Cancelled","爽约":"No-show","手动签到":"Manual check-in","手动签退":"Manual check-out","手动签到与签退":"Manual check-in and check-out","仅在预约时段内签到；签退会完成实际使用记录。":"Check in only during the booked hour; check-out completes the usage record.","签到成功。":"Check-in recorded.","签退成功。":"Check-out recorded.","预约率":"Reservation rate","实际签到使用率":"Actual check-in usage rate","刷新 V2 指标":"Refresh V2 metrics","等待 V2 统计数据…":"Waiting for V2 metrics…","预约率与实际签到使用率分开统计。":"Reservation rate and actual check-in usage rate are reported separately.","容量":"Capacity","设备时":"device-hours","已签到":"Checked in","数据缺失":"Missing data","评分":"Score","空闲设备":"Available devices","排名":"Rank","采用":"Use","确认推荐方案":"Confirm recommended option","AI 仅分析实时指标，不执行管理操作。":"AI analyses live metrics only and makes no changes.","AI 只生成方案；选择后仍需你确认，才会调用预约服务。":"AI only suggests options. Your confirmation is required before the reservation service is called.","实时运营分析":"Live operations analysis","预约偏好与实时推荐":"Booking preferences and live recommendations","预约率与实际签到使用率会分别呈现。":"Reservation rate and actual check-in usage rate are shown separately.","不会直接写入数据库；确认后才创建已确认预约。":"It never writes directly to the database; a confirmed reservation is created only after your confirmation.","你的问题":"Your question","请分析本周预约率、签到率与异常数据":"Analyse this week's reservation rate, check-in rate, and data anomalies","例如：明天上午需要两台空闲设备":"For example: I need two available devices tomorrow morning","生成分析":"Generate analysis","生成方案":"Generate options","预约已取消。":"Reservation cancelled.","受控工具":"Tool-controlled","只读分析":"Read-only analysis","无法请求智能服务，请稍后重试。":"Unable to request the AI service. Please try again later.","请求未完成，请稍后重试。":"The request could not be completed. Please try again later.","AI 管理决策助手":"AI Admin Insights"});
Object.assign(EN,{"故障处置建议":"Fault response plan","故障处置计划":"Fault response plan","分析影响":"Analyse impact","影响等级":"Impact severity","高":"High","中":"Medium","低":"Low","受影响预约":"Affected reservations","替代设备建议":"Replacement device suggestions","需要用户确认":"User confirmation required","未找到同一时段替代设备":"No same-slot replacement found","确认隔离":"Confirm isolation","隔离原因":"Isolation note","管理员确认后，设备将停止新预约；现有预约不会自动改动。":"After confirmation, new reservations stop; existing reservations are not changed automatically.","设备当前已隔离":"Device is already isolated","故障处置已执行。":"Fault response executed.","正在分析故障影响…":"Analysing fault impact…","正在加载故障处置指标…":"Loading fault-response metrics…","暂无受影响预约。":"No future reservations are affected.","本周已隔离":"Isolated this week","本周已修复":"Repaired this week","平均修复时长":"Average repair time","重复故障设备":"Repeat-fault devices","分钟":"minutes","同一时段可用设备":"Same-slot available devices","例如：确认设备无法正常使用，暂停新预约":"For example: the device cannot be used normally; pause new reservations"});
Object.assign(EN,{"请选择单台设备报告故障":"Please select a single device to report a fault"});
Object.assign(EN,{"故障已上报，设备已停止预约。":"Issue reported. The device is now unavailable.","上报后设备立即停用，等待管理员维修":"Reports immediately disable the device pending repair","自动标记故障":"Automatically marked faulty","设备已停用；现有预约请联系管理员处理。":"The device is unavailable. Contact an administrator about existing reservations."});
Object.assign(EN,{"预约管理":"Reservation management","全部预约":"All reservations","预约用户":"Booked by","预约状态":"Reservation status","全部状态":"All statuses","按姓名、学号或设备搜索":"Search name, student ID or device","清除筛选":"Clear filters","强制取消":"Force cancel","只能取消尚未开始的学生预约，取消后时段立即释放，记录仍保留。":"Only unstarted student reservations can be cancelled. The slot is released and the record is preserved.","确认强制取消以下学生预约？":"Force cancel this student reservation?","该预约无法强制取消，请刷新后重试。":"This reservation cannot be force cancelled. Refresh and try again.","预约已强制取消。":"Reservation force cancelled.","预约已取消，但状态刷新失败，请重新刷新。":"Reservation cancelled, but refreshing failed. Please refresh again.","管理员预约或已开始的预约不可强制取消":"Administrator reservations and started reservations cannot be force cancelled","提交时间":"Submitted at","预约操作":"Reservation action"});
let adminBookingFilters={date:"",status:"",query:""};
Object.assign(EN,{"故障现象或原因（选填）":"Fault symptom or cause (optional)","处理说明（选填）":"Repair outcome (optional)","故障说明可不填写，填写时最多 120 个字符。":"Issue details are optional, up to 120 characters.","处理说明可不填写，填写时最多 120 个字符。":"Repair notes are optional, up to 120 characters.","未填写故障说明":"No issue details provided","删除记录":"Delete record","修复满 24 小时后可删除":"Can be deleted 24 hours after repair","确认删除这条已修复的故障记录？":"Delete this resolved issue record?","故障记录已删除。":"Issue record deleted.","该记录尚未修复满 24 小时，暂不能删除。":"This record cannot be deleted until 24 hours after repair.","故障记录已删除，但状态刷新失败，请重新刷新。":"Issue record deleted, but refreshing failed. Please refresh again.","选填，不填写也可以直接提交":"Optional. You can submit without entering any text.","修复完成满 24 小时后，管理员可以删除记录":"Administrators can delete records 24 hours after repair"});
Object.assign(EN,{"总览与统计":"Overview and reports","设备管理中心":"Device management","在这里管理设备状态、学生预约与故障记录":"Manage device status, student reservations and issue records here","查看预约管理":"View reservations","查看故障维护":"View maintenance"});
Object.assign(EN,{"请选择 1、7 或 30 天有效期。":"Choose a validity of 1, 7 or 30 days.","邀请码生成结果异常，请重试。":"The invitation response was invalid. Please try again.","生成中…":"Generating…","已生成，请先保存当前邀请码。":"Invitation generated. Save this code before continuing.","复制失败，请手动复制邀请码。":"Copy failed. Please copy the invitation manually.","请先生成邀请码。":"Generate an invitation first.","邀请码已生成，请及时保存。":"Invitation generated. Save it now.","生成新邀请码":"Generate another invitation","生成新的邀请码前，请确认已保存当前邀请码。是否继续？":"Have you saved the current invitation? Generate another one?","到期时间":"Expires at"});
let inviteGenerationPending=false;
const ZH=Object.fromEntries(Object.entries(EN).map(([zh,en])=>[en,zh]));
let language=localStorage.getItem(LANGKEY)||"en", db={users:[],devices:[],bookings:[],faults:[],session:null}, page="dashboard", date=today(), hours=new Set([8]), desks=new Set(), startHour=8, endHour=9, bookingRequestKey=null, scheduleCandidates=[], adminMetrics=null, agentResponse=null, faultImpactPlan=null, faultResponseMetrics=null;
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
function systemFaultText(text){return tr(text||"未填写故障说明")}
function toast(m,type=""){let e=q("#toast");e.textContent=tr(m);e.className="show "+type;clearTimeout(toast.t);toast.t=setTimeout(()=>e.className="",2500)}
function translateNode(node){let raw=node.nodeValue,trim=raw.trim();if(!trim)return;let symbol=trim.match(/^([*#!])\s+/),value=symbol?`${symbol[1]} ${tr(trim.slice(symbol[0].length))}`:tr(trim);if(value!==trim)node.nodeValue=raw.replace(trim,value)}
function syncLanguage(){document.documentElement.lang=language==="en"?"en":"zh-CN";document.title=tr("智能实验室预约系统");let walker=document.createTreeWalker(document.body,NodeFilter.SHOW_TEXT);for(let node;node=walker.nextNode();)if(!node.parentElement?.closest("script,style"))translateNode(node);qa("[placeholder],[aria-label]").forEach(el=>{for(let name of ["placeholder","aria-label"]){let value=el.getAttribute(name);if(value)el.setAttribute(name,tr(value))}});qa("[data-language]").forEach(button=>button.classList.toggle("active",button.dataset.language===language))}
function setLanguage(next){language=next;localStorage.setItem(LANGKEY,language);render()}
/** Returns the exact local end instant for a one-hour reservation. */
function bookingEnd(booking){let start=slotStart(booking.hour);return new Date(`${booking.date}T00:00:00`).getTime()+(start+60)*60000}
/** Reservation states are authoritative in Supabase; the browser never auto-completes them. */
function refreshBookingStates(){return false}
function scheduleStatusRefresh(){clearTimeout(scheduleStatusRefresh.timer);let now=Date.now(),next=Math.min(...db.bookings.filter(booking=>booking.status!=="completed"&&bookingEnd(booking)>now).map(booking=>bookingEnd(booking)));if(Number.isFinite(next))scheduleStatusRefresh.timer=setTimeout(()=>{let changed=refreshBookingStates();if(changed&&user())render();else scheduleStatusRefresh()},Math.max(250,next-Date.now()+80))}
function fault(n){return db.devices?.find(x=>x.number===n&&x.status==="faulty")||db.faults.find(x=>x.deskNo===n&&x.status!=="resolved")}
function faultyDeviceCount(){return DESKS.filter(n=>fault(n)).length}
function at(n,d=date){return db.bookings.filter(x=>x.deskNo===n&&x.date===d)}
/** Returns the single hourly value occupied by this reservation. */
function bookingSlots(booking){return [+booking.hour]}
function conflict(n){let active=new Set(["reserved","confirmed","in_use","completed"]);return [...hours].some(slot=>db.bookings.some(x=>x.deskNo===n&&x.date===date&&bookingSlots(x).includes(slot)&&active.has(x.status)))}
/** Calculates the first bookable full hour for the selected date. */
function bookingWindow(){let now=new Date(),sameDay=date===today(),nextStart=sameDay?Math.max(8,now.getHours()+(now.getMinutes()||now.getSeconds()?1:0)):8,closed=nextStart>16;return {sameDay,nextStart,closed,valid:!closed&&startHour>=nextStart&&startHour<=16&&endHour===startHour+1,now}}
/** Keeps the UI state constrained to one whole assignment-defined hour. */
function syncBookingRange(){let state=bookingWindow();if(state.closed){hours.clear();return state}if(startHour<state.nextStart||startHour>16)startHour=state.nextStart;endHour=startHour+1;hours=new Set([startHour]);return bookingWindow()}
function liveClock(d=new Date()){return new Intl.DateTimeFormat(language==="en"?"en-US":"zh-CN",{hour:"2-digit",minute:"2-digit",hour12:false}).format(d)}
function scheduleLiveTimeCheck(){clearTimeout(scheduleLiveTimeCheck.timer);if(admin()||page!=="dashboard"||date!==today())return;let delay=60000-Date.now()%60000+80;scheduleLiveTimeCheck.timer=setTimeout(()=>{if(document.body.classList.contains("booking-success")){scheduleLiveTimeCheck();return}dashStudent()},delay)}
function render(keepAuth=false){if(admin()){if(page==="reports")page="dashboard";if(["reservations","faults"].includes(page))page="lab"}refreshBookingStates();if(!user()){q("#auth").hidden=false;q("#app").hidden=true;syncLanguage();return}q("#auth").hidden=!keepAuth;q("#app").hidden=false;q("#profile").innerHTML=`<div class="avatar">${esc(user().name[0])}</div><div><b>${esc(user().name)}</b><span>${tr(admin()?"管理员":"学生")} · ${esc(user().loginId)}</span></div>`;nav();({dashboard:admin()?dashAdmin:dashStudent,lab,users,reports,faults,me,reservations:adminReservations,assistant:agentPage})[page]?.();let content=q("#content");content.classList.remove("page-enter");void content.offsetWidth;content.classList.add("page-enter");syncLanguage();scheduleStatusRefresh()}
function startSession(){render(true);let reduce=matchMedia("(prefers-reduced-motion: reduce)").matches,overlay=q("#entryOverlay");if(reduce){q("#auth").hidden=true;return}overlay.hidden=false;overlay.setAttribute("aria-hidden","false");document.body.classList.add("launching");requestAnimationFrame(()=>document.body.classList.add("launching-ready"));setTimeout(()=>{q("#auth").hidden=true;overlay.hidden=true;overlay.setAttribute("aria-hidden","true");document.body.classList.remove("launching","launching-ready")},2000)}
/** Runs one restrained two-second startup sequence before the login view becomes interactive. */
function playSiteEntry(){playIntro()}
function nav(){let list=admin()?[["dashboard","01","总览与统计"],["lab","02","设备"],["users","03","用户"],["assistant","04","AI 决策"]]:[["dashboard","01","预约"],["assistant","02","智能助手"],["reports","03","时长"],["faults","04","故障"],["me","05","我的"]];q("#nav").innerHTML=list.map(x=>`<button class="nav-btn ${page===x[0]?"active":""}" data-p="${x[0]}"><i class="nav-code">${x[1]}</i>${tr(x[2])}</button>`).join("");qa("[data-p]").forEach(b=>b.onclick=()=>{navigatePage(b.dataset.p)})}
function head(k,t){document.body.dataset.role=admin()?"admin":"student";q("#kicker").textContent=tr(admin()?"NJUPT · 实验室管理":"NJUPT · 实验室预约");q("#title").textContent=tr(t)}
function stats(a){return `<div class="stats">${a.map(x=>`<article class="stat"><span>${tr(x[0])}</span><b class="${x[2]||""}">${x[1]}</b></article>`).join("")}</div>`}
/** Renders forty devices with explicit available (*), reserved (#), and faulty (!) symbols. */
function map(mode){let groups=Array.from({length:20},(_,i)=>[i*2+1,i*2+2]);return `<div class="lab">${groups.map(g=>`<div class="group">${g.map(n=>{let f=fault(n),c=conflict(n),blocked=mode==="student"&&(f||c),s=desks.has(n)&&!blocked,cl=["desk",blocked?"unavailable":"",f?"faulty":"",!f&&c?"occupied":"",s?"selected":""].filter(Boolean).join(" "),status=f?"故障":c?"已预约":s?"已选择":"空闲",symbol=f?"!":c?"#":s?"●":"*",label=language==="en"?`Device ${n}, ${tr(status)}`:`${n}号设备，${status}`;return `<button class="${cl}" data-desk="${n}" aria-label="${label}" ${blocked?"disabled":""}><b>${String(n).padStart(2,"0")}</b><i aria-hidden="true">${symbol}</i></button>`}).join("")}</div>`).join("")}</div>`}
/** Renders the fixed nine one-hour choices from 08:00 through 17:00. */
function timePick(){let state=bookingWindow(),isEn=language==="en",label=isEn?"Reservation time":"预约时间";if(state.closed)return `<div class="time-picker"><h3>${label}</h3><div class="time-check blocked"><div><b>${isEn?"Booking unavailable today":"今日无法预约"}</b><span>${isEn?"Choose another date to continue":"请选择其他日期继续预约"}</span></div></div></div>`;return `<div class="time-picker"><h3>${label}</h3><div class="time-range"><label><span>${isEn?"Start":"开始"}</span><select id="startTime">${SLOTS.map(slot=>`<option value="${slot}" ${slot===startHour?"selected":""} ${slot<state.nextStart?"disabled":""}>${hourlyRange(slot)}</option>`).join("")}</select></label><i>${isEn?"to":"至"}</i><label><span>${isEn?"End":"结束"}</span><select id="endTime" disabled><option>${hourlyRange(startHour)}</option></select></label></div></div>`}
function selection(){let ds=[...desks].sort((a,b)=>a-b);return ds.length?`${ds.map(n=>"#"+String(n).padStart(2,"0")).join(" ")} · ${timeRange()}`:tr("尚未选择设备")}
function pruneSelections(){for(let n of desks)if(fault(n)||conflict(n))desks.delete(n)}
/** Renders the student flow, including a legend that never hides fault status. */
function dashStudent(){syncBookingRange();pruneSelections();head("STUDENT / RESERVATION","实验室预约");let mine=db.bookings.filter(x=>x.userId===user().id),available=DESKS.filter(n=>!fault(n)&&!conflict(n)).length,faulty=faultyDeviceCount(),canBook=bookingWindow().valid;q("#content").innerHTML=`<section class="booking-intro"><div class="booking-copy"><p class="eyebrow">${fd(date)}</p><h2>把时间留给探索</h2><p>选择设备与起止时间　剩下的留给实验本身</p><div class="booking-summary"><span><b>40</b> 台实验设备</span><span><b>${mine.length}</b> 我的预约</span></div></div><section class="reservation-guide"><p class="eyebrow">预约流程</p><h3>三步完成</h3><ol><li><b>01</b>选择设备</li><li><b>02</b>设置时间</li><li><b>03</b>确认预约</li></ol></section></section><div class="booking"><section class="panel lab-panel"><div class="panel-head"><div><h2>实验室平面图</h2><p class="sub">点击空闲设备，可同时选择多个位置</p></div><label class="booking-date"><span>预约日期</span><input id="date" class="date" type="date" min="${today()}" value="${date}"></label></div><div class="legend"><span><i></i>* 空闲</span><span><i class="occ"></i># 已预约</span><span><i class="bad"></i>! 故障</span><span><i class="pick"></i>已选择</span></div>${map("student")}</section><aside class="side-select"><div class="panel booking-flow">${timePick()}<div class="selection compact-selection">${selection()}</div><button id="book" class="primary" ${canBook?"":"disabled"}>确认预约 <span>→</span></button><button id="suggest" class="secondary wide" type="button">智能推荐</button><div id="scheduleSuggestions"></div></div><section class="panel lab-status"><p class="eyebrow">实验室状态</p><b>${available}<span> / 40</span></b><p>台设备可预约</p><div><span>故障设备</span><strong>${faulty}${language==="en"?" devices":" 台"}</strong></div></section><button id="report" class="secondary">报告设备故障</button></aside></div><section class="panel recent-panel schedule-panel"><div class="section-head"><div><h2>近期预约</h2><p class="sub">完整预约记录已合并至“我的”</p></div><button data-go="me" class="secondary small-btn">在我的中查看</button></div>${records(mine.slice(-3).reverse(),true)}</section>`;q("#date").onchange=e=>{date=e.target.value;desks.clear();bookingRequestKey=null;dashStudent()};bindTimes();bindDesks("student");q("#book").onclick=book;q("#suggest").onclick=loadScheduleCandidates;q("#report").onclick=reportSelectedDevice;renderScheduleCandidates();go();syncLanguage();scheduleLiveTimeCheck()}
/** Reports the selected device without requiring a second device selection. */
function reportSelectedDevice(){
  if(desks.size!==1){toast("请选择单台设备报告故障","error");return}
  openModal([...desks][0]);
}
/** Binds the single start selector; the end value is always exactly one hour later. */
function bindTimes(){let start=q("#startTime");if(!start)return;start.onchange=e=>{startHour=+e.target.value;bookingRequestKey=null;syncBookingRange();dashStudent()}}
/** Handles administrator fault toggles and student multi-device selection. */
function bindDesks(mode){qa("[data-desk]").forEach(e=>e.onclick=()=>{let n=+e.dataset.desk;if(mode==="admin"){let issue=latestOpenFault(n);if(fault(n)&&issue)openRepairModal(issue.id);else openModal(n);return}desks.has(n)?desks.delete(n):desks.add(n);bookingRequestKey=null;let selected=desks.has(n);e.classList.toggle("selected",selected);e.querySelector("i").textContent=selected?"●":"*";e.setAttribute("aria-label",language==="en"?`Device ${n}, ${selected?"Selected":"Available"}`:`${n}号设备，${selected?"已选择":"空闲"}`);q(".compact-selection").textContent=selection()})}
/** Shows the fixed two-second confirmation after a successful hourly booking. */
function showBookingSuccess(durationMinutes){let overlay=q("#bookingOverlay"),reduce=matchMedia("(prefers-reduced-motion: reduce)").matches;q("#bookingOverlayCopy").textContent=language==="en"?`Your ${durationMinutes}-minute reservation is ready`:`已为你保留 ${durationMinutes} 分钟`;if(reduce){dashStudent();toast("Reservation confirmed");return}overlay.hidden=false;overlay.setAttribute("aria-hidden","false");document.body.classList.add("booking-success");void overlay.offsetWidth;document.body.classList.add("booking-success-ready");setTimeout(()=>{overlay.hidden=true;overlay.setAttribute("aria-hidden","true");document.body.classList.remove("booking-success","booking-success-ready");dashStudent();toast("Reservation confirmed")},2000)}
/** Validates every selected device immediately before storing the one-hour reservation. */
function reservationStatus(status){return ({reserved:"已确认",confirmed:"已确认",in_use:"使用中",completed:"已完成",cancelled:"已取消",no_show:"爽约"})[status]||status}
function mayCancel(booking){return ["reserved","confirmed"].includes(booking.status)&&booking.date>=today()}
function records(a,cancel=false){return a.length?`<div class="records">${a.map(x=>`<div class="record"><div><b>#${String(x.deskNo).padStart(2,"0")} · ${fh(x.hour)}</b><small>${fd(x.date)}${x.userName?" · "+esc(x.userName):""}</small></div>${cancel&&mayCancel(x)?`<button class="secondary small-btn" data-cancel="${x.id}">${tr("取消")}</button>`:`<span class="tag">${esc(reservationStatus(x.status))}</span>`}</div>`).join("")}</div>`:`<p class="empty">${tr("暂无记录。")}</p>`}
/** Administrators see the owner and full schedule without exposing extra profiles to students. */
function reservationOwner(booking){return db.users.find(item=>item.id===booking.userId)}
function canAdminCancel(booking){
  return admin()&&reservationOwner(booking)?.role==="student"&&["reserved","confirmed"].includes(booking.status)&&
    (booking.date>today()||(booking.date===today()&&Number(booking.hour)>new Date().getHours()));
}
function filteredAdminBookings(){
  let query=adminBookingFilters.query.trim().toLowerCase();
  return db.bookings.filter(booking=>{
    let owner=reservationOwner(booking);
    return (!adminBookingFilters.date||booking.date===adminBookingFilters.date)&&
      (!adminBookingFilters.status||booking.status===adminBookingFilters.status||(adminBookingFilters.status==="confirmed"&&booking.status==="reserved"))&&
      (!query||[owner?.name,owner?.loginId,booking.userName,String(booking.deskNo).padStart(2,"0")].join(" ").toLowerCase().includes(query));
  }).sort((a,b)=>b.date.localeCompare(a.date)||a.hour-b.hour||a.deskNo-b.deskNo);
}
function adminReservationRows(){
  let rows=filteredAdminBookings();
  return rows.length?`<div class="table-wrap"><table class="table"><thead><tr><th>预约用户</th><th>学号 / 工号</th><th>电脑编号</th><th>预约日期</th><th>预约时间</th><th>预约状态</th><th>提交时间</th><th>预约操作</th></tr></thead><tbody>${rows.map(booking=>{let owner=reservationOwner(booking);return `<tr><td>${esc(owner?.name||booking.userName||"—")}</td><td>${esc(owner?.loginId||"—")}</td><td>#${String(booking.deskNo).padStart(2,"0")}</td><td>${esc(booking.date)}</td><td>${esc(fh(booking.hour))}</td><td><span class="tag">${tr(reservationStatus(booking.status))}</span></td><td>${booking.createdAt?esc(new Date(booking.createdAt).toLocaleString(language==="en"?"en-US":"zh-CN",{hour12:false})):"—"}</td><td>${canAdminCancel(booking)?`<button class="secondary small-btn danger-btn" data-admin-cancel="${esc(booking.id)}">${tr("强制取消")}</button>`:"—"}</td></tr>`}).join("")}</tbody></table></div>`:`<p class="empty">${tr("暂无记录。")}</p>`;
}
function renderAdminReservationRows(){q("#adminReservationRows").innerHTML=adminReservationRows();qa("[data-admin-cancel]").forEach(button=>button.onclick=()=>forceCancelStudentReservation(button.dataset.adminCancel,button));syncLanguage()}
function adminReservationModule(){return `<section id="deviceReservations" class="panel admin-module"><div class="section-head"><div><h2>预约管理</h2><p class="sub">只能取消尚未开始的学生预约，取消后时段立即释放，记录仍保留。</p></div></div><div class="grid2"><label>预约日期<input id="adminReservationDate" type="date" value="${esc(adminBookingFilters.date)}"></label><label>预约状态<select id="adminReservationStatus"><option value="">${tr("全部状态")}</option>${["confirmed","in_use","completed","cancelled","no_show"].map(status=>`<option value="${status}" ${adminBookingFilters.status===status?"selected":""}>${tr(reservationStatus(status))}</option>`).join("")}</select></label><label>预约用户<input id="adminReservationSearch" type="search" maxlength="100" placeholder="${tr("按姓名、学号或设备搜索")}" value="${esc(adminBookingFilters.query)}"></label><div><button id="resetAdminReservationFilters" class="secondary small-btn">清除筛选</button></div></div><div id="adminReservationRows"></div></section>`}
function bindAdminReservationFilters(){
  q("#adminReservationDate").onchange=event=>{adminBookingFilters.date=event.target.value;renderAdminReservationRows()};
  q("#adminReservationStatus").onchange=event=>{adminBookingFilters.status=event.target.value;renderAdminReservationRows()};
  q("#adminReservationSearch").oninput=event=>{adminBookingFilters.query=event.target.value;renderAdminReservationRows()};
  q("#resetAdminReservationFilters").onclick=()=>{adminBookingFilters={date:"",status:"",query:""};lab()};
  renderAdminReservationRows();
}
function adminReservations(){if(!admin()){page="dashboard";dashStudent();return}navigatePage("reservations")}
async function forceCancelStudentReservation(reservationId,button){
  let booking=db.bookings.find(item=>item.id===reservationId);
  if(!booking||!canAdminCancel(booking)){toast("该预约无法强制取消，请刷新后重试。","error");return}
  if(button?.disabled)return;
  let owner=reservationOwner(booking);
  if(!window.confirm(`${tr("确认强制取消以下学生预约？")}\n${owner.name} · ${owner.loginId}\n#${String(booking.deskNo).padStart(2,"0")} · ${booking.date} · ${fh(booking.hour)}`))return;
  if(button)button.disabled=true;
  try{
    let {error}=await supabase.rpc("v2_cancel_reservation",{p_reservation_id:reservationId});
    if(error)throw error;
    booking.status="cancelled";booking.cancelledAt=new Date().toISOString();adminMetrics=null;scheduleCandidates=[];bookingRequestKey=null;
    toast("预约已强制取消。");
    try{if(!await refreshCloud(false))throw new Error("Refresh failed")}catch(error){toast("预约已取消，但状态刷新失败，请重新刷新。","error")}
    render();
  }catch(error){toast(cloudError(error),"error")}finally{if(button)button.disabled=false}
}
function favorite(a,key,label){let counts={};a.forEach(x=>counts[x[key]]=(counts[x[key]]||0)+1);let top=Object.entries(counts).sort((a,b)=>b[1]-a[1])[0];return top?label(top[0],top[1]):tr("暂未形成")}
function usageControls(booking){if(booking.status==="in_use")return `<button class="secondary small-btn" data-check-out="${booking.id}">手动签退</button>`;if(["reserved","confirmed"].includes(booking.status)&&booking.date===today())return `<button class="secondary small-btn" data-check-in="${booking.id}">手动签到</button>`;return ""}
function myUsageRecords(bookings){return bookings.length?`<div class="records">${bookings.map(booking=>`<div class="record"><div><b>#${String(booking.deskNo).padStart(2,"0")} · ${fh(booking.hour)}</b><small>${fd(booking.date)} · ${reservationStatus(booking.status)}</small></div><div>${usageControls(booking)}</div></div>`).join("")}</div>`:`<p class="empty">暂无记录。</p>`}
function me(){head("MY PROFILE","我的");let mine=db.bookings.filter(x=>x.userId===user().id),todayMine=mine.filter(x=>x.date===today()).sort((a,b)=>a.hour-b.hour),future=mine.filter(x=>x.date>=today()).sort((a,b)=>(a.date+a.hour).localeCompare(b.date+b.hour)),favoriteDesk=favorite(mine,"deskNo",(n,c)=>"#"+String(n).padStart(2,"0")+" · "+timesLabel(c)),favoriteHour=favorite(mine,"hour",(slot,c)=>fh(slot).split("–")[0]+" · "+timesLabel(c));q("#content").innerHTML=`<section class="profile-hero"><div class="profile-mark">${esc(user().name[0])}</div><div><h2>${esc(user().name)}</h2><p>${tr(admin()?"管理员":"学生")} · ${esc(user().loginId)}</p></div></section><section class="today-spotlight"><div><p class="eyebrow">${fd(today())}</p><h2>${todayMine.length?"今天的预约":"今天还没有预约"}</h2><p>${todayMine.length?todayMine.map(x=>"#"+String(x.deskNo).padStart(2,"0")+" · "+fh(x.hour)).join("　"):"选好设备和起止时间后，会显示在这里。"}</p></div><div class="today-number"><b>${todayMine.length}</b><span>${slotsLabel(todayMine.length)}</span></div>${todayMine.length?"":`<button data-go="dashboard" class="secondary small-btn">去预约</button>`}</section><div class="profile-grid"><section class="panel preference-panel"><h2>预约偏好</h2><p class="sub">根据你的实际预约自动汇总</p><div class="preference-list"><div><span>常用设备</span><b>${favoriteDesk}</b></div><div><span>常用时段</span><b>${favoriteHour}</b></div><div><span>未来预约</span><b>${slotsLabel(future.length)}</b></div></div></section><section class="panel my-bookings"><h2>手动签到与签退</h2><p class="sub">仅在预约时段内签到；签退会完成实际使用记录。</p>${myUsageRecords(mine.slice().sort((a,b)=>(a.date+a.hour).localeCompare(b.date+b.hour)))}</section><section class="panel my-bookings"><h2>我的预约</h2><p class="sub">取消后设备时段会立即释放。</p>${records(mine.slice().sort((a,b)=>(a.date+a.hour).localeCompare(b.date+b.hour)),true)}</section></div>`;go();bindUsageActions()}
/** Renders administrator metrics using the actual current full-hour slot. */
function percent(value){return value===null||value===undefined?"—":`${(Number(value)*100).toFixed(1)}%`}
/** Combines live overview, weekly statistics and management exports. */
function dashAdmin(){
  head("ADMIN / OPERATIONS","总览与统计");
  let t=db.bookings.filter(x=>x.date===today()),nowSlot=new Date().getHours(),metrics=adminMetrics;
  q("#content").innerHTML=stats([["注册用户",db.users.length],["当前实验人数",t.filter(x=>bookingSlots(x).includes(nowSlot)&&x.status==="in_use").length,"cyan"],["预约率",percent(metrics?.reservation_rate)],["实际签到使用率",percent(metrics?.actual_check_in_usage_rate),"cyan"],["No-show",percent(metrics?.no_show_rate),"warn"],["故障设备",faultyDeviceCount(),"warn"],["累计使用时段",db.bookings.length,"cyan"],["本周使用时段",week().reduce((sum,item)=>sum+item.count,0)],["可用设备",40-faultyDeviceCount()] ]).replace('class="stats"','class="stats admin-overview-stats"')+`<div class="grid2"><section class="panel"><div class="section-head"><div><h2>本周使用趋势</h2><p class="sub">预约率与实际签到使用率分开统计。</p></div><button id="refreshMetrics" class="secondary small-btn">刷新 V2 指标</button></div>${chart()}${metrics?`<p class="sub">容量 ${metrics.capacity_device_hours} 设备时 · 已预约 ${metrics.booked_device_hours} · 已签到 ${metrics.checked_in_device_hours} · 数据缺失 ${metrics.data_completeness?.active_or_completed_missing_check_in??0}</p>`:'<p class="sub">等待 V2 统计数据…</p>'}</section><section class="panel admin-export"><p class="eyebrow">管理员权限</p><h2>管理文件导出</h2><p class="helper">导出每日设备使用情况和故障维护记录。</p><button id="c2" class="secondary wide">下载 count.txt</button><button id="e2" class="secondary wide" style="margin-top:.6rem">下载 error.txt</button></section></div><section class="panel admin-module-links"><button data-go="reservations" class="secondary">查看预约管理</button><button data-go="faults" class="secondary">查看故障维护</button></section>`;
  go();q("#c2").onclick=countFile;q("#e2").onclick=errorFile;
  q("#refreshMetrics").onclick=()=>loadAdminMetrics(true);if(adminMetrics===null)loadAdminMetrics();
}
async function loadAdminMetrics(force=false){if(!admin())return;if(force)adminMetrics=null;let range=week(),{data,error}=await supabase.rpc("v2_admin_metrics",{p_start_date:range[0].date,p_end_date:range[6].date});if(error){if(force)toast(cloudError(error),"error");return}adminMetrics=data;if(page==="dashboard")dashAdmin()}
/** Device operations contain both reservation and maintenance modules. */
function adminFaultModule(){
  let issues=db.faults.slice().reverse();
  return `<section id="deviceFaults" class="panel issue-inbox issue-inbox-wide admin-module"><div class="section-head"><div><p class="eyebrow">MAINTENANCE</p><h2>故障维护</h2><p class="sub">修复完成满 24 小时后，管理员可以删除记录</p></div><span class="tag bad">${issues.filter(item=>item.status!=="resolved").length} ${tr("待处理")}</span></div><div id="deviceFaultMetrics">${faultResponseSummary(faultResponseMetrics)}</div>${faultList(issues)}</section>`;
}
function lab(){
  if(!admin()){page="dashboard";dashStudent();return}
  let faulty=faultyDeviceCount();head("ADMIN / DEVICE STATUS","设备管理中心");
  q("#content").innerHTML=`<section class="device-overview"><div><p class="eyebrow">DEVICE OPERATIONS</p><h2>设备管理中心</h2><p>在这里管理设备状态、学生预约与故障记录</p></div><div class="device-overview-metric"><b>${faulty}</b><span>当前故障</span></div></section><div class="device-layout"><section class="panel device-plan"><div class="section-head"><div><p class="eyebrow">LIVE FLOOR PLAN</p><h2>40 台设备平面图</h2><p class="sub">点击可用设备填写故障说明</p></div><div class="device-legend"><span><i></i>空闲</span><span><i class="bad"></i>故障</span></div></div>${map("admin")}</section><aside class="device-console"><section class="panel device-actions"><p class="eyebrow">MAINTENANCE ACTION</p><h2>设备状态</h2><p>选择可用设备创建故障记录；红色设备可恢复</p><button id="report" class="primary wide">标记设备故障 <span>→</span></button><button id="refreshStatus" class="secondary wide">强制刷新状态</button><button data-go="reservations" class="secondary wide">查看预约管理</button><button data-go="faults" class="secondary wide">查看故障维护</button></section></aside></div><div class="device-modules">${adminReservationModule()}${adminFaultModule()}</div>`;
  bindDesks("admin");q("#refreshStatus").onclick=forceRefresh;q("#report").onclick=()=>openModal();
  bindAdminReservationFilters();bindIssueActions();go();syncLanguage();
  if(faultResponseMetrics===null)loadFaultResponseMetrics();
}
function users(){head("ADMIN / USERS","用户档案");q("#content").innerHTML=`<section class="panel"><h2>已注册用户</h2><p class="sub">注册角色、账号和累计预约时长。</p><div class="table-wrap"><table class="table profile-identity-table"><thead><tr><th>姓名</th><th>身份</th><th>学号 / 工号</th><th>累计时长</th><th>账号操作</th></tr></thead><tbody>${db.users.map(x=>`<tr><td>${esc(x.name)}</td><td><span class="tag">${x.role==="admin"?"管理员":"学生"}</span></td><td>${esc(x.loginId)}</td><td>${durationLabel(db.bookings.filter(b=>b.userId===x.id))}</td><td>${x.role==="student"?`<button class="secondary small-btn danger-btn" data-remove-user="${x.id}">注销账号</button>`:"—"}</td></tr>`).join("")}</tbody></table></div></section>`;qa("[data-remove-user]").forEach(button=>button.onclick=()=>removeStudent(button.dataset.removeUser));syncLanguage()}
function reports(){
  if(admin()){page="dashboard";dashAdmin();return}
  head("STUDENT / HOURS","实验累计时间");let a=db.bookings.filter(x=>x.userId===user().id),future=a.filter(x=>x.date>=today()).length;
  q("#content").innerHTML=stats([["累计实验时长",durationLabel(a),"cyan"],["未来预约",future],["已完成记录",Math.max(0,a.length-future)]]);
}
/** Returns the current Monday-through-Sunday week instead of seven future days. */
function week(){let anchor=new Date(`${today()}T00:00:00`),offset=(anchor.getDay()+6)%7;anchor.setDate(anchor.getDate()-offset);return Array.from({length:7},(_,i)=>{let d=new Date(anchor);d.setDate(anchor.getDate()+i);let key=d.toLocaleDateString("en-CA");return {date:key,count:db.bookings.filter(x=>x.date===key&&x.status!=="cancelled").length}})}
function chart(){let a=week(),m=Math.max(1,...a.map(x=>x.count));return `<div class="bars">${a.map(x=>`<div class="bar-col"><b>${x.count}</b><i class="bar" style="height:${Math.max(5,x.count/m*112)}px"></i><span>${fd(x.date).replace("日","")}</span></div>`).join("")}</div>`}
function faultResponseSummary(metrics){if(!metrics)return`<p class="sub">${tr("正在加载故障处置指标…")}</p>`;let repeat=metrics.repeat_devices?.length??0,average=metrics.average_repair_minutes??"—";return`<section class="fault-response-summary"><div><span>${tr("本周已隔离")}</span><b>${metrics.isolated_count??0}</b></div><div><span>${tr("本周已修复")}</span><b>${metrics.repaired_count??0}</b></div><div><span>${tr("平均修复时长")}</span><b>${average}${average==="—"?"":` ${tr("分钟")}`}</b></div><div><span>${tr("重复故障设备")}</span><b>${repeat}</b></div></section>`}
function faults(){
  if(admin()){page="lab";lab();return}
  let issues=db.faults.filter(item=>item.reporterId===user().id).slice().reverse();
  head("STUDENT / DEVICE REPORT","设备故障上报");
  q("#content").innerHTML=`<section class="panel issue-inbox student-issues"><div class="section-head"><div><p class="eyebrow">DEVICE REPORT</p><h2>我的故障记录</h2><p class="sub">提交后可在这里查看故障现象和处理进度</p></div><button id="report" class="secondary small-btn">上报设备故障</button></div>${faultList(issues)}</section>`;
  q("#report").onclick=openModal;bindIssueActions();syncLanguage();
}
async function loadFaultResponseMetrics(){if(!admin())return;let range=week(),{data,error}=await supabase.rpc("v2_fault_response_metrics",{p_start_date:range[0].date,p_end_date:range[6].date});if(error)return;faultResponseMetrics=data;if(page==="lab"&&q("#deviceFaultMetrics")){q("#deviceFaultMetrics").innerHTML=faultResponseSummary(data);syncLanguage()}}
function issueState(issue){return issue.status==="resolved"?'<span class="tag ok">已处理</span>':'<span class="tag bad">故障中</span>'}
function faultList(issues){return issues.length?`<div class="issue-list">${issues.map(issue=>`<article class="issue-card"><div class="issue-signal" aria-hidden="true">!</div><div class="issue-body"><header><div><p class="eyebrow">${deviceLabel(issue.deskNo)}</p><h3>故障现象或原因</h3></div>${issueState(issue)}</header><p class="issue-description">${esc(systemFaultText(issue.description))}</p><footer><span><b>上报人</b>${esc(issue.reporter||tr("管理员"))}</span><span><b>上报时间</b>${new Date(issue.createdAt).toLocaleString(locale(),{hour12:false})}</span></footer></div><div class="issue-actions"><button class="secondary small-btn" data-issue-detail="${issue.id}">查看详情</button>${admin()&&issue.status==="resolved"?`<small>${tr("修复满 24 小时后可删除")}</small><button class="secondary small-btn danger-btn" data-delete-issue="${issue.id}" ${canDeleteIssue(issue)?"":"disabled"}>${tr("删除记录")}</button>`:""}${admin()&&issue.status!=="resolved"?`<button class="secondary small-btn" data-fault-plan="${issue.id}">分析影响</button><button class="secondary small-btn issue-resolve" data-fix="${issue.id}">处理并恢复</button>`:""}</div></article>`).join("")}</div>`:'<div class="issue-empty"><b>暂无故障记录。</b><span>设备状态正常时，这里会保持安静</span></div>'}
/** A resolved record becomes removable after a full 24 hours, not a date rollover. */
function canDeleteIssue(issue){return admin()&&issue.status==="resolved"&&!!issue.resolvedAt&&Number.isFinite(Date.parse(issue.resolvedAt))&&Date.now()-Date.parse(issue.resolvedAt)>=86400000}
async function deleteIssueRecord(issueId,button){
  let issue=db.faults.find(item=>item.id===issueId);
  if(!issue||!canDeleteIssue(issue)){toast("该记录尚未修复满 24 小时，暂不能删除。","error");return}
  if(button?.disabled)return;
  if(!window.confirm(`${tr("确认删除这条已修复的故障记录？")}\n${deviceLabel(issue.deskNo)} · ${faultTime(issue.resolvedAt)}`))return;
  if(button)button.disabled=true;
  try{
    let {error}=await supabase.rpc("archive_resolved_issue",{p_issue_id:issueId});
    if(error)throw error;
    db.faults=db.faults.filter(item=>item.id!==issueId);faultResponseMetrics=null;
    toast("故障记录已删除。");
    try{if(!await refreshCloud(false))throw new Error("Refresh failed")}catch(error){toast("故障记录已删除，但状态刷新失败，请重新刷新。","error")}
    render();
  }catch(error){toast(cloudError(error),"error")}finally{if(button)button.disabled=false}
}
function faultTime(value){return new Date(value).toLocaleString(locale(),{hour12:false})}
function openIssueDetail(issueId){
  let issue=db.faults.find(item=>item.id===issueId);if(!issue)return;
  let steps=[
    {label:"上报",title:"收到故障说明",copy:issue.description||"未填写故障说明",meta:`${issue.reporter} · ${faultTime(issue.createdAt)}`,state:"done"},
    {label:"自动标记故障",title:"设备已停止预约",copy:"设备状态已同步为故障，学生无法预约",meta:"",state:"done"},
    {label:"完成修复",title:issue.status==="resolved"?"设备已恢复可预约":"等待维修完成",copy:issue.status==="resolved"?(issue.repairNote||"暂无处理说明"):`修复后会在这里记录处理说明`,meta:issue.status==="resolved"?`${issue.resolver} · ${faultTime(issue.resolvedAt)}`:"",state:issue.status==="resolved"?"done":"pending"}
  ];
  q("#detailDevice").textContent=deviceLabel(issue.deskNo);q("#detailState").innerHTML=issueState(issue);
  q("#detailTimeline").innerHTML=steps.map((step,index)=>`<article class="timeline-event ${step.state}"><i>${index+1}</i><div><p>${step.label}</p><h3>${step.title}</h3><span>${esc(systemFaultText(step.copy))}</span>${step.meta?`<small>${esc(step.meta)}</small>`:""}</div></article>`).join("");
  q("#issueDetailModal").showModal();syncLanguage();
}
function bindIssueActions(){qa("[data-delete-issue]").forEach(button=>button.onclick=()=>deleteIssueRecord(button.dataset.deleteIssue,button));qa("[data-issue-detail]").forEach(button=>button.onclick=()=>openIssueDetail(button.dataset.issueDetail));qa("[data-fault-plan]").forEach(button=>button.onclick=()=>openFaultPlan(button.dataset.faultPlan));qa("[data-fix]").forEach(button=>button.onclick=()=>openRepairModal(button.dataset.fix))}
function faultSeverityLabel(value){return tr(({high:"高",medium:"中",low:"低"})[value]||"低")}
function faultImpactMarkup(plan){let affected=plan.affected_reservations||[];return`<div class="fault-plan"><div class="fault-plan-summary"><span class="tag ${plan.severity==="high"?"bad":""}">${tr("影响等级")}：${faultSeverityLabel(plan.severity)}</span><b>${tr("受影响预约")} ${plan.affected_count}</b></div><p class="sub">${tr("设备已停用；现有预约请联系管理员处理。")}</p>${affected.length?`<div class="fault-impact-list">${affected.map(item=>{let alternatives=item.replacement_device_numbers||[];return`<article><b>${esc(item.user_name)} · ${item.reservation_date} ${fh(item.start_hour)}</b><span>${tr("同一时段可用设备")}：${alternatives.length?alternatives.map(deviceLabel).join(" · "):tr("未找到同一时段替代设备")}</span><small>${tr("需要用户确认")}</small></article>`}).join("")}</div>`:`<p class="empty">${tr("暂无受影响预约。")}</p>`}</div>`}
async function openFaultPlan(issueId){let modal=q("#faultPlanModal"),content=q("#faultPlanContent"),form=q("#faultPlanForm");faultImpactPlan=null;content.innerHTML=`<p class="sub">${tr("正在分析故障影响…")}</p>`;form.hidden=true;modal.showModal();let {data,error}=await supabase.rpc("v2_fault_impact_analysis",{p_issue_id:issueId});if(error){content.innerHTML=`<p class="empty">${esc(cloudError(error))}</p>`;return}faultImpactPlan=data;content.innerHTML=faultImpactMarkup(data);form.hidden=true;if(data.issue_status==="faulty")content.insertAdjacentHTML("afterbegin",`<p class="tag bad">${tr("设备当前已隔离")}</p>`);syncLanguage()}
async function executeFaultIsolation(event){event.preventDefault();if(!faultImpactPlan)return;let note=q("#faultIsolationNote").value.trim(),button=q("#confirmFaultIsolation");if(!note){q("#faultIsolationNote").focus();return}button.disabled=true;let {error}=await supabase.rpc("v2_isolate_fault",{p_issue_id:faultImpactPlan.issue_id,p_note:note});button.disabled=false;if(error){toast(cloudError(error),"error");return}q("#faultPlanModal").close();faultImpactPlan=null;faultResponseMetrics=null;toast("故障处置已执行。");await refreshCloud(false);faults()}
function openModal(selectedDesk){q("#faultDesk").innerHTML=DESKS.map(n=>`<option value="${n}">${deviceLabel(n)}</option>`).join("");if(Number.isInteger(selectedDesk))q("#faultDesk").value=String(selectedDesk);q("#faultModal").showModal();bindFaultForm();syncLanguage()}
function navigatePage(destination){
  let target=admin()?({reservations:"deviceReservations",faults:"deviceFaults"})[destination]:null;
  page=admin()?(destination==="reports"?"dashboard":target?"lab":destination):destination;
  render();if(target)q("#"+target)?.scrollIntoView({behavior:matchMedia("(prefers-reduced-motion: reduce)").matches?"auto":"smooth",block:"start"});
}
function go(){qa("[data-go]").forEach(b=>b.onclick=()=>navigatePage(b.dataset.go));cancelBind()}
function download(name,text){let a=document.createElement("a");a.href=URL.createObjectURL(new Blob([text],{type:"text/plain;charset=utf-8"}));a.download=name;a.click();URL.revokeObjectURL(a.href);toast(language==="en"?`Downloaded ${name}`:`已下载 ${name}`)}
/** Downloads one daily count.txt report using nine assignment-defined hourly sections. */
function countFile(){let en=language==="en",out=[en?"Intelligent Lab Reservation System - Daily Usage":"智能实验室预约系统 - 每日使用情况统计",en?"Total devices: 40":"设备总数：40 台",en?"Hours: 08:00–17:00 (hourly)":"开放时段：08:00-17:00（每小时）",""];let x={date:today(),count:db.bookings.filter(b=>b.date===today()&&b.status!=="cancelled").length};out.push(en?`${x.date} (${fd(x.date)}): ${x.count} reservations`:`${x.date}（${fd(x.date)}）：${x.count} 人次`);SLOTS.forEach(slot=>{let a=db.bookings.filter(b=>b.date===x.date&&bookingSlots(b).includes(slot)&&b.status!=="cancelled"),names=a.map(b=>`#${String(b.deskNo).padStart(2,"0")} ${b.userName}`).join(en?"; ":"；");out.push(en?`  ${fh(slot)}: ${a.length} reservation${a.length===1?"":"s"}${a.length?` (${names})`:""}`:`  ${fh(slot)}：${a.length} 人次${a.length?`（${names}）`:""}`)});download("count.txt",out.join("\n"))}
function errorFile(){let en=language==="en",out=[en?"Intelligent Lab Reservation System - Device Issues":"智能实验室预约系统 - 设备故障记录",""];if(!db.faults.length)out.push(en?"No issue records yet.":"暂无故障记录。");db.faults.forEach((x,i)=>{let state=x.status==="resolved"?tr("已处理"):x.status==="reported"?tr("待处理"):tr("故障中");out.push(en?`${i+1}. Device #${String(x.deskNo).padStart(2,"0")} | Status: ${state} | Reported by: ${x.reporter||tr("管理员")}\n   Description: ${systemFaultText(x.description)}`:`${i+1}. 设备 #${String(x.deskNo).padStart(2,"0")} | 状态：${state} | 上报人：${x.reporter||tr("管理员")}\n   描述：${x.description}`)});download("error.txt",out.join("\n"))}
qa("[data-tab]").forEach(b=>b.onclick=()=>{let r=b.dataset.tab==="register";qa("[data-tab]").forEach(x=>x.classList.toggle("active",x===b));q("#loginForm").hidden=r;q("#registerForm").hidden=!r;q("#adminRegisterForm").hidden=true});
qa("[data-language]").forEach(button=>button.onclick=()=>setLanguage(button.dataset.language));
q("#closeModal").onclick=()=>q("#faultModal").close();
q("#closeRepairModal").onclick=()=>q("#repairModal").close();
q("#closeIssueDetail").onclick=()=>q("#issueDetailModal").close();

/** Maps a public student ID to an internal Auth email without exposing it in the UI. */
function studentEmail(loginId){
  let normalized=loginId.trim().toLowerCase();
  if(!/^[a-z0-9._-]{2,20}$/.test(normalized))throw new Error("Student ID must use 2–20 letters, numbers, dots, hyphens, or underscores.");
  return `${normalized}@lab.njupt.local`;
}

/** Normalises Supabase tables into the existing screen model. */
function toCloudState(profile,profiles,devices,reservations,reports,usageRecords=[]){
  let users=profiles.map(item=>({id:item.id,name:item.display_name,loginId:item.login_id,role:item.role,active:item.active}));
  let names=new Map(users.map(item=>[item.id,item.name]));
  let usageByReservation=new Map(usageRecords.map(item=>[item.reservation_id,item]));
  return {
    users,
    devices:devices.map(item=>({number:item.number,status:item.status})),
    bookings:reservations.map(item=>({id:item.id,deskNo:item.device_number,date:item.reservation_date,hour:item.start_hour,userId:item.user_id,userName:names.get(item.user_id)||"Student",createdAt:item.created_at,cancelledAt:item.cancelled_at,status:item.status,usage:usageByReservation.get(item.id)||null})),
    faults:reports.filter(item=>!item.deleted_at).map(item=>({id:item.id,deskNo:item.device_number,description:item.description,reporter:names.get(item.reporter_id)||"Student",reporterId:item.reporter_id,status:item.status,createdAt:item.created_at,resolvedAt:item.resolved_at,resolver:names.get(item.resolved_by)||"Administrator",repairNote:item.resolution_note||""})),
    session:profile.id
  };
}

/** Loads the signed-in user's permitted cloud data; RLS controls every response. */
async function refreshCloud(renderAfter=true){
  let {data:{session},error:sessionError}=await supabase.auth.getSession();
  if(sessionError)throw sessionError;
  if(!session){db={users:[],devices:[],bookings:[],faults:[],session:null};if(renderAfter)render();return false}
  let {data:profile,error:profileError}=await supabase.from("profiles").select("id,login_id,display_name,role,active").eq("id",session.user.id).maybeSingle();
  if(profileError||!profile){
    db={users:[],devices:[],bookings:[],faults:[],session:null};
    render();
    toast(profileError?cloudError(profileError):(language==="en"?"Account profile is not ready. Contact the administrator.":"账号资料尚未就绪，请联系管理员检查账号资料配置。"),"error");
    return false;
  }
  if(!profile.active){await supabase.auth.signOut();db={users:[],devices:[],bookings:[],faults:[],session:null};toast("This account has been deactivated.","error");if(renderAfter)render();return false}
  let [devicesResult,reservationsResult,reportsResult,profilesResult,usageResult]=await Promise.all([
    supabase.from("devices").select("number,status").order("number"),
    supabase.from("reservations").select("id,device_number,user_id,reservation_date,start_hour,status,created_at,cancelled_at"),
    loadIssueReports(),
    profile.role==="admin"?supabase.from("profiles").select("id,login_id,display_name,role,active").order("created_at"):Promise.resolve({data:[profile],error:null}),
    loadUsageRecords()
  ]);
  let failed=[devicesResult,reservationsResult,reportsResult,profilesResult,usageResult].find(result=>result.error);
  if(failed){toast(failed.error.message,"error");return false}
  db=toCloudState(profile,profilesResult.data,devicesResult.data,reservationsResult.data,reportsResult.data,usageResult.data||[]);
  refreshBookingStates();
  if(renderAfter)render();
  return true;
}

/** Keeps the site readable for projects that have not yet run the optional tracking migration. */
async function loadIssueReports(){
  let extended=await supabase.from("issue_reports").select("id,device_number,reporter_id,description,status,created_at,resolved_at,resolved_by,resolution_note,deleted_at").is("deleted_at",null);
  if(extended.error&&/deleted_at/i.test(extended.error.message||""))extended=await supabase.from("issue_reports").select("id,device_number,reporter_id,description,status,created_at,resolved_at,resolved_by,resolution_note");
  if(!extended.error)return extended;
  if(!/resolution_note|resolved_by/i.test(extended.error.message||""))return extended;
  return supabase.from("issue_reports").select("id,device_number,reporter_id,description,status,created_at,resolved_at");
}

/** Keeps the existing site readable until the additive V2 migration is applied. */
async function loadUsageRecords(){
  let result=await supabase.from("reservation_usage_records").select("reservation_id,checked_in_at,checked_out_at");
  if(!result.error)return result;
  if(/reservation_usage_records|does not exist|schema cache/i.test(result.error.message||""))return {data:[],error:null};
  return result;
}

function cloudError(error){console.warn("Supabase request failed",error);return authErrorMessage(error,language)}

/** Calls the atomic server function, which rejects a reservation race safely. */
async function book(){
  if(!syncBookingRange().valid){toast("预约时间已开始或今日预约已结束。","error");dashStudent();return}
  if(!desks.size){toast("请至少选择一台设备。","error");return}
  let button=q("#book");button.disabled=true;
  bookingRequestKey??=crypto.randomUUID();
  let {error}=await supabase.rpc("v2_create_reservations",{p_date:date,p_start_hour:startHour,p_device_numbers:[...desks].sort((a,b)=>a-b),p_idempotency_key:bookingRequestKey});
  if(error){button.disabled=false;toast(cloudError(error),"error");await refreshCloud();return}
  desks.clear();
  bookingRequestKey=null;
  await refreshCloud(false);
  showBookingSuccess(60);
}

async function cancelCloud(reservationId){
  let {error}=await supabase.rpc("v2_cancel_reservation",{p_reservation_id:reservationId});
  if(error){toast(cloudError(error),"error");return}
  toast("预约已取消。");
  await refreshCloud();
}

function cancelBind(){qa("[data-cancel]").forEach(button=>button.onclick=()=>cancelCloud(button.dataset.cancel))}

async function checkIn(reservationId){let {error}=await supabase.rpc("v2_check_in",{p_reservation_id:reservationId});if(error){toast(cloudError(error),"error");return}toast("签到成功。");await refreshCloud()}
async function checkOut(reservationId){let {error}=await supabase.rpc("v2_check_out",{p_reservation_id:reservationId});if(error){toast(cloudError(error),"error");return}toast("签退成功。");await refreshCloud()}
function bindUsageActions(){qa("[data-check-in]").forEach(button=>button.onclick=()=>checkIn(button.dataset.checkIn));qa("[data-check-out]").forEach(button=>button.onclick=()=>checkOut(button.dataset.checkOut))}

function candidateMarkup(candidates){return candidates?.length?`<div class="records">${candidates.map(candidate=>`<div class="record"><div><b>${fh(candidate.start_hour)} · ${candidate.device_numbers.map(number=>"#"+String(number).padStart(2,"0")).join(" ")}</b><small>${tr("排名")} ${candidate.candidate_rank}</small></div><button class="secondary small-btn" data-use-candidate="${candidate.candidate_rank}">采用</button></div>`).join("")}</div>`:`<p class="empty">没有符合全部约束的候选方案。</p>`}
async function loadScheduleCandidates(){let target=q("#scheduleSuggestions");if(target)target.innerHTML="<p class=\"sub\">正在计算实时方案…</p>";let {data,error}=await supabase.rpc("v2_schedule_candidates",{p_date:date,p_preferred_hour:startHour,p_required_device_count:Math.max(1,desks.size),p_preferred_devices:[...desks],p_limit:3});if(error){if(target)target.innerHTML=`<p class="empty">${esc(cloudError(error))}</p>`;return}scheduleCandidates=data||[];await supabase.rpc("v2_log_schedule_candidates",{p_date:date,p_preferred_hour:startHour,p_required_device_count:Math.max(1,desks.size),p_candidates:scheduleCandidates});renderScheduleCandidates()}
function renderScheduleCandidates(){let target=q("#scheduleSuggestions");if(!target)return;target.innerHTML=candidateMarkup(scheduleCandidates);qa("[data-use-candidate]").forEach(button=>button.onclick=()=>{let candidate=scheduleCandidates.find(item=>String(item.candidate_rank)===button.dataset.useCandidate);if(!candidate)return;date=candidate.reservation_date;startHour=Number(candidate.start_hour);endHour=startHour+1;hours=new Set([startHour]);desks=new Set(candidate.device_numbers);bookingRequestKey=null;dashStudent()})}

async function askAgent(confirmation=null){let input=admin()?{startDate:week()[0].date,endDate:week()[6].date}:{date,preferredHour:startHour,requiredDeviceCount:Math.max(1,desks.size),preferredDevices:[...desks]};let {data,error}=await supabase.functions.invoke("deepseek-booking-agent",{body:{mode:admin()?"admin":"booking",language,message:q("#agentPrompt")?.value||"请分析当前情况。",input,confirmation}});if(error){console.warn("DeepSeek booking agent invocation failed",error);agentResponse={error:tr("无法请求智能服务，请稍后重试。")};agentPage();return}agentResponse=data;if(data?.candidates)scheduleCandidates=data.candidates;if(data?.outcome==="confirmed"){desks.clear();bookingRequestKey=null;await refreshCloud(false)}agentPage()}
function agentPage(){let administrator=admin(),output=agentResponse?.error?`<p class="empty">${esc(agentResponse.error)}</p>`:agentResponse?`<p>${esc(agentResponse.message||"")}</p>${agentResponse.candidates?candidateMarkup(agentResponse.candidates):""}${agentResponse.metrics?`<pre>${esc(JSON.stringify(agentResponse.metrics,null,2))}</pre>`:""}${!administrator&&agentResponse.candidates?.length?`<button id="confirmAgent" class="primary">确认推荐方案</button>`:""}`:`<p class="sub">${administrator?"AI 仅分析实时指标，不执行管理操作。":"AI 只生成方案；选择后仍需你确认，才会调用预约服务。"}</p>`;head(administrator?"ADMIN / AI INSIGHTS":"STUDENT / AI ASSISTANT",administrator?"AI 管理决策助手":"智能助手");q("#content").innerHTML=`<section class="panel"><p class="eyebrow">${tr(administrator?"只读分析":"受控工具")}</p><h2>${administrator?"实时运营分析":"预约偏好与实时推荐"}</h2><p class="sub">${administrator?"预约率与实际签到使用率会分别呈现。":"不会直接写入数据库；确认后才创建已确认预约。"}</p><label>你的问题<textarea id="agentPrompt" maxlength="500" placeholder="${administrator?"请分析本周预约率、签到率与异常数据":"例如：明天上午需要两台空闲设备"}"></textarea></label><button id="askAgent" class="primary">${administrator?"生成分析":"生成方案"}</button><div id="agentOutput" class="panel" style="margin-top:1rem">${output}</div></section>`;q("#askAgent").onclick=()=>askAgent();q("#agentPrompt").onkeydown=event=>{if(event.key==="Enter"&&!event.shiftKey&&!event.isComposing&&!event.repeat){event.preventDefault();q("#askAgent").click()}};q("#confirmAgent")?.addEventListener("click",()=>{let candidate=agentResponse.candidates[0];if(!candidate)return;askAgent({accepted:true,date:candidate.reservation_date,startHour:Number(candidate.start_hour),deviceNumbers:candidate.device_numbers,idempotencyKey:crypto.randomUUID()})});qa("[data-use-candidate]").forEach(button=>button.onclick=()=>{let candidate=(agentResponse?.candidates||[]).find(item=>String(item.candidate_rank)===button.dataset.useCandidate);if(candidate)askAgent({accepted:true,date:candidate.reservation_date,startHour:Number(candidate.start_hour),deviceNumbers:candidate.device_numbers,idempotencyKey:crypto.randomUUID()})});syncLanguage()}

/** Opens a repair record so restoring a device always leaves an auditable outcome. */
function openRepairModal(issueId){
  let issue=db.faults.find(item=>item.id===issueId);if(!issue)return;
  q("#repairModal").dataset.issueId=issueId;
  q("#repairDeviceLabel").textContent=`${deviceLabel(issue.deskNo)} · ${tr("故障现象或原因")}：${systemFaultText(issue.description)}`;
  q("#repairText").value="";q("#repairCount").textContent="0 / 120";q("#repairError").hidden=true;
  q("#repairModal").showModal();bindRepairForm();syncLanguage();
}
function validateRepairDescription(){let field=q("#repairText"),error=q("#repairError"),valid=field.value.trim().length<=120;error.hidden=valid;if(!valid)error.textContent=tr("处理说明可不填写，填写时最多 120 个字符。");field.setAttribute("aria-invalid",String(!valid));return valid}
function bindRepairForm(){let field=q("#repairText"),count=q("#repairCount"),refresh=()=>count.textContent=`${field.value.length} / 120`;refresh();field.oninput=refresh;field.onblur=validateRepairDescription}
async function submitRepair(event){
  event.preventDefault();let issue=db.faults.find(item=>item.id===q("#repairModal").dataset.issueId),note=q("#repairText").value.trim(),button=q("#submitRepair");
  if(!issue||!validateRepairDescription()){q("#repairText").focus();return}
  button.disabled=true;
  let {error}=await supabase.rpc("v2_complete_fault_repair",{p_issue_id:issue.id,p_note:note});
  button.disabled=false;if(error){toast(cloudError(error),"error");return}
  q("#repairModal").close();faultResponseMetrics=null;toast("故障已处理。");await refreshCloud(false);faults();
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
  if(button.disabled)return;
  if(!validateFaultDescription()){q("#faultText").focus();return}
  let deviceNumber=+q("#faultDesk").value;
  if(!DESKS.includes(deviceNumber)||fault(deviceNumber)){toast("该设备已处于故障状态。","error");return}
  button.disabled=true;
  try{
    // The database trigger marks the shared device faulty in this same transaction.
    let {data:issue,error}=await supabase.from("issue_reports")
      .insert({device_number:deviceNumber,reporter_id:user().id,description,status:"faulty"})
      .select("id,device_number,reporter_id,description,status,created_at").single();
    if(error){toast(cloudError(error),"error");return}
    if(!db.faults.some(item=>item.id===issue.id))db.faults.push({id:issue.id,deskNo:deviceNumber,description:issue.description,reporter:user().name,reporterId:issue.reporter_id,status:issue.status,createdAt:issue.created_at});
    let device=db.devices.find(item=>item.number===deviceNumber);if(device)device.status="faulty";
    desks.delete(deviceNumber);bookingRequestKey=null;faultResponseMetrics=null;
    q("#faultForm").reset();q("#faultModal").close();
    toast("故障已上报，设备已停止预约。");render();
    // A failed follow-up read must not erase the successful report or re-enable it.
    try{await refreshCloud(false)}catch(error){console.warn("Post-report refresh failed",error)}
    render();
  }catch(error){toast(cloudError(error),"error")}
  finally{button.disabled=false}
}

/** Gives the short maintenance form local feedback before it reaches the database. */
function validateFaultDescription(){
  let field=q("#faultText"),error=q("#faultError"),valid=field.value.trim().length<=120;
  error.hidden=valid;if(!valid)error.textContent=tr("故障说明可不填写，填写时最多 120 个字符。");
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
  const target=event.target,button=target.querySelector('button[type="submit"],button.primary');
  if(button.disabled)return;
  const form=new FormData(target),loginId=form.get("loginId").trim(),name=form.get("name").trim(),password=form.get("password");
  const invalid=registrationError(name,password,language);
  if(invalid){toast(invalid,"error");return}
  let email;
  try{email=studentEmail(loginId)}catch(error){toast(language==="en"?error.message:"学号需要 2–20 位字母、数字、点、横线或下划线。","error");return}
  button.disabled=true;
  try{
    const {data,error}=await supabase.auth.signUp({email,password,options:{data:{login_id:loginId,display_name:name}}});
    if(error){toast(cloudError(error),"error");return}
    if(!data.session){toast(language==="en"?"Account created but email confirmation is required. Contact the project administrator.":"账号已创建，但云端要求邮箱验证，请联系项目管理员。","error");return}
    if(await refreshCloud(false)){page="dashboard";toast("注册成功。");startSession()}
  }catch(error){toast(cloudError(error),"error")}
  finally{button.disabled=false}
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
  const target=event.target,button=target.querySelector('button.primary');
  if(button.disabled)return;
  const form=new FormData(target),name=form.get("name").trim(),email=form.get("email").trim().toLowerCase(),password=form.get("password"),inviteCode=form.get("inviteCode").trim().toUpperCase();
  const invalid=registrationError(name,password,language);
  if(invalid){toast(invalid,"error");return}
  if(!email||!inviteCode){toast(language==="en"?"Complete every field to continue.":"请填写邮箱和管理员邀请码。","error");return}
  button.disabled=true;
  try{
    const {data,error}=await supabase.auth.signUp({email,password,options:{data:{login_id:email,display_name:name,admin_invite_code:inviteCode}}});
    if(error){toast(cloudError(error),"error");return}
    if(!data.session){toast(language==="en"?"Check your email to confirm your account before signing in.":"请先通过邮箱验证账号，再登录。","error");return}
    if(await refreshCloud(false)){
      if(!admin()){await logoutCloud();toast(language==="en"?"The invitation was not accepted.":"管理员邀请码未被接受，请联系管理员。","error");return}
      page="dashboard";toast(language==="en"?"Administrator account created.":"管理员账号注册成功。");startSession();
    }
  }catch(error){toast(cloudError(error),"error")}
  finally{button.disabled=false}
}

/** Generates one time-limited code through the administrator-only database function. */
async function createAdministratorInvite(event){
  event.preventDefault();
  if(!admin()||inviteGenerationPending)return;
  if(q("#inviteCodeOutput").value&&!window.confirm(tr("生成新的邀请码前，请确认已保存当前邀请码。是否继续？")))return;
  let days=Number(q("#inviteDays").value),button=event.target.querySelector('button.primary');
  if(![1,7,30].includes(days)){toast("请选择 1、7 或 30 天有效期。","error");return}
  let actorId=user().id;inviteGenerationPending=true;button.disabled=true;let originalLabel=button.textContent;
  button.textContent=tr("生成中…");q("#inviteDays").disabled=true;
  try{
    let {data,error}=await supabase.rpc("create_admin_invite_for_days",{p_valid_days:days});
    if(error)throw error;
    if(user()?.id!==actorId||!admin())return;
    let invitation=Array.isArray(data)?data[0]:data;
    if(!invitation||!/^NJUPT-[A-F0-9]{12}$/.test(invitation.invite_code||"")||!Number.isFinite(Date.parse(invitation.expires_at)))throw new Error(tr("邀请码生成结果异常，请重试。"));
    q("#inviteCodeOutput").value=invitation.invite_code;
    if(q("#inviteExpiry"))q("#inviteExpiry").textContent=`${tr("到期时间")}：${new Date(invitation.expires_at).toLocaleString(locale(),{hour12:false})}`;
    q("#inviteResult").hidden=false;
    toast("邀请码已生成，请及时保存。");
  }catch(error){toast(cloudError(error),"error")}
  finally{inviteGenerationPending=false;button.disabled=false;button.textContent=q("#inviteCodeOutput").value?tr("生成新邀请码"):originalLabel;q("#inviteDays").disabled=false}
}

function openInviteDialog(){
  if(!admin())return;
  q("#inviteResult").hidden=!q("#inviteCodeOutput").value;
  q("#inviteForm button.primary").disabled=inviteGenerationPending;
  q("#inviteModal").showModal();
}
async function copyInvitationCode(){
  let code=q("#inviteCodeOutput").value;if(!code){toast("请先生成邀请码。","error");return}
  try{await navigator.clipboard.writeText(code);toast("邀请码已复制。")}catch{q("#inviteCodeOutput").select();toast("复制失败，请手动复制邀请码。","error")}
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
  const target=event.target,button=target.querySelector('button.primary');
  if(button.disabled)return;
  const form=new FormData(target),identity=form.get("loginId").trim(),password=form.get("password");
  button.disabled=true;
  try{
    let email;
    if(identity.includes("@"))email=identity;
    else{
      const {data,error}=await supabase.rpc("resolve_student_auth_email",{entered_login_id:identity});
      // Older deployments have no lookup RPC. Their existing student accounts
      // already use this canonical Auth email; password verification remains server-side.
      if(error&&!['PGRST202','42883'].includes(error.code))throw error;
      email=error?studentEmail(identity):data;
      if(!email)throw {code:'invalid_credentials'};
    }
    const {error}=await supabase.auth.signInWithPassword({email,password});
    if(error){toast(cloudError(error),"error");return}
    if(await refreshCloud(false)){page="dashboard";startSession()}
  }catch(error){toast(cloudError(error),"error")}
  finally{button.disabled=false}
}

async function logoutCloud(){
  try{
    const {error}=await supabase.auth.signOut();
    if(error)throw error;
    db={users:[],devices:[],bookings:[],faults:[],session:null};
    adminBookingFilters={date:"",status:"",query:""};
    q("#inviteCodeOutput").value="";if(q("#inviteExpiry"))q("#inviteExpiry").textContent="";q("#inviteResult").hidden=true;q("#inviteModal").close();
    desks.clear();adminMetrics=null;agentResponse=null;faultImpactPlan=null;faultResponseMetrics=null;scheduleCandidates=[];
    clearTimeout(scheduleStatusRefresh.timer);
    render();
  }catch(error){toast(cloudError(error),"error")}
}

function subscribeCloud(){
  let timer;
  supabase.channel("lab-reservation-live")
    .on("postgres_changes",{event:"*",schema:"public",table:"devices"},queue)
    .on("postgres_changes",{event:"*",schema:"public",table:"reservations"},queue)
    .on("postgres_changes",{event:"*",schema:"public",table:"issue_reports"},queue)
    .on("postgres_changes",{event:"*",schema:"public",table:"reservation_usage_records"},queue)
    .on("postgres_changes",{event:"*",schema:"public",table:"maintenance_windows"},queue)
    .subscribe();
  function queue(){clearTimeout(timer);timer=setTimeout(()=>{if(!document.body.classList.contains("booking-success"))refreshCloud()},150)}
  window.addEventListener("online",queue);
  document.addEventListener("visibilitychange",()=>{if(document.visibilityState!=="hidden"&&user())queue()});
  setInterval(()=>{
    if(user()&&document.visibilityState!=="hidden"&&["dashboard","lab","faults","reservations","me"].includes(page)&&!document.activeElement?.closest("#content input, #content textarea, #content select"))queue();
  },15000);
}

async function bootCloud(){
  q("#registerForm").onsubmit=registerCloud;
  q("#adminRegisterForm").onsubmit=registerAdministrator;
  q("#loginForm").onsubmit=loginCloud;
  q("#faultForm").onsubmit=submitCloudIssue;
  q("#repairForm").onsubmit=submitRepair;
  q("#faultPlanForm").onsubmit=executeFaultIsolation;
  q("#closeFaultPlan").onclick=()=>q("#faultPlanModal").close();
  q("#logout").onclick=logoutCloud;
  q("#showAdminRegister").onclick=()=>showAuthForm("admin");
  q("#showStudentRegister").onclick=()=>showAuthForm("student");
  q("#openInvite").onclick=openInviteDialog;
  q("#closeInviteModal").onclick=()=>q("#inviteModal").close();
  q("#inviteForm").onsubmit=createAdministratorInvite;
  q("#copyInviteCode").onclick=copyInvitationCode;
  render();
  try{await refreshCloud();subscribeCloud()}catch(error){toast(cloudError(error),"error")}
}

playSiteEntry();
bootCloud();

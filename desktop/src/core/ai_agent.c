/*
 * ai_agent.c —— AI 预约助手客户端
 *
 * 与云端函数 deepseek-booking-agent 交互：
 *   - lab_agent_payload：按用户角色选择助手类型（学生 booking_planner / 管理员 admin_analyst），
 *     把用户输入、界面当前选择、会话编号与对话历史组装成请求；
 *   - LabAgentState：保存会话编号、最近一次回复（含候选方案）和最多 20 条对话历史，
 *     并与当前登录用户绑定，切换账号时清空，属于旧账号的迟到回复会被丢弃；
 *   - 两步确认：助手只返回候选方案，用户选定后由 lab_agent_confirmation 生成
 *     {proposalId, idempotencyKey}；同一方案重复确认沿用原幂等键，云端不会重复预约。
 *
 * 候选方案由云端根据数据库中的真实空位生成；DeepSeek 密钥只保存在云端函数中，客户端不接触。
 */
#include "client.h"   // 共用的数据结构和函数声明
#include <stdio.h>    // snprintf
#include <string.h>   // strcmp / strlen / memset

/* ---------- 请求组装 ----------
 * 功能："智能助手"页面把用户的话、当前选择和对话历史打包发给云端 AI。
 * 请求格式和网页版完全一样，两边共用同一个云端 AI 服务。
 */

// 打包发给 AI 的请求。返回的 JSON 由调用者释放；参数不合法返回 NULL
cJSON *lab_agent_payload(const LabSession *session, const char *message, const char *date,
                        int start, int end, const int *devices, size_t count,
                        const char *request_id, const char *session_id,
                        const cJSON *history, const cJSON *confirmation) {
    // 步骤 1：参数检查：必要的指针不能为空，时间范围和设备号必须合法
    if (!session || !date || !message || !request_id || !*request_id || count > 40 ||
        (count && !devices) || start < 8 || end > 17 || start >= end) return NULL;
    for (size_t i=0;i<count;i++) if (devices[i]<1 || devices[i]>40) return NULL;
    cJSON *p=cJSON_CreateObject(), *input=cJSON_CreateObject();   // p 是整个请求，input 是"用户当前在界面上选了什么"
    // 步骤 2：基本信息
    cJSON_AddStringToObject(p,"requestId",request_id);            // 本次请求的唯一编号，防止重复处理
    cJSON_AddStringToObject(p,"agentType",!strcmp(session->role,"admin")?"admin_analyst":"booking_planner"); // 管理员用"数据分析"助手，学生用"预约规划"助手
    cJSON_AddStringToObject(p,"language","zh");                   // 要求用中文回答
    cJSON_AddStringToObject(p,"message",message);                 // 用户输入的那句话
    if(session_id && *session_id)cJSON_AddStringToObject(p,"sessionId",session_id);   // 有对话编号就带上，AI 才能接着上一轮聊
    // 步骤 3：对话历史和确认信息用 cJSON_Duplicate 深拷贝一份再挂上去。
    //         原件属于 LabAgentState，如果直接挂进 p，删除 p 时会把原件也删掉
    cJSON_AddItemToObject(p,"history",cJSON_IsArray(history)?cJSON_Duplicate(history,1):cJSON_CreateArray());
    if(confirmation)cJSON_AddItemToObject(p,"confirmation",cJSON_Duplicate(confirmation,1));   // 用户点了"选择此方案"时才有
    // 步骤 4：告诉 AI 用户当前在界面上的选择
    if(!strcmp(session->role,"admin")) {        // 管理员：只需要统计的日期范围
        cJSON_AddStringToObject(input,"startDate",date);
        cJSON_AddStringToObject(input,"endDate",date);
    } else if(count) {                          // 学生且选了设备：把日期、开始小时、时长、设备列表都发过去
        cJSON *selection=cJSON_CreateObject(),*numbers=cJSON_CreateArray(),*hours=cJSON_CreateArray();
        cJSON_AddStringToObject(selection,"date",date);
        cJSON_AddItemToArray(hours,cJSON_CreateNumber(start));
        cJSON_AddItemToObject(selection,"startHours",hours);
        cJSON_AddNumberToObject(selection,"durationHours",end-start);
        cJSON_AddNumberToObject(selection,"requiredDeviceCount",(double)count);
        for(size_t i=0;i<count;i++)cJSON_AddItemToArray(numbers,cJSON_CreateNumber(devices[i]));
        cJSON_AddItemToObject(selection,"preferredDevices",numbers);
        cJSON_AddItemToObject(input,"bookingSelection",selection);
    } else cJSON_AddNullToObject(input,"bookingSelection");   // 学生没选设备：明确写 null，让 AI 自己推荐
    cJSON_AddItemToObject(p,"input",input);
    return p;
}

/* ---------- 对话状态 ----------
 * 功能：记住和 AI 聊了什么、AI 给了哪些方案、用户选了哪个方案，并把它们变成界面上的文字。
 * 一轮对话的顺序：bind 绑定用户 → payload 打包请求 → （main.c 发请求）→ accept 记录回复 →
 *               proposals / option 显示方案 → confirmation 确认方案 → 再发一次请求完成预约
 */

// 记录 AI 回复：owner 是发出请求时的用户，message 是用户当时说的话，response 是 AI 的完整回复
void lab_agent_accept(LabAgentState *s,const char *owner,const char *message,const cJSON *response) {
    // 步骤 1：安全检查。请求发出后用户可能已经切换了账号，这条迟到的回复属于上一个人，丢弃
    if(strcmp(s->owner,owner))return;
    if(!cJSON_IsObject(response))return;              // 回复格式不对也丢弃
    // 步骤 2：保存云端分配的对话编号，下一轮请求带上它，AI 才知道是同一段对话
    const char *session=lab_str(response,"sessionId");if(*session)lab_copy_text(s->session,sizeof(s->session),session);
    // 步骤 3：深拷贝保存完整回复（里面有候选方案）。response 属于调用者，调用者稍后会删除它，所以必须复制一份
    cJSON_Delete(s->response);s->response=cJSON_Duplicate(response,1);
    // 步骤 4：确定要显示的回答：有错误就显示第一条错误，否则显示 AI 的回答
    const cJSON *errors=cJSON_GetObjectItemCaseSensitive(response,"errors");
    const char *reply=lab_str(response,"message");
    if(cJSON_GetArraySize(errors))reply=lab_str(cJSON_GetArrayItem(errors,0),"message");
    // 步骤 5：把"用户说的话"和"AI 的回答"依次追加到历史里，每条记录形如 {"role":"user","message":"..."}
    if(message && *message){
        cJSON *entry=cJSON_CreateObject();
        cJSON_AddStringToObject(entry,"role","user");
        cJSON_AddStringToObject(entry,"message",message);
        cJSON_AddItemToArray(s->history,entry);
    }
    if(*reply){
        cJSON *entry=cJSON_CreateObject();
        cJSON_AddStringToObject(entry,"role","assistant");
        cJSON_AddStringToObject(entry,"message",reply);
        cJSON_AddItemToArray(s->history,entry);
    }
    // 步骤 6：只保留最近 20 条，从最旧的（下标 0）开始删，避免历史无限增长、请求越来越大
    while(cJSON_GetArraySize(s->history)>20)cJSON_DeleteItemFromArray(s->history,0);
    // 步骤 7：如果这次回复表示"预约已确认"，清掉选中的方案和确认编号，下一次是全新的确认
    if(!strcmp(lab_str(response,"outcome"),"confirmed")){s->proposal[0]=0;s->confirmation_key[0]=0;}
}

// 用户在下拉框选中第 option 个方案并点"选择此方案"，生成确认信息 {proposalId, idempotencyKey}
cJSON *lab_agent_confirmation(LabAgentState *s,int option,const char *new_key) {
    // 步骤 1：按下标取出方案和它的编号
    const cJSON *p=cJSON_GetArrayItem(lab_agent_proposals(s),option);const char *id=lab_str(p,"proposalId");
    // 步骤 2：编号和新的防重复编号都必须存在且放得下
    if(!*id || strlen(id)>=sizeof(s->proposal) || !new_key || !*new_key || strlen(new_key)>=sizeof(s->confirmation_key))return NULL;
    // 步骤 3：关键设计：只有换了一个方案，才启用新的防重复编号；
    //         同一个方案重复点确认（例如上次超时），沿用旧编号，云端就知道是同一次确认，不会重复预约
    if(strcmp(s->proposal,id)){lab_copy_text(s->proposal,sizeof(s->proposal),id);lab_copy_text(s->confirmation_key,sizeof(s->confirmation_key),new_key);}
    // 步骤 4：组装确认信息返回，调用者负责释放
    cJSON *result=cJSON_CreateObject();cJSON_AddStringToObject(result,"proposalId",s->proposal);
    cJSON_AddStringToObject(result,"idempotencyKey",s->confirmation_key);return result;
}

// 绑定用户：每次使用助手前调用。发现换了账号，就把上一个人的对话清掉，防止 A 看到 B 的聊天记录
void lab_agent_bind(LabAgentState *s,const char *owner) {
    if(strcmp(s->owner,owner)){                       // 当前对话的主人不是这个用户
        lab_agent_clear(s);                           // 清空旧对话
        lab_copy_text(s->owner,sizeof(s->owner),owner);   // 记下新主人
    }
    if(!s->history)s->history=cJSON_CreateArray();    // 还没有历史数组就建一个空的
}

// 清空对话：退出登录时调用
void lab_agent_clear(LabAgentState *s) {
    cJSON_Delete(s->history);     // 释放对话历史
    cJSON_Delete(s->response);    // 释放最近一次回复
    memset(s,0,sizeof(*s));       // 所有字段清零，指针变 NULL
}


/* ---------- 内部辅助函数 ---------- */

// 取出 AI 给的候选方案（借用指针，不能删除）。已经确认过的回复不再显示旧方案
const cJSON *lab_agent_proposals(const LabAgentState *s) {
    if(!strcmp(lab_str(s->response,"outcome"),"confirmed"))return NULL;
    return cJSON_GetObjectItemCaseSensitive(s->response,"proposals");
}

// 把对话历史拼成界面文本，例如"我：明天下午有空位吗\n\n助手：……\n\n"
void lab_agent_transcript(const LabAgentState *s,char *out,size_t cap) {
    if(!cap)return;              // 缓冲区大小为 0，连 \0 都放不下，直接返回
    out[0]=0;                    // 先设成空字符串
    size_t used=0;               // 已经写了多少字节
    const cJSON *entry;
    // 逐条追加：每次写到 out+used 处，最多写 cap-used 字节。
    // snprintf 返回"本来想写多少"，如果 ≥ 剩余空间说明被截断了，停止追加，保证不越界
    cJSON_ArrayForEach(entry,s->history){int n=snprintf(out+used,cap-used,"%s：%s\n\n",!strcmp(lab_str(entry,"role"),"user")?"我":"助手",lab_str(entry,"message"));if(n<0 || (size_t)n>=cap-used)break;used+=(size_t)n;}
    if(!*out)snprintf(out,cap,"告诉我你的预约需求。");   // 还没聊过：显示引导语
}

// 把一个方案写成下拉框里的一行，例如"方案 1 · 2026-11-05 14:00 · 设备 #03 #04"
void lab_agent_option(const cJSON *proposal,int number,char *out,size_t cap) {
    if(!cap)return;
    size_t used=0;
    int n=snprintf(out,cap,"方案 %d",number);if(n<0 || (size_t)n>=cap)return;used=(size_t)n;   // 步骤 1：写"方案 N"
    // 步骤 2：方案里可能包含多个时段，存在 candidate.items 数组里
    const cJSON *items=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(proposal,"candidate"),"items"),*item;
    // 步骤 3：每个时段写" · 日期 时间 · 设备"，再把这个时段的每台设备写成" #03"。
    //         每次追加后都检查是否写满，写满就停（和上面 transcript 的写法相同）
    cJSON_ArrayForEach(item,items){n=snprintf(out+used,cap-used," · %s %02d:00 · 设备",lab_str(item,"reservation_date"),lab_int(item,"start_hour"));if(n<0 || (size_t)n>=cap-used)return;used+=(size_t)n;
      const cJSON *device;cJSON_ArrayForEach(device,cJSON_GetObjectItemCaseSensitive(item,"device_numbers")){n=snprintf(out+used,cap-used," #%02d",device->valueint);if(n<0 || (size_t)n>=cap-used)return;used+=(size_t)n;}}
}

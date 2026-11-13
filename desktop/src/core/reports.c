/*
 * reports.c —— 统计与导出
 *
 * 管理员功能的客户端部分：查询使用统计（v3_admin_usage_metrics）、把统计结果排版为文字报表、
 * 将已加载的预约或故障记录导出为 CSV。统计数字全部由云端计算，客户端只负责展示，
 * 保证与网页端口径一致。
 */
#include "client.h"
#include <stdarg.h>   // va_list：实现可变参数的 report_append
#include <stdio.h>    // snprintf / fputc
#include <string.h>   // strlen / strchr

/* ---------- 管理员统计查询 ---------- */
// 管理员查询使用统计。device 为 0 表示全部设备；返回的 JSON 由调用者释放
cJSON *lab_usage_metrics(LabClient *c,const char *start_date,const char *end_date,int device) {
    // 步骤 1：检查参数
    if(!start_date || !end_date || device<0 || device>40) {
        snprintf(c->error,sizeof(c->error),"Invalid metrics filter");return NULL;
    }
    // 步骤 2：组织参数，"全部设备"用 null 表示
    cJSON *p=cJSON_CreateObject();
    cJSON_AddStringToObject(p,"p_start_date",start_date);
    cJSON_AddStringToObject(p,"p_end_date",end_date);
    if(device)cJSON_AddNumberToObject(p,"p_device_number",device);
    else cJSON_AddNullToObject(p,"p_device_number");
    // 步骤 3：调用云端统计函数
    cJSON *r=lab_rpc(c,"v3_admin_usage_metrics",p);cJSON_Delete(p);
    // 步骤 4：检查回复里至少有"实际使用分钟"和"预约分钟"两个数字，缺了就说明回复不完整，不能拿去出报表
    if(r && (!cJSON_IsObject(r) || !cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(r,"actual_used_minutes")) ||
             !cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(r,"booked_device_minutes")))) {
        cJSON_Delete(r);snprintf(c->error,sizeof(c->error),"统计响应不完整，请刷新重试。");return NULL;
    }
    return r;
}

/* ---------- 导出 CSV ----------
 * 功能：管理员点"导出完整预约 / 导出完整故障"，把已加载的全部记录保存成 Excel 能直接打开的 CSV 文件。
 */

// 写一个单元格：例如 abc"d 写成 "abc""d"
static void export_cell(FILE *file,const char *value) {
    // 步骤 1：写开头的双引号。内容以 = + - @ 制表符 回车 开头时，再加一个单引号：
    //         Excel 会把 "=..." 当成公式执行，恶意内容可能借此运行命令（CSV 注入），加单引号就只当文字显示
    fputc('"',file);if(strchr("=+-@\t\r",*value)&&*value)fputc(39,file);
    // 步骤 2：逐字符写出内容；遇到双引号就多写一个（CSV 标准规定的转义方式）；最后写结尾双引号
    for(;*value;value++){if(*value=='"')fputc('"',file);fputc((unsigned char)*value,file);}fputc('"',file);
}

// 导出整张表：issues 为 1 导出故障，为 0 导出预约。文件由调用者打开和关闭，这里只负责写
int lab_export_csv(const LabModel *model,int issues,FILE *file) {
    if(!file || !model)return 0;
    // 步骤 1：两种表各自要导出的列
    static const char *book[]={"id","reservation_date","start_hour","device_number","user_id","status","created_at"};
    static const char *fault[]={"id","device_number","reporter_id","description","status","created_at","resolved_at","resolved_by","resolution_note"};
    // 步骤 2：选定列表和列数；文件开头写 3 个字节的 UTF-8 BOM 标记，否则 Excel 打开中文会乱码
    const char **fields=issues?fault:book;int count=issues?9:7;fwrite("\xef\xbb\xbf",1,3,file);
    // 步骤 3：写表头行（列名用逗号隔开），行尾用 \r\n（Windows 换行）
    for(int i=0;i<count;i++){if(i)fputc(',',file);export_cell(file,fields[i]);}fputs("\r\n",file);
    // 步骤 4：每条记录写一行
    const cJSON *row;cJSON_ArrayForEach(row,(issues?model->issues:model->bookings)){
        for(int i=0;i<count;i++){if(i)fputc(',',file);const cJSON *value=cJSON_GetObjectItemCaseSensitive(row,fields[i]);
            // 字符串直接写；数字先转成文字（%.17g 保证不丢精度）；缺失或 null 写空
            char number[64];const char *text="";if(cJSON_IsString(value))text=value->valuestring;
            else if(cJSON_IsNumber(value)){snprintf(number,sizeof(number),"%.17g",value->valuedouble);text=number;}
            export_cell(file,text);
        }fputs("\r\n",file);
    }return !ferror(file);   // 步骤 5：检查写文件过程中有没有出错（例如磁盘满），没错返回 1
}

/* ---------- 统计报表排版 ----------
 * 功能：管理员点"统计报表"后弹出的文字报表。云端已经算好了所有数字，这里只负责排版成可读的文字，
 *       不在客户端重新计算，保证和网页版的统计口径一致。
 */

// 往报表末尾追加一段格式化文字，用法同 printf；超出容量时截断，不会越界
static void report_append(char *out,size_t cap,const char *format,...) {
    size_t used=strlen(out);          // 找到当前末尾
    if(used>=cap)return;              // 已经写满，不再追加
    va_list args;
    va_start(args,format);            // 从 format 之后开始取
    vsnprintf(out+used,cap-used,format,args);   // 带容量限制地写到末尾，超长会被截断而不是越界
    va_end(args);                     // 取完参数后必须调用，和 va_start 成对出现
}

// 写一行指标，例如"预约率：35.20%"。percent 为 1 时把 0.352 乘 100 显示成百分比；
// 云端没给出数字（例如数据不足时为 null）就显示"暂无完整数据"，不瞎编一个 0
static void report_value(char *out,size_t cap,const cJSON *data,const char *key,const char *label,int percent){
 const cJSON *v=cJSON_GetObjectItemCaseSensitive(data,key);if(cJSON_IsNumber(v))report_append(out,cap,"%s：%.2f%s\r\n",label,v->valuedouble*(percent?100:1),percent?"%":"");else report_append(out,cap,"%s：暂无完整数据\r\n",label);
}

// 生成整份报表，写进 out（大小 cap）
void lab_metrics_report(const cJSON *data,char *out,size_t cap) {
 if(!cap)return;
 out[0]=0;                            // 从空字符串开始追加
 // 步骤 1：标题行：统计的日期范围
 const cJSON *range=cJSON_GetObjectItemCaseSensitive(data,"range");
 report_append(out,cap,"实验室使用情况 %s — %s\r\n（北京时间；时长单位为设备分钟）\r\n\r\n",lab_str(range,"start_date"),lab_str(range,"end_date"));
 // 步骤 2：总览指标。字段名数组和中文名数组一一对应，用一个循环写完；前 4 个是时长，从第 5 个（下标 4）起是比率
 const char *keys[]={"booked_device_minutes","actual_used_minutes","provisional_used_minutes","capacity_device_minutes","reservation_rate","actual_usage_rate","usage_gap_rate","no_show_rate","cancellation_rate"};
 const char *labels[]={"预约时长","实际使用时长","进行中临时时长","可用时长","预约率","实际使用率","预约与使用率差异","爽约率","取消率"};
 for(int i=0;i<9;i++)report_value(out,cap,data,keys[i],labels[i],i>=4);
 // 步骤 3：数据完整度：有多少预约有完整的签到签退记录，管理员据此判断统计是否可信
 report_value(out,cap,cJSON_GetObjectItemCaseSensitive(data,"data_completeness"),"complete_usage_record_coverage","记录完整率",1);
 // 步骤 4：逐台设备：实际使用分钟 / 可用分钟 / 利用率
 const cJSON *row;report_append(out,cap,"\r\n每台设备：使用分钟 / 已知可用分钟 / 利用率\r\n");
 cJSON_ArrayForEach(row,cJSON_GetObjectItemCaseSensitive(data,"device_utilization")){
 report_append(out,cap,"设备 #%02d  %.2f / %.2f  ",lab_int(row,"device_number"),cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"actual_used_minutes")),cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"known_capacity_minutes")));report_value(out,cap,row,"actual_usage_rate","利用率",1);}
 // 步骤 5：逐个时段（8:00–9:00 …）：管理员可以看出哪些时段最忙
 report_append(out,cap,"\r\n每个时段：使用分钟 / 已知可用分钟 / 利用率\r\n");
 cJSON_ArrayForEach(row,cJSON_GetObjectItemCaseSensitive(data,"slot_usage_trends")){
 report_append(out,cap,"%02d:00–%02d:00  %.2f / %.2f  ",lab_int(row,"start_hour"),lab_int(row,"start_hour")+1,cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"actual_used_minutes")),cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"known_capacity_minutes")));report_value(out,cap,row,"actual_usage_rate","利用率",1);}
}

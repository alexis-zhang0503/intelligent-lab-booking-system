//【答辩注释】两端共用的界面颜色、状态翻译、时间显示和预览数据。预览数据只用于截图与启动检查，不代表真实云端数据。
//【讲解 L1】按 LAB_UI_THEME_H 是否已定义选择编译分支，避免重复定义或适配平台。
#ifndef LAB_UI_THEME_H
//【讲解 L2】定义 LAB_UI_THEME_H，用于编译配置或头文件保护。
#define LAB_UI_THEME_H
//【讲解 L3】引入 time.h 的类型和函数声明。
#include <time.h>
/* Native equivalents of the live web workspace tokens. No HTML renderer. */
//【讲解 L5】定义 UI_BG，当前值为 "#f7f7f5"。
#define UI_BG "#f7f7f5"
//【讲解 L6】定义 UI_SURFACE，当前值为 "#ffffff"。
#define UI_SURFACE "#ffffff"
//【讲解 L7】定义 UI_INK，当前值为 "#23252b"。
#define UI_INK "#23252b"
//【讲解 L8】定义 UI_MUTED，当前值为 "#686b73"。
#define UI_MUTED "#686b73"
//【讲解 L9】定义 UI_BORDER，当前值为 "#e8e8e3"。
#define UI_BORDER "#e8e8e3"
//【讲解 L10】定义 UI_ACCENT，当前值为 "#5c63d6"。
#define UI_ACCENT "#5c63d6"
//【讲解 L11】定义 UI_SOFT，当前值为 "#eff0ff"。
#define UI_SOFT "#eff0ff"
//【讲解 L12】定义 UI_SUCCESS，当前值为 "#398563"。
#define UI_SUCCESS "#398563"
//【讲解 L13】定义 UI_DANGER，当前值为 "#bd5252"。
#define UI_DANGER "#bd5252"
//【讲解 L14】定义 UI_SIDEBAR，当前值为 228。
#define UI_SIDEBAR 228
//【讲解 L15】定义 UI_RADIUS，当前值为 16。
#define UI_RADIUS 16
//【讲解 L16】定义 UI_DEVICE_COLUMNS，当前值为 8。
#define UI_DEVICE_COLUMNS 8
//【讲解 L17】函数 ui_page_title：按页面编号返回统一中文标题。
static const char *ui_page_title(int page) {
    //【讲解 L18】结束当前函数并返回 page == 0 ? "实验室预约" : page == 1 ? "我的" : page == 2 ? "故障记录" : page == 3 ? "用户管理" : "智能助手"；具体成功或失败含义见函数说明。
    return page == 0 ? "实验室预约" : page == 1 ? "我的" : page == 2 ? "故障记录" : page == 3 ? "用户管理" : "智能助手";
//【讲解 L19】结束当前代码块。
}
//【讲解 L20】函数 ui_status：将云端状态码转换为中文展示，未识别值保留原文。
static const char *ui_status(const char *s) {
    //【讲解 L21】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(s,"confirmed") || !strcmp(s,"reserved")) return "已预约";
    //【讲解 L22】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(s,"in_use")) return "使用中";
    //【讲解 L23】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(s,"completed")) return "已完成";
    //【讲解 L24】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(s,"cancelled")) return "已取消";
    //【讲解 L25】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(s,"no_show")) return "未到场";
    //【讲解 L26】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(s,"resolved")) return "已修复";
    //【讲解 L27】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(s,"reported") || !strcmp(s,"open")) return "待处理";
    //【讲解 L28】结束当前函数并返回 s；具体成功或失败含义见函数说明。
    return s;
//【讲解 L29】结束当前代码块。
}
//【讲解 L30】函数 ui_usage：按 reservation_id 查找对应的签到签退记录，返回模型内部借用指针。
static const cJSON *ui_usage(const LabModel *m,const cJSON *booking) {
    //【讲解 L31】遍历 JSON 数组。
    const cJSON *u; cJSON_ArrayForEach(u,m->usage)
        //【讲解 L32】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        if (!strcmp(lab_str(u,"reservation_id"),lab_str(booking,"id"))) return u;
    //【讲解 L33】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
    return NULL;
//【讲解 L34】结束当前代码块。
}
//【讲解 L35】函数 ui_summary：根据当前用户非取消预约统计当日记录、常用设备及时段；累计数按一条设备小时预约计数。
static void ui_summary(const LabModel *m,const LabSession *s,const char *date,char *today,size_t tn,char *prefs,size_t pn) {
    //【讲解 L36】声明或更新 devices（设备列表）；按本行表达式保存对应状态。
    int count=0,total=0,devices[40]={0},hours[9]={0},bestD=0,bestH=0; const cJSON *b;
    //【讲解 L37】遍历 JSON 数组。
    cJSON_ArrayForEach(b,m->bookings) {
        //【讲解 L38】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        if(strcmp(lab_str(b,"user_id"),s->uid)||!strcmp(lab_str(b,"status"),"cancelled"))continue;
        //【讲解 L39】比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        ++total;if(!strcmp(lab_str(b,"reservation_date"),date))++count;
        //【讲解 L40】调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
        int d=lab_int(b,"device_number")-1,h=lab_int(b,"start_hour")-8;
        //【讲解 L41】检查本行条件，满足后执行紧随其后的语句或代码块。
        if(d>=0&&d<40)++devices[d];
        //【讲解 L42】检查本行条件，满足后执行紧随其后的语句或代码块。
        if(h>=0&&h<9)++hours[h];
    //【讲解 L43】结束当前代码块。
    }
    //【讲解 L44】按本行初值、条件和步长循环。
    for(int i=1;i<40;i++)if(devices[i]>devices[bestD])bestD=i;
    //【讲解 L45】按本行初值、条件和步长循环。
    for(int i=1;i<9;i++)if(hours[i]>hours[bestH])bestH=i;
    //【讲解 L46】按容量格式化 UTF-8 文本。
    snprintf(today,tn,"%s · 今天 %d 个预约时段",date,count);
    //【讲解 L47】检查本行条件，满足后执行紧随其后的语句或代码块；按容量格式化 UTF-8 文本。
    if(total)snprintf(prefs,pn,"常用设备 #%02d    常用时段 %02d:00    累计预约 %d 小时",bestD+1,bestH+8,total);
    //【讲解 L48】处理另一分支；按容量格式化 UTF-8 文本。
    else snprintf(prefs,pn,"暂未形成预约偏好 · 选好设备和时段后，会显示在这里。");
//【讲解 L49】结束当前代码块。
}
//【讲解 L50】函数 ui_timestamp：解析云端 ISO 时间文本和时区偏移，转换为 Unix 毫秒；这是面向当前接口格式的解析器。
static int64_t ui_timestamp(const char *value) {
    //【讲解 L51】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int y,mo,d,h,mi,se;
    //【讲解 L52】检查本行条件，满足后执行紧随其后的语句或代码块；计算字符串字节长度。
    if(strlen(value)<19)return 0;
    //【讲解 L53】检查本行条件，满足后执行紧随其后的语句或代码块；从文本解析字段。
    if(sscanf(value,"%d-%d-%dT%d:%d:%d",&y,&mo,&d,&h,&mi,&se)!=6)return 0;
    //【讲解 L54】按容量格式化 UTF-8 文本。
    char date[32];snprintf(date,sizeof(date),"%04d-%02d-%02d",y,mo,d);
    //【讲解 L55】创建 JSON 对象；写入字符串字段；写入数值字段（字段 reservation_date）。
    cJSON *b=cJSON_CreateObject();cJSON_AddStringToObject(b,"reservation_date",date);cJSON_AddNumberToObject(b,"start_hour",8);
    //【讲解 L56】调用 lab_booking_start，校验日期、闰年和 08–16 点起始小时；释放 JSON 对象及其子节点。
    int64_t t=lab_booking_start(b)+(int64_t)(h*3600+mi*60+se)*1000;cJSON_Delete(b);
    //【讲解 L57】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    const char *z=value+19;while(*z=='.'||(*z>='0'&&*z<='9'))++z;
    //【讲解 L58】从文本解析字段。
    int zh=0,zm=0;if((*z=='+'||*z=='-')&&sscanf(z+1,"%d:%d",&zh,&zm)>=1)t-=(int64_t)(*z=='+'?1:-1)*(zh*60+zm)*60000;
    //【讲解 L59】结束当前函数并返回 t；具体成功或失败含义见函数说明。
    return t;
//【讲解 L60】结束当前代码块。
}
//【讲解 L61】函数 ui_time：将云端时间转换为北京时间文本；无法解析时显示占位符。
static void ui_time(const char *value,char *out,size_t n) {
    //【讲解 L62】调用 ui_timestamp，解析云端 ISO 时间文本和时区偏移，转换为 Unix 毫秒；按容量格式化 UTF-8 文本。
    int64_t t=ui_timestamp(value);if(!t){snprintf(out,n,"—");return;}
    //【讲解 L63】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    time_t seconds=(time_t)(t/1000+8*3600);struct tm *tm=gmtime(&seconds);
    //【讲解 L64】检查本行条件，满足后执行紧随其后的语句或代码块；按容量格式化 UTF-8 文本。
    if(tm)strftime(out,n,"%Y-%m-%d %H:%M",tm);else snprintf(out,n,"—");
//【讲解 L65】结束当前代码块。
}
//【讲解 L66】函数 ui_duration：有签到和有效签退时显示分钟差，否则显示进行中或占位符。
static void ui_duration(const cJSON *usage,char *out,size_t n) {
    //【讲解 L67】调用 ui_timestamp，解析云端 ISO 时间文本和时区偏移，转换为 Unix 毫秒；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    int64_t in=ui_timestamp(lab_str(usage,"checked_in_at")),end=ui_timestamp(lab_str(usage,"checked_out_at"));
    //【讲解 L68】检查本行条件，满足后执行紧随其后的语句或代码块；按容量格式化 UTF-8 文本。
    if(in&&end>=in)snprintf(out,n,"%lld 分钟",(long long)((end-in)/60000));
    //【讲解 L69】处理另一分支；按容量格式化 UTF-8 文本。
    else snprintf(out,n,"%s",in?"进行中":"—");
//【讲解 L70】结束当前代码块。
}
//【讲解 L71】函数 ui_preview_model：生成离线截图用示例用户、设备和预约，不读取生产数据库。
static void ui_preview_model(LabClient *c,LabModel *m) {
    //【讲解 L72】按容量格式化 UTF-8 文本。
    snprintf(c->session.uid,sizeof(c->session.uid),"preview-user");
    //【讲解 L73】按容量格式化 UTF-8 文本。
    snprintf(c->session.name,sizeof(c->session.name),"示例用户");
    //【讲解 L74】按容量格式化 UTF-8 文本。
    snprintf(c->session.login,sizeof(c->session.login),"B000001");
    //【讲解 L75】按容量格式化 UTF-8 文本。
    snprintf(c->session.role,sizeof(c->session.role),"student");
    //【讲解 L76】创建 JSON 数组。
    m->devices=cJSON_CreateArray();for(int i=1;i<=40;i++){
        //【讲解 L77】创建 JSON 对象；写入数值字段（字段 number）。
        cJSON *d=cJSON_CreateObject();cJSON_AddNumberToObject(d,"number",i);
        //【讲解 L78】写入字符串字段；把节点所有权交给数组（字段 status）。
        cJSON_AddStringToObject(d,"status",i==6?"faulty":"available");cJSON_AddItemToArray(m->devices,d);
    //【讲解 L79】结束当前代码块。
    }
    //【讲解 L80】解析 JSON 文本。
    m->bookings=cJSON_Parse("[{\"id\":\"preview-1\",\"user_id\":\"preview-user\",\"device_number\":12,\"reservation_date\":\"2026-11-04\",\"start_hour\":9,\"status\":\"completed\"},{\"id\":\"preview-2\",\"user_id\":\"preview-user\",\"device_number\":18,\"reservation_date\":\"2026-11-05\",\"start_hour\":14,\"status\":\"confirmed\"}]");
    //【讲解 L81】解析 JSON 文本。
    m->usage=cJSON_Parse("[{\"reservation_id\":\"preview-1\",\"checked_in_at\":\"2026-11-04T01:02:00+00:00\",\"checked_out_at\":\"2026-11-04T01:52:00+00:00\"}]");
    //【讲解 L82】创建 JSON 数组。
    m->profiles=cJSON_CreateArray();m->issues=cJSON_CreateArray();
//【讲解 L83】结束当前代码块。
}
//【讲解 L84】结束当前条件编译区域。
#endif

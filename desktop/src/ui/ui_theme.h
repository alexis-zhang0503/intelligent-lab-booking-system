/*
 * ui_theme.h —— 界面的"翻译官"和"化妆师"
 *
 * 功能：把云端给的原始数据变成界面上好看、好懂的样子。
 *   - 颜色、尺寸常量：和网页版保持同一套配色
 *   - 文字翻译：云端写 "in_use"，界面显示"使用中"
 *   - 统计摘要："我的"页面上的今日预约数、常用设备、常用时段
 *   - 时间换算：云端给 UTC 时间，界面显示北京时间和使用时长
 *   - 预览数据：不登录也能截图检查界面用的假数据
 *
 * 注意：这个头文件里直接写了函数体，并且每个函数都加了 static。
 *   static 让函数只在包含它的那个 .c 文件里可见，所以多个文件同时包含也不会"重复定义"。
 *   它没有自己 #include client.h 和 string.h，依赖包含它的文件（main.c）先包含好。
 */

/* ===================== 模块 1：头文件保护与依赖 ===================== */
#ifndef LAB_UI_THEME_H       // 防止重复包含
#define LAB_UI_THEME_H
#include <time.h>            // time_t、gmtime、strftime：把时间戳格式化成"2026-11-04 09:02"

/* ===================== 模块 2：配色与尺寸 =====================
 * 功能：记录网页版使用的颜色和尺寸，桌面版照着画，两边看起来一致。
 * 这里是网页格式的颜色文字（#rrggbb），main.c 里实际绘图用的是对应的 RGB() 宏。
 */
#define UI_BG "#f7f7f5"        // 页面背景：浅米白
#define UI_SURFACE "#ffffff"   // 卡片底色：纯白
#define UI_INK "#23252b"       // 正文文字：深灰
#define UI_MUTED "#686b73"     // 次要文字：中灰
#define UI_BORDER "#e8e8e3"    // 边框线：浅灰
#define UI_ACCENT "#5c63d6"    // 强调色：蓝紫，用于主按钮、选中状态
#define UI_SOFT "#eff0ff"      // 浅强调色：已选设备的底色
#define UI_SUCCESS "#398563"   // 成功：绿色
#define UI_DANGER "#bd5252"    // 危险：红色，故障设备的文字颜色
#define UI_SIDEBAR 228         // 左侧导航栏宽度（像素），main.c 布局时据此算出右侧内容区的起点
#define UI_RADIUS 16           // 圆角半径（像素）
#define UI_DEVICE_COLUMNS 8    // 实验室平面图每行摆 8 台设备

/* ===================== 模块 3：文字翻译 =====================
 * 功能：把程序内部的编号、英文状态码换成用户看得懂的中文。
 */

// 页面标题：page 是 main.c 里的当前页面编号，返回显示在页面顶部的大标题
static const char *ui_page_title(int page) {
    // 用连续的 ? : 依次判断：0 预约页、1 我的、2 故障记录、3 用户管理、其他（4）智能助手
    return page == 0 ? "实验室预约" : page == 1 ? "我的" : page == 2 ? "故障记录" : page == 3 ? "用户管理" : "智能助手";
}

// 状态翻译：预约列表、故障列表的"状态"一栏用它显示中文
static const char *ui_status(const char *s) {
    if (!strcmp(s,"confirmed") || !strcmp(s,"reserved")) return "已预约";   // strcmp 相等时返回 0，所以前面要加 !
    if (!strcmp(s,"in_use")) return "使用中";       // 已签到、还没签退
    if (!strcmp(s,"completed")) return "已完成";    // 已签退
    if (!strcmp(s,"cancelled")) return "已取消";
    if (!strcmp(s,"no_show")) return "未到场";      // 预约了但没来签到
    if (!strcmp(s,"resolved")) return "已修复";     // 故障已维修
    if (!strcmp(s,"reported") || !strcmp(s,"open")) return "待处理";       // 故障刚报告
    return s;                                       // 不认识的状态原样显示，至少不会显示空白
}

/* ===================== 模块 4："我的"页面数据整理 =====================
 * 功能：为"我的"页面准备两样东西：每条预约对应的签到签退记录，以及个人预约统计。
 */

// 查找签到记录：预约表和使用记录表是两张表，靠 reservation_id 关联。
// 返回的是表里那条记录的地址（借用，不能 free）；找不到返回 NULL
static const cJSON *ui_usage(const LabModel *m,const cJSON *booking) {
    const cJSON *u; cJSON_ArrayForEach(u,m->usage)                              // 逐条遍历使用记录表
        if (!strcmp(lab_str(u,"reservation_id"),lab_str(booking,"id"))) return u; // 预约编号对上了就是它
    return NULL;
}

// 个人统计：生成"今天的预约"和"预约偏好"两张卡片上的文字。
// 参数：(全部数据, 当前用户, 日期, 写"今天"的缓冲区及大小, 写"偏好"的缓冲区及大小)
static void ui_summary(const LabModel *m,const LabSession *s,const char *date,char *today,size_t tn,char *prefs,size_t pn) {
    // count：选定日期的预约数；total：累计预约数；
    // devices[40]、hours[9]：每台设备、每个时段被约的次数（计数器，先全部清零）；bestD、bestH：次数最多的下标
    int count=0,total=0,devices[40]={0},hours[9]={0},bestD=0,bestH=0; const cJSON *b;
    // 步骤 1：遍历所有预约，只统计"我的、没取消的"
    cJSON_ArrayForEach(b,m->bookings) {
        if(strcmp(lab_str(b,"user_id"),s->uid)||!strcmp(lab_str(b,"status"),"cancelled"))continue; // 不是我的或已取消，跳过
        ++total;if(!strcmp(lab_str(b,"reservation_date"),date))++count;   // 累计数加 1；日期正好是选定日期，今日数也加 1
        int d=lab_int(b,"device_number")-1,h=lab_int(b,"start_hour")-8;   // 设备 1~40 换成下标 0~39；8~16 点换成下标 0~8
        if(d>=0&&d<40)++devices[d];   // 先检查下标范围再计数，防止脏数据导致数组越界
        if(h>=0&&h<9)++hours[h];
    }
    // 步骤 2：找出次数最多的设备和时段（经典的"打擂台"求最大值：从第 0 个开始，谁更大谁当擂主）
    for(int i=1;i<40;i++)if(devices[i]>devices[bestD])bestD=i;
    for(int i=1;i<9;i++)if(hours[i]>hours[bestH])bestH=i;
    // 步骤 3：把结果写成卡片文字；snprintf 带缓冲区大小，文字再长也不会写越界
    snprintf(today,tn,"%s · 今天 %d 个预约时段",date,count);
    if(total)snprintf(prefs,pn,"常用设备 #%02d    常用时段 %02d:00    累计预约 %d 小时",bestD+1,bestH+8,total); // 下标换回设备号和钟点
    else snprintf(prefs,pn,"暂未形成预约偏好 · 选好设备和时段后，会显示在这里。");                         // 从没约过时的提示
}

/* ===================== 模块 5：时间换算 =====================
 * 功能："我的"页面显示实际签到、签退时间和使用了多少分钟。
 * 云端给的时间形如 "2026-11-04T01:02:00+00:00"（UTC 时间 + 时区偏移），
 * 先统一换成 Unix 毫秒（从 1970-01-01 00:00 UTC 起经过的毫秒数），再换成北京时间显示。
 */

// 文字时间 → Unix 毫秒。无法识别时返回 0，调用方把 0 当作"没有这个时间"
static int64_t ui_timestamp(const char *value) {
    int y,mo,d,h,mi,se;
    // 步骤 1：长度不足 19 个字符（"2026-11-04T01:02:00"）肯定不是完整时间；空字符串也在这里被挡掉
    if(strlen(value)<19)return 0;
    // 步骤 2：按格式拆出年月日时分秒，sscanf 返回成功读到的个数，必须正好是 6
    if(sscanf(value,"%d-%d-%dT%d:%d:%d",&y,&mo,&d,&h,&mi,&se)!=6)return 0;
    // 步骤 3：借用业务层现成的日期换算。把日期包装成一条"8 点开始的预约"交给 lab_booking_start：
    //         北京时间 8:00 正好是 UTC 0:00，于是得到的就是这一天 UTC 零点的毫秒数
    char date[32];snprintf(date,sizeof(date),"%04d-%02d-%02d",y,mo,d);
    cJSON *b=cJSON_CreateObject();cJSON_AddStringToObject(b,"reservation_date",date);cJSON_AddNumberToObject(b,"start_hour",8);
    // 步骤 4：再加上当天的时分秒（换成毫秒）；临时对象用完立即删除，避免内存泄漏
    int64_t t=lab_booking_start(b)+(int64_t)(h*3600+mi*60+se)*1000;cJSON_Delete(b);
    // 步骤 5：跳过秒后面可能出现的小数部分（如 ".123456"），指针停在时区符号 + 或 - 上
    const char *z=value+19;while(*z=='.'||(*z>='0'&&*z<='9'))++z;
    // 步骤 6：读出时区偏移"+08:00"中的小时和分钟。这个时间是"当地时间"，减去偏移才是 UTC：
    //         +08:00 要减 8 小时，-05:00 要加 5 小时
    int zh=0,zm=0;if((*z=='+'||*z=='-')&&sscanf(z+1,"%d:%d",&zh,&zm)>=1)t-=(int64_t)(*z=='+'?1:-1)*(zh*60+zm)*60000;
    return t;
}

// Unix 毫秒 → 北京时间文字，例如 "2026-11-04 09:02"
static void ui_time(const char *value,char *out,size_t n) {
    int64_t t=ui_timestamp(value);if(!t){snprintf(out,n,"—");return;}   // 没有时间（例如还没签退）就显示破折号
    // 毫秒换成秒，再加 8 小时变成北京时间；用 gmtime（按 UTC 解释）而不是 localtime，
    // 这样不管电脑设置在哪个时区，显示的都是北京时间
    time_t seconds=(time_t)(t/1000+8*3600);struct tm *tm=gmtime(&seconds);
    if(tm)strftime(out,n,"%Y-%m-%d %H:%M",tm);else snprintf(out,n,"—"); // 按格式输出年-月-日 时:分
}

// 使用时长："实际使用"一栏显示的内容
static void ui_duration(const cJSON *usage,char *out,size_t n) {
    // 分别读出签到和签退时间（usage 为 NULL 时 lab_str 返回 ""，ui_timestamp 返回 0，不会崩溃）
    int64_t in=ui_timestamp(lab_str(usage,"checked_in_at")),end=ui_timestamp(lab_str(usage,"checked_out_at"));
    if(in&&end>=in)snprintf(out,n,"%lld 分钟",(long long)((end-in)/60000)); // 两个都有：相减后毫秒换分钟
    else snprintf(out,n,"%s",in?"进行中":"—");                              // 只签到没签退：进行中；都没有：破折号
}

/* ===================== 模块 6：预览用的假数据 =====================
 * 功能：用 --preview 参数启动程序时不连云端，用这些假数据填满界面，方便截图检查布局。
 *       全是写死的示例，不会读取真实数据库。
 */
static void ui_preview_model(LabClient *c,LabModel *m) {
    // 步骤 1：伪造一个学生身份
    snprintf(c->session.uid,sizeof(c->session.uid),"preview-user");
    snprintf(c->session.name,sizeof(c->session.name),"示例用户");
    snprintf(c->session.login,sizeof(c->session.login),"B000001");
    snprintf(c->session.role,sizeof(c->session.role),"student");
    // 步骤 2：造 40 台设备，其中 6 号设为故障，用来检查故障设备的显示效果
    m->devices=cJSON_CreateArray();for(int i=1;i<=40;i++){
        cJSON *d=cJSON_CreateObject();cJSON_AddNumberToObject(d,"number",i);
        cJSON_AddStringToObject(d,"status",i==6?"faulty":"available");cJSON_AddItemToArray(m->devices,d); // 加进数组后由数组负责释放
    }
    // 步骤 3：造两条预约（一条已完成、一条已预约）。直接解析一段 JSON 文字，\" 是字符串里的双引号
    m->bookings=cJSON_Parse("[{\"id\":\"preview-1\",\"user_id\":\"preview-user\",\"device_number\":12,\"reservation_date\":\"2026-11-04\",\"start_hour\":9,\"status\":\"completed\"},{\"id\":\"preview-2\",\"user_id\":\"preview-user\",\"device_number\":18,\"reservation_date\":\"2026-11-05\",\"start_hour\":14,\"status\":\"confirmed\"}]");
    // 步骤 4：给已完成的那条配一条签到签退记录（UTC 01:02 到 01:52，即北京时间 09:02 到 09:52，共 50 分钟）
    m->usage=cJSON_Parse("[{\"reservation_id\":\"preview-1\",\"checked_in_at\":\"2026-11-04T01:02:00+00:00\",\"checked_out_at\":\"2026-11-04T01:52:00+00:00\"}]");
    // 步骤 5：用户表和故障表给空数组，保证界面代码遍历时不会遇到 NULL
    m->profiles=cJSON_CreateArray();m->issues=cJSON_CreateArray();
}
#endif // 头文件保护结束

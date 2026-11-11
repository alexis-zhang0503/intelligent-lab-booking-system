//【答辩注释】平台界面入口：收集用户输入，交给后台任务调用共享业务层，再在界面线程展示结果。Windows 使用 Win32 消息；macOS 使用 GTK 信号及主循环。界面权限提示不能替代云端鉴权。
//【讲解 L1】按 UNICODE 是否已定义选择编译分支，避免重复定义或适配平台。
#ifndef UNICODE
//【讲解 L2】定义 UNICODE，用于编译配置或头文件保护。
#define UNICODE
//【讲解 L3】结束当前条件编译区域。
#endif
//【讲解 L4】按 _UNICODE 是否已定义选择编译分支，避免重复定义或适配平台。
#ifndef _UNICODE
//【讲解 L5】定义 _UNICODE，用于编译配置或头文件保护。
#define _UNICODE
//【讲解 L6】结束当前条件编译区域。
#endif
//【讲解 L7】按 _WIN32_WINNT 是否已定义选择编译分支，避免重复定义或适配平台。
#ifndef _WIN32_WINNT
//【讲解 L8】定义 _WIN32_WINNT，当前值为 0x0A00。
#define _WIN32_WINNT 0x0A00
//【讲解 L9】结束当前条件编译区域。
#endif
//【讲解 L10】引入 windows.h 的类型和函数声明。
#include <windows.h>
//【讲解 L11】引入 client.h 的类型和函数声明。
#include "client.h"
//【讲解 L12】引入 win_http.h 的类型和函数声明。
#include "win_http.h"
//【讲解 L13】引入 commctrl.h 的类型和函数声明。
#include <commctrl.h>
//【讲解 L14】引入 commdlg.h 的类型和函数声明。
#include <commdlg.h>
//【讲解 L15】引入 objbase.h 的类型和函数声明。
#include <objbase.h>
//【讲解 L16】引入 shellapi.h 的类型和函数声明。
#include <shellapi.h>
//【讲解 L17】引入 stdio.h 的类型和函数声明。
#include <stdio.h>
//【讲解 L18】引入 stdlib.h 的类型和函数声明。
#include <stdlib.h>
//【讲解 L19】引入 string.h 的类型和函数声明。
#include <string.h>
//【讲解 L20】引入 windowsx.h 的类型和函数声明。
#include <windowsx.h>
//【讲解 L21】引入 ui_theme.h 的类型和函数声明。
#include "ui_theme.h"
//【讲解 L22】定义 RESULT_MESSAGE，当前值为 (WM_APP + 1)。
#define RESULT_MESSAGE (WM_APP + 1)
//【讲解 L23】定义 BG，当前值为 RGB(247,247,245)。
#define BG RGB(247,247,245)
//【讲解 L24】定义 ACCENT，当前值为 RGB(92,99,214)。
#define ACCENT RGB(92,99,214)
//【讲解 L25】定义 BORDER，当前值为 RGB(232,232,227)。
#define BORDER RGB(232,232,227)
//【讲解 L26】定义 MUTED，当前值为 RGB(104,107,115)。
#define MUTED RGB(104,107,115)
//【讲解 L27】定义 INK，当前值为 RGB(35,37,43)。
#define INK RGB(35,37,43)
//【讲解 L28】定义操作编号，供按钮事件或后台任务分派使用。
enum {
    //【讲解 L29】声明 ID_LOGIN 操作编号，与事件分派中的同名分支对应。
    ID_LOGIN = 100,
    //【讲解 L30】声明 ID_REGISTER 操作编号，与事件分派中的同名分支对应。
    ID_REGISTER,
    //【讲解 L31】声明 ID_REFRESH 操作编号，与事件分派中的同名分支对应。
    ID_REFRESH,
    //【讲解 L32】声明 ID_LOGOUT 操作编号，与事件分派中的同名分支对应。
    ID_LOGOUT,
    //【讲解 L33】声明 ID_BOOK 操作编号，与事件分派中的同名分支对应。
    ID_BOOK,
    //【讲解 L34】声明 ID_ATTEND 操作编号，与事件分派中的同名分支对应。
    ID_ATTEND,
    //【讲解 L35】声明 ID_CANCEL 操作编号，与事件分派中的同名分支对应。
    ID_CANCEL,
    //【讲解 L36】声明 ID_REPORT 操作编号，与事件分派中的同名分支对应。
    ID_REPORT,
    //【讲解 L37】声明 ID_REPAIR 操作编号，与事件分派中的同名分支对应。
    ID_REPAIR,
    //【讲解 L38】声明 ID_ARCHIVE 操作编号，与事件分派中的同名分支对应。
    ID_ARCHIVE,
    //【讲解 L39】声明 ID_DEACTIVATE 操作编号，与事件分派中的同名分支对应。
    ID_DEACTIVATE,
    //【讲解 L40】声明 ID_INVITE 操作编号，与事件分派中的同名分支对应。
    ID_INVITE,
    //【讲解 L41】声明 ID_METRICS 操作编号，与事件分派中的同名分支对应。
    ID_METRICS,
    //【讲解 L42】声明 ID_UPDATE 操作编号，与事件分派中的同名分支对应。
    ID_UPDATE,
    //【讲解 L43】声明 ID_PROFILE 操作编号，与事件分派中的同名分支对应。
    ID_PROFILE,
    //【讲解 L44】声明 ID_SUGGEST 操作编号，与事件分派中的同名分支对应。
    ID_SUGGEST,
    //【讲解 L45】声明 ID_AGENT 操作编号，与事件分派中的同名分支对应。
    ID_AGENT,
    //【讲解 L46】声明 ID_AGENT_CONFIRM 操作编号，与事件分派中的同名分支对应。
    ID_AGENT_CONFIRM,
    //【讲解 L47】声明 ID_EXPORT_BOOKINGS 操作编号，与事件分派中的同名分支对应。
    ID_EXPORT_BOOKINGS,
    //【讲解 L48】声明 ID_EXPORT_ISSUES 操作编号，与事件分派中的同名分支对应。
    ID_EXPORT_ISSUES,
    //【讲解 L49】声明 ID_USE_CANDIDATE 操作编号，与事件分派中的同名分支对应。
    ID_USE_CANDIDATE,
    //【讲解 L50】声明 ID_TAB 操作编号，与事件分派中的同名分支对应。
    ID_TAB = 200,
    //【讲解 L51】声明 ID_LIST 操作编号，与事件分派中的同名分支对应。
    ID_LIST,
    //【讲解 L52】声明 ID_DATE 操作编号，与事件分派中的同名分支对应。
    ID_DATE,
    //【讲解 L53】声明 ID_START 操作编号，与事件分派中的同名分支对应。
    ID_START,
    //【讲解 L54】声明 ID_END 操作编号，与事件分派中的同名分支对应。
    ID_END,
    //【讲解 L55】声明 ID_FIRST_DEVICE 操作编号，与事件分派中的同名分支对应。
    ID_FIRST_DEVICE = 300
//【讲解 L56】结束当前代码块或类型声明。
};
//【讲解 L57】定义操作编号，供按钮事件或后台任务分派使用。
enum {
    //【讲解 L58】声明 JOB_LOGIN 操作编号，与事件分派中的同名分支对应。
    JOB_LOGIN,
    //【讲解 L59】声明 JOB_REGISTER 操作编号，与事件分派中的同名分支对应。
    JOB_REGISTER,
    //【讲解 L60】声明 JOB_REFRESH 操作编号，与事件分派中的同名分支对应。
    JOB_REFRESH,
    //【讲解 L61】声明 JOB_BOOK 操作编号，与事件分派中的同名分支对应。
    JOB_BOOK,
    //【讲解 L62】声明 JOB_ATTEND 操作编号，与事件分派中的同名分支对应。
    JOB_ATTEND,
    //【讲解 L63】声明 JOB_CANCEL 操作编号，与事件分派中的同名分支对应。
    JOB_CANCEL,
    //【讲解 L64】声明 JOB_REPORT 操作编号，与事件分派中的同名分支对应。
    JOB_REPORT,
    //【讲解 L65】声明 JOB_REPAIR 操作编号，与事件分派中的同名分支对应。
    JOB_REPAIR,
    //【讲解 L66】声明 JOB_ARCHIVE 操作编号，与事件分派中的同名分支对应。
    JOB_ARCHIVE,
    //【讲解 L67】声明 JOB_DEACTIVATE 操作编号，与事件分派中的同名分支对应。
    JOB_DEACTIVATE,
    //【讲解 L68】声明 JOB_INVITE 操作编号，与事件分派中的同名分支对应。
    JOB_INVITE,
    //【讲解 L69】声明 JOB_METRICS 操作编号，与事件分派中的同名分支对应。
    JOB_METRICS,
    //【讲解 L70】声明 JOB_UPDATE 操作编号，与事件分派中的同名分支对应。
    JOB_UPDATE,
    //【讲解 L71】声明 JOB_PROFILE 操作编号，与事件分派中的同名分支对应。
    JOB_PROFILE,
    //【讲解 L72】声明 JOB_SUGGEST 操作编号，与事件分派中的同名分支对应。
    JOB_SUGGEST,
    //【讲解 L73】声明 JOB_AGENT 操作编号，与事件分派中的同名分支对应。
    JOB_AGENT
//【讲解 L74】结束当前代码块或类型声明。
};
//【讲解 L75】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef struct {
    //【讲解 L76】声明或更新 devices（设备列表）；按本行表达式保存对应状态。
    int kind, action, start, end, devices[40], count, metricDevice, inviteDays;
    //【讲解 L77】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    LabClient client;
    //【讲解 L78】声明或更新 model（当前数据模型）；按本行表达式保存对应状态。
    LabModel model;
    //【讲解 L79】声明或更新 row（当前模型记录）、agentRequest（助手请求对象）、agentResult（助手响应对象）；按本行表达式保存对应状态。
    cJSON *row, *agentRequest, *agentResult;
    //【讲解 L80】声明或更新 password（密码输入或任务副本）、identity（学号或邮箱输入）；按本行表达式保存对应状态。
    char identity[256], password[256], name[256], invite[256], date[16], endDate[16], note[2048];
    //【讲解 L81】声明或更新 keys（每小时幂等键）；按本行表达式保存对应状态。
    char keys[9][64], message[16384];
    //【讲解 L82】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int ok, loaded;
    //【讲解 L83】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    HWND target;
//【讲解 L84】结束当前代码块或类型声明。
} Job;
//【讲解 L85】声明或更新 window（主窗口）、password（密码输入或任务副本）、identity（学号或邮箱输入）；按本行表达式保存对应状态。
static HWND window, identity, password, nameEdit, inviteEdit, loginButton, registerButton, loginInfo;
//【讲解 L86】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
static HWND tabs, list, dateEdit, startEdit, endEdit, noteEdit, statusText, helpText, whoText,
    //【讲解 L87】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    deviceButtons[40];
//【讲解 L88】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
static HWND actions[19], labels[4];
//【讲解 L89】声明或更新 font（常规字体）；按本行表达式保存对应状态。
static HFONT font, bold, headingFont, heroFont;
//【讲解 L90】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
static HWND agentOptions,scheduleOptions;
//【讲解 L91】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
static HWND navigation[5],profileFault,usageList,agentReply;
//【讲解 L92】声明或更新 preview（离线预览标记）；按本行表达式保存对应状态。
static int scrollY,preview,rendering,selectedTable;
//【讲解 L93】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
static char selectedId[80];
//【讲解 L94】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
static wchar_t previewOutput[MAX_PATH];
//【讲解 L95】函数 layout：按窗口、页面和滚动状态排列控件；这是原生布局，与浏览器 CSS 实现不同。
static void layout(void);
//【讲解 L96】声明或更新 background（背景画刷）；按本行表达式保存对应状态。
static HBRUSH background;
//【讲解 L97】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
static LabClient client;
//【讲解 L98】声明或更新 model（当前数据模型）；按本行表达式保存对应状态。
static LabModel model;
//【讲解 L99】声明或更新 agent（当前账号助手状态）；按本行表达式保存对应状态。
static LabAgentState agent;
//【讲解 L100】声明或更新 scheduleCandidates（规则推荐候选方案）；按本行表达式保存对应状态。
static cJSON *scheduleCandidates;
//【讲解 L101】声明或更新 busy（请求忙标记）、logged（登录标记）、page（当前页面编号）、selected（设备选择状态）；按本行表达式保存对应状态。
static int busy, checkingUpdate, logged, page, selected[40];
//【讲解 L102】声明或更新 listed（界面行与模型记录映射）；按本行表达式保存对应状态。
static cJSON *listed[100000];
//【讲解 L103】声明或更新 listedCount（已显示记录数）；按本行表达式保存对应状态。
static int listedCount;
//【讲解 L104】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
static const int actionIds[] = {ID_REFRESH, ID_LOGOUT, ID_BOOK,    ID_ATTEND,     ID_CANCEL,
                                //【讲解 L105】声明 ID_REPORT 操作编号，与事件分派中的同名分支对应。
                                ID_REPORT,  ID_REPAIR, ID_ARCHIVE, ID_DEACTIVATE, ID_INVITE,
                                //【讲解 L106】声明 ID_METRICS 操作编号，与事件分派中的同名分支对应。
                                ID_METRICS, ID_UPDATE, ID_PROFILE, ID_SUGGEST,    ID_AGENT, ID_AGENT_CONFIRM, ID_EXPORT_BOOKINGS, ID_EXPORT_ISSUES, ID_USE_CANDIDATE};
//【讲解 L107】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
static const wchar_t *actionNames[] = {L"刷新",         L"退出账号",     L"确认预约 →", L"签到 / 签退",
                                       //【讲解 L108】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                                       L"取消预约",     L"报告设备故障",     L"完成维修",     L"归档故障",
                                       //【讲解 L109】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                                       L"停用学生",     L"生成邀请码",   L"统计报表",     L"检查版本",
                                       //【讲解 L110】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                                       L"修改本人资料", L"智能推荐", L"发送", L"选择此方案", L"导出完整预约", L"导出完整故障", L"采用推荐方案"};
//【讲解 L111】函数 wide：把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
static wchar_t *wide(const char *s) {
    //【讲解 L112】把 UTF-8 转为 Windows 宽字符。
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    //【讲解 L113】申请并清零堆内存。
    wchar_t *p = calloc(n ? n : 1, sizeof(wchar_t));
    //【讲解 L114】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (p && n)
        //【讲解 L115】把 UTF-8 转为 Windows 宽字符。
        MultiByteToWideChar(CP_UTF8, 0, s, -1, p, n);
    //【讲解 L116】结束当前函数并返回 p；具体成功或失败含义见函数说明。
    return p;
//【讲解 L117】结束当前代码块。
}
//【讲解 L118】函数 set_utf8：转换后设置 Windows 控件文字，并释放临时宽字符串。
static void set_utf8(HWND h, const char *s) {
    //【讲解 L119】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
    wchar_t *p = wide(s);
    //【讲解 L120】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (p) {
        //【讲解 L121】更新控件文字。
        SetWindowTextW(h, p);
        //【讲解 L122】释放堆内存。
        free(p);
    //【讲解 L123】结束当前代码块。
    }
//【讲解 L124】结束当前代码块。
}
//【讲解 L125】函数 get_utf8：读取 Windows 控件宽字符文本，转换为业务层 UTF-8，写入有限容量缓冲区。
static void get_utf8(HWND h, char *s, int n) {
    //【讲解 L126】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    wchar_t b[2048];
    //【讲解 L127】执行 GetWindowTextW，参数沿用本行给出的对象、标志和容量。
    GetWindowTextW(h, b, 2048);
    //【讲解 L128】检查本行条件，满足后执行紧随其后的语句或代码块；把宽字符转回 UTF-8。
    if (!WideCharToMultiByte(CP_UTF8, 0, b, -1, s, n, NULL, NULL))
        //【讲解 L129】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        s[0] = 0;
//【讲解 L130】结束当前代码块。
}
//【讲解 L131】函数 tell：显示平台消息提示；网络结果由界面线程调用此函数展示。
static void tell(const char *s, UINT flags) {
    //【讲解 L132】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
    wchar_t *p = wide(s);
    //【讲解 L133】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (p) {
        //【讲解 L134】声明或更新 window（主窗口）；按本行表达式保存对应状态。
        MessageBoxW(window, p, L"实验室预约 · 原生 C", flags);
        //【讲解 L135】释放堆内存。
        free(p);
    //【讲解 L136】结束当前代码块。
    }
//【讲解 L137】结束当前代码块。
}
//【讲解 L138】函数 admin：根据云端加载的 session.role 判断是否显示管理入口；不能替代服务端权限规则。
static int admin(void) { return !strcmp(client.session.role, "admin"); }
//【讲解 L139】函数 now_ms：取得当前 Unix 毫秒；比较预约区间时统一使用绝对时间。
static int64_t now_ms(void) {
    //【讲解 L140】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    FILETIME f;
    //【讲解 L141】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    ULARGE_INTEGER u;
    //【讲解 L142】执行 GetSystemTimeAsFileTime，参数沿用本行给出的对象、标志和容量。
    GetSystemTimeAsFileTime(&f);
    //【讲解 L143】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    u.LowPart = f.dwLowDateTime;
    //【讲解 L144】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    u.HighPart = f.dwHighDateTime;
    //【讲解 L145】结束当前函数并返回 (int64_t)(u.QuadPart / 10000 - 11644473600000ULL)；具体成功或失败含义见函数说明。
    return (int64_t)(u.QuadPart / 10000 - 11644473600000ULL);
//【讲解 L146】结束当前代码块。
}
//【讲解 L147】函数 date_string：将日期控件的年月日格式化为 YYYY-MM-DD。
static void date_string(char out[16]) {
    //【讲解 L148】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    SYSTEMTIME t;
    //【讲解 L149】执行 DateTime_GetSystemtime，参数沿用本行给出的对象、标志和容量。
    DateTime_GetSystemtime(dateEdit, &t);
    //【讲解 L150】按容量格式化 UTF-8 文本。
    snprintf(out, 16, "%04u-%02u-%02u", t.wYear, t.wMonth, t.wDay);
//【讲解 L151】结束当前代码块。
}
//【讲解 L152】函数 today_beijing：以 UTC 加八小时确定北京当天，再设置日期控件。
static void today_beijing(void) {
    //【讲解 L153】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    FILETIME f;
    //【讲解 L154】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    ULARGE_INTEGER u;
    //【讲解 L155】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    SYSTEMTIME t;
    //【讲解 L156】执行 GetSystemTimeAsFileTime，参数沿用本行给出的对象、标志和容量。
    GetSystemTimeAsFileTime(&f);
    //【讲解 L157】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    u.LowPart = f.dwLowDateTime;
    //【讲解 L158】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    u.HighPart = f.dwHighDateTime;
    //【讲解 L159】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    u.QuadPart += 8ULL * 3600 * 10000000;
    //【讲解 L160】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    f.dwLowDateTime = u.LowPart;
    //【讲解 L161】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    f.dwHighDateTime = u.HighPart;
    //【讲解 L162】执行 FileTimeToSystemTime，参数沿用本行给出的对象、标志和容量。
    FileTimeToSystemTime(&f, &t);
    //【讲解 L163】执行 DateTime_SetSystemtime，参数沿用本行给出的对象、标志和容量。
    DateTime_SetSystemtime(dateEdit, GDT_VALID, &t);
//【讲解 L164】结束当前代码块。
}
//【讲解 L165】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
static HWND control(const wchar_t *cls, const wchar_t *text, DWORD style, int id, int x, int y, int w,
                    //【讲解 L166】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
                    int h) {
    //【讲解 L167】声明或更新 window（主窗口）；按本行表达式保存对应状态。
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, window,
                             //【讲解 L168】执行 GetModuleHandleW，参数沿用本行给出的对象、标志和容量。
                             (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    //【讲解 L169】同步读写控件状态。
    SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
    //【讲解 L170】结束当前函数并返回 c；具体成功或失败含义见函数说明。
    return c;
//【讲解 L171】结束当前代码块。
}
//【讲解 L172】函数 add_cell：向列表指定行列填入文字，处理 UTF-8 与宽字符转换。
static void add_cell(int row, int col, const char *s) {
    //【讲解 L173】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
    wchar_t *w = wide(s);
    //【讲解 L174】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    LVITEMW item = {0};
    //【讲解 L175】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    item.mask = LVIF_TEXT;
    //【讲解 L176】声明或更新 row（当前模型记录）；按本行表达式保存对应状态。
    item.iItem = row;
    //【讲解 L177】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    item.iSubItem = col;
    //【讲解 L178】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    item.pszText = w;
    //【讲解 L179】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (col == 0)
        //【讲解 L180】同步读写控件状态。
        SendMessageW(list, LVM_INSERTITEMW, 0, (LPARAM)&item);
    //【讲解 L181】处理另一分支。
    else
        //【讲解 L182】同步读写控件状态。
        SendMessageW(list, LVM_SETITEMTEXTW, (WPARAM)row, (LPARAM)&item);
    //【讲解 L183】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (page == 1 && usageList) {
        //【讲解 L184】检查本行条件，满足后执行紧随其后的语句或代码块；同步读写控件状态。
        if(col==0)SendMessageW(usageList,LVM_INSERTITEMW,0,(LPARAM)&item);
        //【讲解 L185】处理另一分支；同步读写控件状态。
        else SendMessageW(usageList,LVM_SETITEMTEXTW,(WPARAM)row,(LPARAM)&item);
    //【讲解 L186】结束当前代码块。
    }
    //【讲解 L187】释放堆内存。
    free(w);
//【讲解 L188】结束当前代码块。
}
//【讲解 L189】函数 column：设置 Windows 列表的列名及宽度。
static void column(int i, const wchar_t *title, int width) {
    //【讲解 L190】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    LVCOLUMNW c = {0};
    //【讲解 L191】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    //【讲解 L192】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    c.pszText = (LPWSTR)title;
    //【讲解 L193】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    c.cx = width;
    //【讲解 L194】同步读写控件状态。
    SendMessageW(list, LVM_INSERTCOLUMNW, (WPARAM)i, (LPARAM)&c);
    //【讲解 L195】检查本行条件，满足后执行紧随其后的语句或代码块；同步读写控件状态。
    if(page==1&&usageList)SendMessageW(usageList,LVM_INSERTCOLUMNW,(WPARAM)i,(LPARAM)&c);
//【讲解 L196】结束当前代码块。
}
//【讲解 L197】函数 render：按页面、账号、日期和设备状态更新控件内容；列表行保存模型记录引用，不负责云端写入。
static void render(void) {
    //【讲解 L198】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    const cJSON *r;
    //【讲解 L199】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char b[1024], d[16];
    //【讲解 L200】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int i = 0;
    //【讲解 L201】调用 date_string，将日期控件的年月日格式化为 YYYY-MM-DD。
    date_string(d);
    //【讲解 L202】检查本行条件，满足后执行紧随其后的语句或代码块；调用 lab_agent_clear，释放助手历史及最近响应并清空状态；调用 lab_agent_bind，把助手状态绑定到账号。
    if(!logged)lab_agent_clear(&agent);else lab_agent_bind(&agent,client.session.uid);
    //【讲解 L203】调用 lab_agent_transcript，把内存中的对话拼成有容量限制的中文显示文本；调用 set_utf8，转换后设置 Windows 控件文字，并释放临时宽字符串。
    char transcript[16384];lab_agent_transcript(&agent,transcript,sizeof(transcript));set_utf8(agentReply,transcript);
    //【讲解 L204】同步读写控件状态。
    SendMessageW(agentOptions,CB_RESETCONTENT,0,0);const cJSON *proposal;int option=0;
    //【讲解 L205】遍历 JSON 数组；调用 lab_agent_proposals，借用最近响应中的候选方案数组；调用 lab_agent_option，把一个候选方案的日期、小时、设备拼成下拉框摘要，并在写入时检查剩余容量。
    cJSON_ArrayForEach(proposal,lab_agent_proposals(&agent)){char label[1024];lab_agent_option(proposal,++option,label,sizeof(label));wchar_t *value=wide(label);SendMessageW(agentOptions,CB_ADDSTRING,0,(LPARAM)value);free(value);}
    //【讲解 L206】同步读写控件状态。
    SendMessageW(agentOptions,CB_SETCURSEL,option?0:(WPARAM)-1,0);
    //【讲解 L207】同步读写控件状态。
    SendMessageW(scheduleOptions,CB_RESETCONTENT,0,0);
    //【讲解 L208】遍历 JSON 数组；创建 JSON 对象；创建 JSON 数组（字段 items）。
    const cJSON *candidate;int candidateCount=0;cJSON_ArrayForEach(candidate,scheduleCandidates){char label[1024];cJSON *wrapper=cJSON_CreateObject(),*body=cJSON_CreateObject(),*items=cJSON_CreateArray();cJSON_AddItemToArray(items,cJSON_Duplicate(candidate,1));cJSON_AddItemToObject(body,"items",items);cJSON_AddItemToObject(wrapper,"candidate",body);lab_agent_option(wrapper,++candidateCount,label,sizeof(label));cJSON_Delete(wrapper);
    //【讲解 L209】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free；同步读写控件状态；释放堆内存。
    wchar_t *value=wide(label);SendMessageW(scheduleOptions,CB_ADDSTRING,0,(LPARAM)value);free(value);}
    //【讲解 L210】同步读写控件状态。
    SendMessageW(scheduleOptions,CB_SETCURSEL,candidateCount?0:(WPARAM)-1,0);
    //【讲解 L211】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    rendering=1;
    //【讲解 L212】执行 ListView_DeleteAllItems，参数沿用本行给出的对象、标志和容量。
    ListView_DeleteAllItems(usageList);
    //【讲解 L213】条件成立时继续循环。
    while(ListView_DeleteColumn(usageList,0)){}
    //【讲解 L214】同步读写控件状态。
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    //【讲解 L215】执行 ListView_DeleteAllItems，参数沿用本行给出的对象、标志和容量。
    ListView_DeleteAllItems(list);
    //【讲解 L216】条件成立时继续循环。
    while (ListView_DeleteColumn(list, 0)) {
    //【讲解 L217】结束当前代码块。
    }
    //【讲解 L218】声明或更新 listedCount（已显示记录数）；按本行表达式保存对应状态。
    listedCount = 0;
    //【讲解 L219】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (page < 2) {
        //【讲解 L220】调用 column，设置 Windows 列表的列名及宽度。
        column(0, L"设备", 75);
        //【讲解 L221】调用 column，设置 Windows 列表的列名及宽度。
        column(1, L"日期（北京）", 120);
        //【讲解 L222】调用 column，设置 Windows 列表的列名及宽度。
        column(2, L"时段", 100);
        //【讲解 L223】调用 column，设置 Windows 列表的列名及宽度。
        column(3, L"状态", 95);
        //【讲解 L224】调用 column，设置 Windows 列表的列名及宽度。
        column(4, page==1?L"实际签到":L"预约人", 160);
        //【讲解 L225】检查本行条件，满足后执行紧随其后的语句或代码块；调用 column，设置 Windows 列表的列名及宽度。
        if(page==1){column(5,L"实际签退",160);column(6,L"实际使用",100);}
        //【讲解 L226】遍历 JSON 数组。
        cJSON_ArrayForEach(r, model.bookings) {
            //【讲解 L227】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            if (page == 1 && strcmp(lab_str(r, "user_id"), client.session.uid))
                //【讲解 L228】跳过当前记录，继续下一次循环。
                continue;
            //【讲解 L229】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针；调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
            if (page == 0 && (strcmp(lab_str(r, "reservation_date"), d)||(!admin()&&strcmp(lab_str(r,"user_id"),client.session.uid))))
                //【讲解 L230】跳过当前记录，继续下一次循环。
                continue;
            //【讲解 L231】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (i >= 100000)
                //【讲解 L232】结束当前最近的循环或 switch 分支。
                break;
            //【讲解 L233】声明或更新 listed（界面行与模型记录映射）；按本行表达式保存对应状态。
            listed[i] = (cJSON *)r;
            //【讲解 L234】按容量格式化 UTF-8 文本；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
            snprintf(b, sizeof(b), "%02d", lab_int(r, "device_number"));
            //【讲解 L235】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换。
            add_cell(i, 0, b);
            //【讲解 L236】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 1, lab_str(r, "reservation_date"));
            //【讲解 L237】按容量格式化 UTF-8 文本；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
            snprintf(b, sizeof(b), "%02d:00–%02d:00", lab_int(r, "start_hour"), lab_int(r, "start_hour") + 1);
            //【讲解 L238】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换。
            add_cell(i, 2, b);
            //【讲解 L239】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 ui_status，将云端状态码转换为中文展示，未识别值保留原文；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 3, ui_status(lab_str(r, "status")));
            //【讲解 L240】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            const char *owner = lab_str(r, "user_id");
            //【讲解 L241】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
            const cJSON *p;
            //【讲解 L242】遍历 JSON 数组；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            cJSON_ArrayForEach(p, model.profiles) if (!strcmp(lab_str(p, "id"), owner)) {
                //【讲解 L243】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
                owner = lab_str(p, "display_name");
                //【讲解 L244】结束当前最近的循环或 switch 分支。
                break;
            //【讲解 L245】结束当前代码块。
            }
            //【讲解 L246】检查本行条件，满足后执行紧随其后的语句或代码块。
            if(page==1){
                //【讲解 L247】调用 ui_usage，按 reservation_id 查找对应的签到签退记录，返回模型内部借用指针。
                const cJSON *u=ui_usage(&model,r);char value[96];
                //【讲解 L248】调用 ui_time，将云端时间转换为北京时间文本；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针；调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换。
                ui_time(lab_str(u,"checked_in_at"),value,sizeof(value));add_cell(i,4,value);
                //【讲解 L249】调用 ui_time，将云端时间转换为北京时间文本；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针；调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换。
                ui_time(lab_str(u,"checked_out_at"),value,sizeof(value));add_cell(i,5,value);
                //【讲解 L250】调用 ui_duration，有签到和有效签退时显示分钟差，否则显示进行中或占位符；调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换。
                ui_duration(u,value,sizeof(value));add_cell(i,6,value);
            //【讲解 L251】处理另一分支；调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换。
            }else add_cell(i, 4, owner);
            //【讲解 L252】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            i++;
        //【讲解 L253】结束当前代码块。
        }
    //【讲解 L254】上一条件不成立时检查下一条件。
    } else if (page == 2) {
        //【讲解 L255】调用 column，设置 Windows 列表的列名及宽度。
        column(0, L"设备", 65);
        //【讲解 L256】调用 column，设置 Windows 列表的列名及宽度。
        column(1, L"状态", 85);
        //【讲解 L257】调用 column，设置 Windows 列表的列名及宽度。
        column(2, L"故障说明", 290);
        //【讲解 L258】调用 column，设置 Windows 列表的列名及宽度。
        column(3, L"处理记录", 230);
        //【讲解 L259】调用 column，设置 Windows 列表的列名及宽度。
        column(4, L"报告时间", 190);
        //【讲解 L260】遍历 JSON 数组。
        cJSON_ArrayForEach(r, model.issues) {
            //【讲解 L261】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (i >= 100000)
                //【讲解 L262】结束当前最近的循环或 switch 分支。
                break;
            //【讲解 L263】声明或更新 listed（界面行与模型记录映射）；按本行表达式保存对应状态。
            listed[i] = (cJSON *)r;
            //【讲解 L264】按容量格式化 UTF-8 文本；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
            snprintf(b, sizeof(b), "%02d", lab_int(r, "device_number"));
            //【讲解 L265】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换。
            add_cell(i, 0, b);
            //【讲解 L266】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 ui_status，将云端状态码转换为中文展示，未识别值保留原文；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 1, ui_status(lab_str(r, "status")));
            //【讲解 L267】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 2, lab_str(r, "description"));
            //【讲解 L268】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 3, lab_str(r, "resolution_note"));
            //【讲解 L269】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 4, lab_str(r, "created_at"));
            //【讲解 L270】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            i++;
        //【讲解 L271】结束当前代码块。
        }
    //【讲解 L272】上一条件不成立时检查下一条件。
    } else if(page==3) {
        //【讲解 L273】调用 column，设置 Windows 列表的列名及宽度。
        column(0, L"姓名", 180);
        //【讲解 L274】调用 column，设置 Windows 列表的列名及宽度。
        column(1, L"学号 / 登录名", 220);
        //【讲解 L275】调用 column，设置 Windows 列表的列名及宽度。
        column(2, L"角色", 90);
        //【讲解 L276】调用 column，设置 Windows 列表的列名及宽度。
        column(3, L"账号状态", 110);
        //【讲解 L277】遍历 JSON 数组。
        cJSON_ArrayForEach(r, model.profiles) {
            //【讲解 L278】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (i >= 100000)
                //【讲解 L279】结束当前最近的循环或 switch 分支。
                break;
            //【讲解 L280】声明或更新 listed（界面行与模型记录映射）；按本行表达式保存对应状态。
            listed[i] = (cJSON *)r;
            //【讲解 L281】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 0, lab_str(r, "display_name"));
            //【讲解 L282】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 1, lab_str(r, "login_id"));
            //【讲解 L283】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            add_cell(i, 2, lab_str(r, "role"));
            //【讲解 L284】调用 add_cell，向列表指定行列填入文字，处理 UTF-8 与宽字符转换；按精确字段名借用 JSON 子节点（字段 active）。
            add_cell(i, 3, cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(r, "active")) ? "已停用" : "可用");
            //【讲解 L285】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            i++;
        //【讲解 L286】结束当前代码块。
        }
    //【讲解 L287】结束当前代码块。
    }
    //【讲解 L288】声明或更新 listedCount（已显示记录数）；按本行表达式保存对应状态。
    listedCount = i;
    //【讲解 L289】按本行初值、条件和步长循环；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    for(int row=0;row<i;row++)if(*selectedId&&!strcmp(lab_str(listed[row],"id"),selectedId)){
        //【讲解 L290】声明或更新 row（当前模型记录）；按本行表达式保存对应状态。
        ListView_SetItemState(list,row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
        //【讲解 L291】检查本行条件，满足后执行紧随其后的语句或代码块。
        if(page==1)ListView_SetItemState(usageList,row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    //【讲解 L292】结束当前代码块。
    }
    //【讲解 L293】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    rendering=0;
    //【讲解 L294】同步读写控件状态。
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    //【讲解 L295】请求重绘界面。
    InvalidateRect(list, NULL, TRUE);
    //【讲解 L296】按容量格式化 UTF-8 文本；调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
    snprintf(b, sizeof(b), "%s  ·  %s  ·  原生 C %s", client.session.name, admin() ? "管理员" : "学生",
             //【讲解 L297】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
             LAB_NATIVE_VERSION);
    //【讲解 L298】调用 set_utf8，转换后设置 Windows 控件文字，并释放临时宽字符串。
    set_utf8(whoText, b);
    //【讲解 L299】按本行初值、条件和步长循环。
    for (i = 0; i < 40; i++)
        //【讲解 L300】请求重绘界面。
        InvalidateRect(deviceButtons[i], NULL, TRUE);
    //【讲解 L301】请求重绘界面。
    InvalidateRect(window,NULL,TRUE);
//【讲解 L302】结束当前代码块。
}
//【讲解 L303】函数 visibility：根据登录态、页面、角色及 busy 状态切换控件可见性和可操作性。
static void visibility(void) {
    //【讲解 L304】声明或更新 password（密码输入或任务副本）、identity（学号或邮箱输入）；按本行表达式保存对应状态。
    HWND auth[] = {identity, password, nameEdit, inviteEdit, loginButton, registerButton, loginInfo};
    //【讲解 L305】按本行初值、条件和步长循环。
    for (size_t i = 0; i < sizeof(auth) / sizeof(auth[0]); i++)
        //【讲解 L306】设置窗口或控件显示状态。
        ShowWindow(auth[i], logged ? SW_HIDE : SW_SHOW);
    //【讲解 L307】按本行初值、条件和步长循环。
    for (int i = 0; i < 4; i++)
        //【讲解 L308】设置窗口或控件显示状态。
        ShowWindow(labels[i], logged ? SW_HIDE : SW_SHOW);
    //【讲解 L309】设置窗口或控件显示状态。
    ShowWindow(tabs,SW_HIDE);
    //【讲解 L310】按本行初值、条件和步长循环；设置窗口或控件显示状态；调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
    for(int i=0;i<5;i++)ShowWindow(navigation[i],logged&&((i!=2&&i!=3)||admin())?SW_SHOW:SW_HIDE);
    //【讲解 L311】设置窗口或控件显示状态。
    ShowWindow(profileFault,logged&&page==1?SW_SHOW:SW_HIDE);
    //【讲解 L312】设置窗口或控件显示状态。
    ShowWindow(usageList,logged&&page==1?SW_SHOW:SW_HIDE);
    //【讲解 L313】设置窗口或控件显示状态；读取数组元素数。
    ShowWindow(scheduleOptions,logged&&page==0&&cJSON_GetArraySize(scheduleCandidates)>0?SW_SHOW:SW_HIDE);
    //【讲解 L314】设置窗口或控件显示状态；读取数组元素数；调用 lab_agent_proposals，借用最近响应中的候选方案数组。
    ShowWindow(agentOptions,logged&&page==4&&cJSON_GetArraySize(lab_agent_proposals(&agent))>0?SW_SHOW:SW_HIDE);
    //【讲解 L315】设置窗口或控件显示状态。
    ShowWindow(agentReply,logged&&page==4?SW_SHOW:SW_HIDE);
    //【讲解 L316】设置窗口或控件显示状态。
    ShowWindow(list, logged&&page!=4 ? SW_SHOW : SW_HIDE);
    //【讲解 L317】设置窗口或控件显示状态。
    ShowWindow(whoText, logged ? SW_SHOW : SW_HIDE);
    //【讲解 L318】设置窗口或控件显示状态。
    ShowWindow(dateEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    //【讲解 L319】设置窗口或控件显示状态。
    ShowWindow(startEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    //【讲解 L320】设置窗口或控件显示状态。
    ShowWindow(endEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    //【讲解 L321】设置窗口或控件显示状态。
    ShowWindow(noteEdit, logged ? SW_SHOW : SW_HIDE);
    //【讲解 L322】设置窗口或控件显示状态。
    ShowWindow(helpText, logged ? SW_SHOW : SW_HIDE);
    //【讲解 L323】按本行初值、条件和步长循环。
    for (int i = 0; i < 40; i++)
        //【讲解 L324】设置窗口或控件显示状态。
        ShowWindow(deviceButtons[i], logged && page == 0 ? SW_SHOW : SW_HIDE);
    //【讲解 L325】按本行初值、条件和步长循环。
    for (int i = 0; i < 19; i++) {
        //【讲解 L326】声明或更新 logged（登录标记）；按本行表达式保存对应状态。
        int show = logged;
        //【讲解 L327】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        int id = actionIds[i];
        //【讲解 L328】检查本行条件，满足后执行紧随其后的语句或代码块；调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
        if(id==ID_EXPORT_BOOKINGS||id==ID_EXPORT_ISSUES)show=logged&&admin()&&(page==2||page==3);
        //【讲解 L329】检查本行条件，满足后执行紧随其后的语句或代码块；读取数组元素数。
        if(id==ID_USE_CANDIDATE)show=logged&&page==0&&cJSON_GetArraySize(scheduleCandidates)>0;
        //【讲解 L330】检查本行条件，满足后执行紧随其后的语句或代码块；读取数组元素数；调用 lab_agent_proposals，借用最近响应中的候选方案数组。
        if(id==ID_AGENT_CONFIRM)show=logged&&page==4&&cJSON_GetArraySize(lab_agent_proposals(&agent))>0;
        //【讲解 L331】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id == ID_AGENT)
            //【讲解 L332】调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
            show = logged && (page == 4 || (page == 3 && admin()));
        //【讲解 L333】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id == ID_PROFILE)show=logged&&page==1;
        //【讲解 L334】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id == ID_BOOK || id == ID_SUGGEST)
            //【讲解 L335】声明或更新 logged（登录标记）、page（当前页面编号）；按本行表达式保存对应状态。
            show = logged && page == 0;
        //【讲解 L336】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id == ID_ATTEND || id == ID_CANCEL)
            //【讲解 L337】调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
            show = logged && (page==1||(id==ID_CANCEL&&page==0&&admin()));
        //【讲解 L338】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id == ID_REPORT)
            //【讲解 L339】声明或更新 logged（登录标记）、page（当前页面编号）；按本行表达式保存对应状态。
            show = logged && (page == 2 || page == 0);
        //【讲解 L340】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id == ID_REPAIR || id == ID_ARCHIVE)
            //【讲解 L341】调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
            show = logged && page == 2 && admin();
        //【讲解 L342】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id == ID_DEACTIVATE || id == ID_INVITE || id == ID_METRICS)
            //【讲解 L343】调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
            show = logged && page == 3 && admin();
        //【讲解 L344】设置窗口或控件显示状态。
        ShowWindow(actions[i], show ? SW_SHOW : SW_HIDE);
        //【讲解 L345】设置控件能否操作。
        EnableWindow(actions[i], !busy && (id != ID_UPDATE || !checkingUpdate));
    //【讲解 L346】结束当前代码块。
    }
    //【讲解 L347】设置控件能否操作。
    EnableWindow(loginButton, !busy);
    //【讲解 L348】设置控件能否操作。
    EnableWindow(registerButton, !busy);
    //【讲解 L349】设置控件能否操作。
    EnableWindow(tabs, !busy);
    //【讲解 L350】更新控件文字。
    SetWindowTextW(
        //【讲解 L351】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
        helpText, page == 0 ? L"选择日期、开始/结束小时和设备；多小时预约逐时提交。设备颜色对应所选整个时段。"
                  //【讲解 L352】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
                  : page == 1
                      //【讲解 L353】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                      ? L"选择自己的预约，再点击签到 / 签退。签到仅限预约时段（北京时间），签退需先签到。"
                  //【讲解 L354】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
                  : page == 4 ? L"根据设备、日期和时段获取建议；确认后再预约。"
                  //【讲解 L355】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
                  : page == 2 ? L"在说明框输入故障/维修说明。报告故障使用预约页中选中的第一台设备。"
                              //【讲解 L356】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                              : L"管理操作由云端验证权限。修改本人资料：说明框输入“姓名|学号或邮箱”。");
//【讲解 L357】结束当前代码块。
}
//【讲解 L358】函数 selected_row：从列表当前选择取回模型记录；任务启动前需要复制记录以避免刷新后悬空。
static cJSON *selected_row(void) {
    //【讲解 L359】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
    int i = ListView_GetNextItem(page==1&&selectedTable?usageList:list, -1, LVNI_SELECTED);
    //【讲解 L360】结束当前函数并返回 i >= 0 && i < listedCount ? listed[i] : NULL；具体成功或失败含义见函数说明。
    return i >= 0 && i < listedCount ? listed[i] : NULL;
//【讲解 L361】结束当前代码块。
}
//【讲解 L362】函数 parameter：给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
static void parameter(cJSON *p, const char *key, const char *s) { cJSON_AddStringToObject(p, key, s); }
//【讲解 L363】函数 worker：后台线程执行登录、预约、签到、管理 RPC 或助手请求，整理结果并安排界面线程接收；不直接绘制界面。
static DWORD WINAPI worker(void *arg) {
    //【讲解 L364】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    Job *j = arg;
    //【讲解 L365】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    cJSON *p = NULL, *r = NULL;
    //【讲解 L366】声明或更新 committed（已成功提交的小时数）；按本行表达式保存对应状态。
    int success = 0, committed = 0;
    //【讲解 L367】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (j->kind == JOB_UPDATE) {
        //【讲解 L368】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
        int status = 0;
        //【讲解 L369】调用 lab_win_http，Windows 传输回调。
        char *raw = lab_win_http(NULL, LAB_SITE_HOST, "GET", LAB_SITE_BASE "native-desktop.json", "", "", "",
                                 //【讲解 L370】声明或更新 error（错误文本）、status（状态码或业务状态）；按本行表达式保存对应状态。
                                 &status, j->client.error, sizeof(j->client.error));
        //【讲解 L371】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (raw) {
            //【讲解 L372】解析 JSON 文本。
            r = cJSON_Parse(raw);
            //【讲解 L373】释放堆内存。
            free(raw);
        //【讲解 L374】结束当前代码块。
        }
        //【讲解 L375】检查本行条件，满足后执行紧随其后的语句或代码块；调用 lab_semver_newer，只比较三段纯数字版本号；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        if (status == 200 && r && lab_semver_newer(lab_str(r, "version"), LAB_NATIVE_VERSION)) {
            //【讲解 L376】按容量格式化 UTF-8 文本。
            snprintf(j->message, sizeof(j->message), "发现原生桌面新版本 %s。是否打开官网下载页？",
                     //【讲解 L377】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
                     lab_str(r, "version"));
            //【讲解 L378】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            j->ok = 1;
        //【讲解 L379】结束当前代码块。
        }
        //【讲解 L380】释放 JSON 对象及其子节点。
        cJSON_Delete(r);
        //【讲解 L381】转到 post 统一处理，避免遗漏资源清理。
        goto post;
    //【讲解 L382】结束当前代码块。
    }
    //【讲解 L383】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (j->kind == JOB_LOGIN)
        //【讲解 L384】调用 lab_login，支持邮箱或学号登录。
        success = lab_login(&j->client, j->identity, j->password);
    //【讲解 L385】上一条件不成立时检查下一条件。
    else if (j->kind == JOB_REGISTER)
        //【讲解 L386】调用 lab_register，组织学生或带邀请码的注册参数，调用云端注册流程。
        success = lab_register(&j->client, j->name, j->identity, j->password, j->invite);
    //【讲解 L387】上一条件不成立时检查下一条件。
    else if (j->kind == JOB_REFRESH)
        //【讲解 L388】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        success = 1;
    //【讲解 L389】上一条件不成立时检查下一条件。
    else if (j->kind == JOB_BOOK) {
        //【讲解 L390】声明或更新 keys（每小时幂等键）；按本行表达式保存对应状态。
        const char *keys[9];
        //【讲解 L391】按本行初值、条件和步长循环。
        for (int i = 0; i < 9; i++)
            //【讲解 L392】声明或更新 keys（每小时幂等键）；按本行表达式保存对应状态。
            keys[i] = j->keys[i];
        //【讲解 L393】调用 lab_reserve_range，按小时提交多个设备，每小时使用一个幂等键。
        success = lab_reserve_range(&j->client, j->date, j->start, j->end, j->devices, (size_t)j->count, keys,
                                    //【讲解 L394】声明或更新 committed（已成功提交的小时数）；按本行表达式保存对应状态。
                                    &committed);
        //【讲解 L395】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!success)
            //【讲解 L396】按容量格式化 UTF-8 文本。
            snprintf(j->message, sizeof(j->message),
                     //【讲解 L397】声明或更新 committed（已成功提交的小时数）；按本行表达式保存对应状态。
                     "已确认成功 %d 个小时；本次未能完成全部提交，请刷新核实剩余小时。%s", committed,
                     //【讲解 L398】声明或更新 error（错误文本）；按本行表达式保存对应状态。
                     j->client.error);
    //【讲解 L399】上一条件不成立时检查下一条件。
    } else if (j->kind == JOB_ATTEND)
        //【讲解 L400】调用 lab_attend，校验预约归属及动作，再调用 v2_check_in 或 v2_check_out；调用 now_ms，取得当前 Unix 毫秒。
        success = lab_attend(&j->client, j->row, j->action, now_ms());
    //【讲解 L401】上一条件不成立时检查下一条件。
    else if (j->kind == JOB_AGENT) {
        //【讲解 L402】调用 lab_request，对外提供统一请求入口，开启一次令牌刷新重试。
        r = lab_request(&j->client,"POST","/functions/v1/deepseek-booking-agent",j->agentRequest);
        //【讲解 L403】声明或更新 agentResult（助手响应对象）；按本行表达式保存对应状态。
        j->agentResult=r;
        //【讲解 L404】比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        success=r && strcmp(lab_str(r,"outcome"),"failed");
        //【讲解 L405】检查本行条件，满足后执行紧随其后的语句或代码块；按精确字段名借用 JSON 子节点（字段 errors）。
        if(r){const cJSON *errors=cJSON_GetObjectItemCaseSensitive(r,"errors");
            //【讲解 L406】按容量格式化 UTF-8 文本；读取数组元素数；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            snprintf(j->message,sizeof(j->message),"%s",cJSON_GetArraySize(errors)?lab_str(cJSON_GetArrayItem(errors,0),"message"):lab_str(r,"message"));}

    //【讲解 L408】处理另一分支。
    } else {
        //【讲解 L409】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        const char *rpc = NULL;
        //【讲解 L410】创建 JSON 对象。
        p = cJSON_CreateObject();
        //【讲解 L411】根据消息或任务类型选择处理分支。
        switch (j->kind) {
        //【讲解 L412】处理 JOB_CANCEL 对应的事件或任务。
        case JOB_CANCEL:
            //【讲解 L413】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = "v2_cancel_reservation";
            //【讲解 L414】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            parameter(p, "p_reservation_id", lab_str(j->row, "id"));
            //【讲解 L415】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L416】处理 JOB_REPORT 对应的事件或任务。
        case JOB_REPORT:
            //【讲解 L417】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = "report_device_fault";
            //【讲解 L418】写入数值字段（字段 p_device_number）。
            cJSON_AddNumberToObject(p, "p_device_number", j->devices[0]);
            //【讲解 L419】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
            parameter(p, "p_description", j->note);
            //【讲解 L420】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L421】处理 JOB_REPAIR 对应的事件或任务。
        case JOB_REPAIR:
            //【讲解 L422】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = "v2_complete_fault_repair";
            //【讲解 L423】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            parameter(p, "p_issue_id", lab_str(j->row, "id"));
            //【讲解 L424】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
            parameter(p, "p_note", j->note);
            //【讲解 L425】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L426】处理 JOB_ARCHIVE 对应的事件或任务。
        case JOB_ARCHIVE:
            //【讲解 L427】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = "archive_resolved_issue";
            //【讲解 L428】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            parameter(p, "p_issue_id", lab_str(j->row, "id"));
            //【讲解 L429】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L430】处理 JOB_DEACTIVATE 对应的事件或任务。
        case JOB_DEACTIVATE:
            //【讲解 L431】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = "deactivate_student";
            //【讲解 L432】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            parameter(p, "target_user", lab_str(j->row, "id"));
            //【讲解 L433】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L434】处理 JOB_INVITE 对应的事件或任务。
        case JOB_INVITE:
            //【讲解 L435】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = "create_admin_invite_for_days";
            //【讲解 L436】写入数值字段（字段 p_valid_days）。
            cJSON_AddNumberToObject(p, "p_valid_days", j->inviteDays);
            //【讲解 L437】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L438】处理 JOB_METRICS 对应的事件或任务。
        case JOB_METRICS:
            //【讲解 L439】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = NULL;
            //【讲解 L440】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
            parameter(p, "p_start_date", j->date);
            //【讲解 L441】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
            parameter(p, "p_end_date", j->date);
            //【讲解 L442】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L443】处理 JOB_PROFILE 对应的事件或任务。
        case JOB_PROFILE: {
            //【讲解 L444】查找指定分隔字符。
            char *sep = strchr(j->note, '|');
            //【讲解 L445】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (!sep) {
                //【讲解 L446】按容量格式化 UTF-8 文本。
                snprintf(j->client.error, sizeof(j->client.error), "请在说明框输入：姓名|学号或邮箱");
                //【讲解 L447】结束当前最近的循环或 switch 分支。
                break;
            //【讲解 L448】结束当前代码块。
            }
            *sep++ = 0;
            //【讲解 L450】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = "update_profile_identity";
            //【讲解 L451】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
            parameter(p, "target_user", j->client.session.uid);
            //【讲解 L452】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
            parameter(p, "next_display_name", j->note);
            //【讲解 L453】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
            parameter(p, "next_login_id", sep);
            //【讲解 L454】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L455】结束当前代码块。
        }
        //【讲解 L456】处理 JOB_SUGGEST 对应的事件或任务。
        case JOB_SUGGEST:
            //【讲解 L457】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            rpc = "v2_schedule_candidates";
            //【讲解 L458】调用 parameter，给 RPC 参数对象添加字符串字段，字段名要与云端函数参数一致。
            parameter(p, "p_date", j->date);
            //【讲解 L459】写入数值字段（字段 p_preferred_hour）。
            cJSON_AddNumberToObject(p, "p_preferred_hour", j->start);
            //【讲解 L460】写入数值字段（字段 p_required_device_count）。
            cJSON_AddNumberToObject(p, "p_required_device_count", j->count ? j->count : 1);
            //【讲解 L461】把节点所有权交给对象；创建 JSON 数组（字段 p_preferred_devices）。
            cJSON_AddItemToObject(p, "p_preferred_devices", cJSON_CreateArray());
            //【讲解 L462】写入数值字段（字段 p_limit）。
            cJSON_AddNumberToObject(p, "p_limit", 3);
            //【讲解 L463】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L464】结束当前代码块。
        }
        //【讲解 L465】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (j->kind == JOB_METRICS)
            //【讲解 L466】调用 lab_usage_metrics，调用 v3_admin_usage_metrics，device=0 表示全部设备。
            r = lab_usage_metrics(&j->client,j->date,j->endDate,j->metricDevice);
        //【讲解 L467】上一条件不成立时检查下一条件。
        else if (rpc)
            //【讲解 L468】调用 lab_rpc，检查 RPC 名称只包含允许的字符，再构造 /rest/v1/rpc/ 路径并发送 POST。
            r = lab_rpc(&j->client, rpc, p);
        //【讲解 L469】释放 JSON 对象及其子节点。
        cJSON_Delete(p);
        //【讲解 L470】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        success = r != NULL;
        //【讲解 L471】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (success && j->kind == JOB_INVITE) {
            //【讲解 L472】按下标借用数组元素。
            const cJSON *v = cJSON_IsArray(r) ? cJSON_GetArrayItem(r, 0) : r;
            //【讲解 L473】按容量格式化 UTF-8 文本。
            snprintf(j->message, sizeof(j->message),
                     //【讲解 L474】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                     "管理员邀请码：%s\n有效期至：%s\n请妥善保管，仅分享给授权管理员。",
                     //【讲解 L475】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
                     lab_str(v, "invite_code"), lab_str(v, "expires_at"));
        //【讲解 L476】结束当前代码块。
        }
        //【讲解 L477】检查本行条件，满足后执行紧随其后的语句或代码块；调用 lab_metrics_report，将云端统计结果转换为总览、逐设备和逐时段报表。
        if(success && j->kind==JOB_METRICS)lab_metrics_report(r,j->message,sizeof(j->message));
        //【讲解 L478】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (success && j->kind == JOB_SUGGEST) {
            //【讲解 L479】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
            const cJSON *v;
            //【讲解 L480】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
            size_t used = 0;
            //【讲解 L481】按容量格式化 UTF-8 文本。
            snprintf(j->message, sizeof(j->message), "可预约方案（请选择设备后确认预约）：\n");
            //【讲解 L482】计算字符串字节长度。
            used = strlen(j->message);
            //【讲解 L483】遍历 JSON 数组。
            cJSON_ArrayForEach(v, r) {
                //【讲解 L484】按容量格式化 UTF-8 文本。
                int n = snprintf(j->message + used, sizeof(j->message) - used, "%s %02d:00，设备：",
                                 //【讲解 L485】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
                                 lab_str(v, "reservation_date"), lab_int(v, "start_hour"));
                //【讲解 L486】检查本行条件，满足后执行紧随其后的语句或代码块。
                if (n < 0 || (size_t)n >= sizeof(j->message) - used)
                    //【讲解 L487】结束当前最近的循环或 switch 分支。
                    break;
                //【讲解 L488】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                used += (size_t)n;
                //【讲解 L489】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
                const cJSON *device;
                //【讲解 L490】遍历 JSON 数组；按精确字段名借用 JSON 子节点（字段 device_numbers）。
                cJSON_ArrayForEach(device, cJSON_GetObjectItemCaseSensitive(v, "device_numbers")) {
                    //【讲解 L491】按容量格式化 UTF-8 文本。
                    n = snprintf(j->message + used, sizeof(j->message) - used, "%02d ", device->valueint);
                    //【讲解 L492】检查本行条件，满足后执行紧随其后的语句或代码块。
                    if (n < 0 || (size_t)n >= sizeof(j->message) - used)
                        //【讲解 L493】结束当前最近的循环或 switch 分支。
                        break;
                    //【讲解 L494】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                    used += (size_t)n;
                //【讲解 L495】结束当前代码块。
                }
                //【讲解 L496】检查本行条件，满足后执行紧随其后的语句或代码块。
                if (used + 2 < sizeof(j->message)) {
                    //【讲解 L497】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                    j->message[used++] = '\n';
                    //【讲解 L498】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                    j->message[used] = 0;
                //【讲解 L499】结束当前代码块。
                }
            //【讲解 L500】结束当前代码块。
            }
            //【讲解 L501】检查本行条件，满足后执行紧随其后的语句或代码块；读取数组元素数。
            if (cJSON_GetArraySize(r) == 0)
                //【讲解 L502】按容量格式化 UTF-8 文本。
                snprintf(j->message, sizeof(j->message), "当前条件下没有可预约的方案。");
        //【讲解 L503】结束当前代码块。
        }
        //【讲解 L504】检查本行条件，满足后执行紧随其后的语句或代码块。
        if(j->kind==JOB_SUGGEST && success){j->agentResult=r;r=NULL;}
        //【讲解 L505】释放 JSON 对象及其子节点。
        cJSON_Delete(r);
    //【讲解 L506】结束当前代码块。
    }
    //【讲解 L507】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    j->ok = success;
    //【讲解 L508】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!success && !*j->message)
        //【讲解 L509】按容量格式化 UTF-8 文本。
        snprintf(j->message, sizeof(j->message), "%s", j->client.error);
    //【讲解 L510】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (*j->client.session.access) {
        //【讲解 L511】调用 lab_load_model，先加载账号资料并确认 active，再加载设备、预约、故障及使用记录。
        j->loaded = lab_load_model(&j->client, &j->model);
        //【讲解 L512】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (success && !j->loaded) {
            //【讲解 L513】按容量格式化 UTF-8 文本。
            snprintf(j->message, sizeof(j->message), "操作已完成，但刷新失败：%s。请刷新核实后再操作。",
                     //【讲解 L514】声明或更新 error（错误文本）；按本行表达式保存对应状态。
                     j->client.error);
        //【讲解 L515】结束当前代码块。
        }
    //【讲解 L516】结束当前代码块。
    }
//【讲解 L517】整理后台任务结束状态，将结果交给界面线程。
post:
    //【讲解 L518】清除敏感临时数据。
    SecureZeroMemory(j->password, sizeof(j->password));
    //【讲解 L519】检查本行条件，满足后执行紧随其后的语句或代码块；把后台结果投递给主窗口。
    if (!PostMessageW(j->target, RESULT_MESSAGE, 0, (LPARAM)j)) {
        //【讲解 L520】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体。
        lab_model_free(&j->model);
        //【讲解 L521】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
        lab_session_clear(&j->client.session);
        //【讲解 L522】释放 JSON 对象及其子节点。
        cJSON_Delete(j->row);cJSON_Delete(j->agentRequest);cJSON_Delete(j->agentResult);
        //【讲解 L523】释放堆内存。
        free(j);
    //【讲解 L524】结束当前代码块。
    }
    //【讲解 L525】结束当前函数并返回 0；具体成功或失败含义见函数说明。
    return 0;
//【讲解 L526】结束当前代码块。
}
//【讲解 L527】函数 start_job：保存任务快照、设置忙状态并启动后台线程；创建失败时恢复界面并释放任务。
static void start_job(Job *j) {
    //【讲解 L528】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    HANDLE thread;
    //【讲解 L529】声明或更新 window（主窗口）；按本行表达式保存对应状态。
    j->target = window;
    //【讲解 L530】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    j->client = client;
    //【讲解 L531】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (j->kind == JOB_UPDATE)
        //【讲解 L532】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        checkingUpdate = 1;
    //【讲解 L533】处理另一分支。
    else
        //【讲解 L534】声明或更新 busy（请求忙标记）；按本行表达式保存对应状态。
        busy = 1;
    //【讲解 L535】调用 visibility，根据登录态、页面、角色及 busy 状态切换控件可见性和可操作性。
    visibility();
    //【讲解 L536】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (j->kind != JOB_UPDATE)
        //【讲解 L537】更新控件文字。
        SetWindowTextW(statusText, L"正在连接云端，请稍候…");
    //【讲解 L538】创建后台工作线程。
    thread = CreateThread(NULL, 0, worker, j, 0, NULL);
    //【讲解 L539】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (thread)
        //【讲解 L540】释放线程句柄引用。
        CloseHandle(thread);
    //【讲解 L541】处理另一分支。
    else {
        //【讲解 L542】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (j->kind == JOB_UPDATE)
            //【讲解 L543】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            checkingUpdate = 0;
        //【讲解 L544】处理另一分支。
        else
            //【讲解 L545】声明或更新 busy（请求忙标记）；按本行表达式保存对应状态。
            busy = 0;
        //【讲解 L546】调用 visibility，根据登录态、页面、角色及 busy 状态切换控件可见性和可操作性。
        visibility();
        //【讲解 L547】释放 JSON 对象及其子节点。
        cJSON_Delete(j->row);cJSON_Delete(j->agentRequest);cJSON_Delete(j->agentResult);
        //【讲解 L548】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
        lab_session_clear(&j->client.session);
        //【讲解 L549】释放堆内存。
        free(j);
        //【讲解 L550】调用 tell，显示平台消息提示。
        tell("无法创建请求线程，请重试。", MB_ICONERROR);
    //【讲解 L551】结束当前代码块。
    }
//【讲解 L552】结束当前代码块。
}
//【讲解 L553】函数 report_proc：Windows 报表窗口消息处理函数，创建只读文本控件并处理尺寸、关闭等消息。
static LRESULT CALLBACK report_proc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
 //【讲解 L554】检查本行条件，满足后执行紧随其后的语句或代码块；调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free；释放堆内存；同步读写控件状态。
 if(msg==WM_CREATE){wchar_t *text=wide((const char*)((CREATESTRUCTW*)lp)->lpCreateParams);HWND edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",text,WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,12,12,760,520,h,(HMENU)210,NULL,NULL);free(text);SendMessageW(edit,WM_SETFONT,(WPARAM)font,TRUE);return 0;}
 //【讲解 L555】检查本行条件，满足后执行紧随其后的语句或代码块；调整控件位置和尺寸。
 if(msg==WM_SIZE){MoveWindow(GetDlgItem(h,210),12,12,LOWORD(lp)-24,HIWORD(lp)-24,TRUE);return 0;}
 //【讲解 L556】检查本行条件，满足后执行紧随其后的语句或代码块；交给 Windows 默认消息处理。
 if(msg==WM_CLOSE){DestroyWindow(h);return 0;}return DefWindowProcW(h,msg,wp,lp);
//【讲解 L557】结束当前代码块。
}
//【讲解 L558】函数 native_report：显示可滚动的原生文字报表窗口；目前不是网页版图表的完整视觉复刻。
static void native_report(const char *title,const char *text){
 //【讲解 L559】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
 WNDCLASSW cls={0};cls.lpfnWndProc=report_proc;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"LabNativeReport";cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&cls);wchar_t *caption=wide(title);
 //【讲解 L560】释放堆内存。
 HWND dialog=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,caption,WS_OVERLAPPEDWINDOW,100,100,840,640,window,NULL,cls.hInstance,(void*)text);free(caption);if(!dialog)return;
 //【讲解 L561】设置控件能否操作；设置窗口或控件显示状态；等待下一个窗口消息。
 int previousBusy=busy;busy=1;EnableWindow(window,FALSE);ShowWindow(dialog,SW_SHOW);MSG message;while(IsWindow(dialog)&&GetMessageW(&message,NULL,0,0)>0){if(!IsDialogMessageW(dialog,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}EnableWindow(window,TRUE);SetActiveWindow(window);busy=previousBusy;
//【讲解 L562】结束当前代码块。
}
//【讲解 L563】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef struct {const char **labels;char (*values)[256];int count,accepted;HWND fields[3];} NativeForm;
//【讲解 L564】函数 form_proc：Windows 表单窗口消息处理：创建输入框，响应确定、取消并收集输入。
static LRESULT CALLBACK form_proc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
 //【讲解 L565】读取窗口状态或附加指针。
 NativeForm *f=(NativeForm*)GetWindowLongPtrW(h,GWLP_USERDATA);
 //【讲解 L566】检查本行条件，满足后执行紧随其后的语句或代码块；写入窗口状态或附加指针。
 if(msg==WM_CREATE){f=(NativeForm*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)f);
 //【讲解 L567】按本行初值、条件和步长循环；调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free；同步读写控件状态；释放堆内存。
 for(int i=0;i<f->count;i++){wchar_t *caption=wide(f->labels[i]),*value=wide(f->values[i]);HWND label=CreateWindowW(L"STATIC",caption,WS_CHILD|WS_VISIBLE,20,16+i*76,450,24,h,NULL,NULL,NULL);f->fields[i]=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",value,WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,20,42+i*76,450,30,h,(HMENU)(INT_PTR)(200+i),NULL,NULL);SendMessageW(label,WM_SETFONT,(WPARAM)font,TRUE);SendMessageW(f->fields[i],WM_SETFONT,(WPARAM)font,TRUE);SendMessageW(f->fields[i],EM_SETLIMITTEXT,255,0);free(caption);free(value);}
 //【讲解 L568】同步读写控件状态。
 HWND ok=CreateWindowW(L"BUTTON",L"确定",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,260,24+f->count*76,100,34,h,(HMENU)IDOK,NULL,NULL);HWND cancel=CreateWindowW(L"BUTTON",L"取消",WS_CHILD|WS_VISIBLE|WS_TABSTOP,370,24+f->count*76,100,34,h,(HMENU)IDCANCEL,NULL,NULL);SendMessageW(ok,WM_SETFONT,(WPARAM)font,TRUE);SendMessageW(cancel,WM_SETFONT,(WPARAM)font,TRUE);return 0;}
 //【讲解 L569】检查本行条件，满足后执行紧随其后的语句或代码块；调用 get_utf8，读取 Windows 控件宽字符文本，转换为业务层 UTF-8，写入有限容量缓冲区。
 if(msg==WM_COMMAND&&(LOWORD(wp)==IDOK||LOWORD(wp)==IDCANCEL)){if(LOWORD(wp)==IDOK){for(int i=0;i<f->count;i++)get_utf8(f->fields[i],f->values[i],256);f->accepted=1;}DestroyWindow(h);return 0;}
 //【讲解 L570】检查本行条件，满足后执行紧随其后的语句或代码块；交给 Windows 默认消息处理。
 if(msg==WM_CLOSE){DestroyWindow(h);return 0;}return DefWindowProcW(h,msg,wp,lp);
//【讲解 L571】结束当前代码块。
}
//【讲解 L572】函数 native_form：显示少量字段的原生输入表单，接受时写回 values；调用者继续做业务参数校验。
static int native_form(const char *title,const char *labels[],char values[][256],int count){
 //【讲解 L573】执行 GetModuleHandleW、LoadCursorW、RegisterClassW，参数沿用本行给出的对象、标志和容量。
 WNDCLASSW cls={0};cls.lpfnWndProc=form_proc;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"LabNativeForm";cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&cls);
 //【讲解 L574】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
 NativeForm f={labels,values,count,0,{0}};wchar_t *caption=wide(title);RECT r;GetWindowRect(window,&r);
 //【讲解 L575】释放堆内存。
 HWND dialog=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,caption,WS_CAPTION|WS_SYSMENU,r.left+150,r.top+100,510,115+count*76,window,NULL,cls.hInstance,&f);free(caption);if(!dialog)return 0;
 //【讲解 L576】设置控件能否操作；设置窗口或控件显示状态。
 int previousBusy=busy;busy=1;EnableWindow(window,FALSE);ShowWindow(dialog,SW_SHOW);SetFocus(f.fields[0]);MSG message;
 //【讲解 L577】条件成立时继续循环；等待下一个窗口消息；转换键盘消息；派发消息给窗口过程。
 while(IsWindow(dialog)&&GetMessageW(&message,NULL,0,0)>0){if(!IsDialogMessageW(dialog,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
 //【讲解 L578】设置控件能否操作。
 EnableWindow(window,TRUE);SetActiveWindow(window);busy=previousBusy;return f.accepted;
//【讲解 L579】结束当前代码块。
}
//【讲解 L580】函数 export_records：弹出原生保存对话框，再调用共享 CSV 输出函数，报告文件写入结果。
static void export_records(int issues){
 //【讲解 L581】声明或更新 issues（故障列表）、window（主窗口）；按本行表达式保存对应状态。
 wchar_t path[MAX_PATH];wcscpy(path,issues?L"issues.csv":L"reservations.csv");OPENFILENAMEW save={0};save.lStructSize=sizeof(save);save.hwndOwner=window;save.lpstrFile=path;save.nMaxFile=MAX_PATH;save.lpstrFilter=L"CSV 文件\0*.csv\0\0";save.lpstrDefExt=L"csv";save.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 //【讲解 L582】检查本行条件，满足后执行紧随其后的语句或代码块；调用 lab_export_csv，以 UTF-8 BOM 输出当前已加载模型中的全部预约或故障；关闭文件；调用 tell，显示平台消息提示。
 if(GetSaveFileNameW(&save)){FILE *out=_wfopen(path,L"wb");int ok=out&&lab_export_csv(&model,issues,out);if(out&&fclose(out)!=0)ok=0;tell(ok?"完整记录已导出。":"导出失败，请检查保存位置。",ok?MB_ICONINFORMATION:MB_ICONERROR);}
//【讲解 L583】结束当前代码块。
}
//【讲解 L584】函数 command：把按钮编号映射成具体操作；先检查输入和选中记录，再创建 Job，交由 worker 异步执行。
static void command(int id) {
    //【讲解 L585】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    Job *j;
    //【讲解 L586】声明或更新 row（当前模型记录）；按本行表达式保存对应状态。
    cJSON *row;
    //【讲解 L587】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char confirmation[512];
    //【讲解 L588】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (busy || preview)
        //【讲解 L589】结束当前函数并返回 ；具体成功或失败含义见函数说明。
        return;
    //【讲解 L590】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (id == ID_LOGOUT) {
        //【讲解 L591】调用 lab_agent_clear，释放助手历史及最近响应并清空状态；释放 JSON 对象及其子节点。
        lab_agent_clear(&agent);cJSON_Delete(scheduleCandidates);scheduleCandidates=NULL;
        //【讲解 L592】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
        lab_session_clear(&client.session);
        //【讲解 L593】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体。
        lab_model_free(&model);
        //【讲解 L594】声明或更新 logged（登录标记）；按本行表达式保存对应状态。
        logged = 0;
        //【讲解 L595】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        scrollY=0;selectedId[0]=0;
        //【讲解 L596】声明或更新 listedCount（已显示记录数）；按本行表达式保存对应状态。
        listedCount = 0;
        //【讲解 L597】执行 ListView_DeleteAllItems，参数沿用本行给出的对象、标志和容量。
        ListView_DeleteAllItems(list);
        //【讲解 L598】更新控件文字。
        SetWindowTextW(password, L"");
        //【讲解 L599】调用 layout，按窗口、页面和滚动状态排列控件。
        layout();
        //【讲解 L600】结束当前函数并返回 ；具体成功或失败含义见函数说明。
        return;
    //【讲解 L601】结束当前代码块。
    }
    //【讲解 L602】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(id==ID_USE_CANDIDATE){
 //【讲解 L603】按下标借用数组元素；同步读写控件状态。
 const cJSON *candidate=cJSON_GetArrayItem(scheduleCandidates,(int)SendMessageW(scheduleOptions,CB_GETCURSEL,0,0));
 //【讲解 L604】按精确字段名借用 JSON 子节点；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针（字段 device_numbers）。
 const cJSON *numbers=cJSON_GetObjectItemCaseSensitive(candidate,"device_numbers");int hour=lab_int(candidate,"start_hour");const char *date=lab_str(candidate,"reservation_date");
 //【讲解 L605】检查本行条件，满足后执行紧随其后的语句或代码块；计算字符串字节长度；读取数组元素数。
 if(strlen(date)!=10||hour<8||hour>16||!cJSON_GetArraySize(numbers))return;
 //【讲解 L606】从文本解析字段；同步读写控件状态；初始化或清空指定内存。
 SYSTEMTIME time={0};int year,month,day;if(sscanf(date,"%d-%d-%d",&year,&month,&day)!=3)return;time.wYear=(WORD)year;time.wMonth=(WORD)month;time.wDay=(WORD)day;DateTime_SetSystemtime(dateEdit,GDT_VALID,&time);SendMessageW(startEdit,CB_SETCURSEL,hour-8,0);SendMessageW(endEdit,CB_SETCURSEL,hour-8,0);memset(selected,0,sizeof(selected));const cJSON *n;cJSON_ArrayForEach(n,numbers)if(n->valueint>=1&&n->valueint<=40)selected[n->valueint-1]=1;
 //【讲解 L607】调用 render，按页面、账号、日期和设备状态更新控件内容。
 render();return;
 //【讲解 L608】结束当前代码块。
 }
    //【讲解 L609】检查本行条件，满足后执行紧随其后的语句或代码块；调用 admin，根据云端加载的 session.role 判断是否显示管理入口；调用 export_records，弹出原生保存对话框，再调用共享 CSV 输出函数，报告文件写入结果。
    if(id==ID_EXPORT_BOOKINGS||id==ID_EXPORT_ISSUES){if(admin())export_records(id==ID_EXPORT_ISSUES);return;}
    //【讲解 L610】申请并清零堆内存。
    j = calloc(1, sizeof(*j));
    //【讲解 L611】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!j)
        //【讲解 L612】结束当前函数并返回 ；具体成功或失败含义见函数说明。
        return;
    //【讲解 L613】调用 date_string，将日期控件的年月日格式化为 YYYY-MM-DD。
    date_string(j->date);
    //【讲解 L614】同步读写控件状态。
    j->start = 8 + (int)SendMessageW(startEdit, CB_GETCURSEL, 0, 0);
    //【讲解 L615】同步读写控件状态。
    j->end = 9 + (int)SendMessageW(endEdit, CB_GETCURSEL, 0, 0);
    //【讲解 L616】调用 get_utf8，读取 Windows 控件宽字符文本，转换为业务层 UTF-8，写入有限容量缓冲区。
    get_utf8(noteEdit, j->note, sizeof(j->note));
    //【讲解 L617】按本行初值、条件和步长循环。
    for (int i = 0; i < 40; i++)
        //【讲解 L618】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (selected[i])
            //【讲解 L619】声明或更新 devices（设备列表）；按本行表达式保存对应状态。
            j->devices[j->count++] = i + 1;
    //【讲解 L620】检查本行条件，满足后执行紧随其后的语句或代码块；按容量格式化 UTF-8 文本。
    if(id==ID_METRICS){const char *labels[]={"开始日期 YYYY-MM-DD","结束日期 YYYY-MM-DD","设备编号（0 为全部，1–40）"};char fields[3][256];snprintf(fields[0],256,"%s",j->date);snprintf(fields[1],256,"%s",j->date);snprintf(fields[2],256,"0");
        //【讲解 L621】检查本行条件，满足后执行紧随其后的语句或代码块；调用 native_form，显示少量字段的原生输入表单，接受时写回 values。
        if(!native_form("实验室使用情况",labels,fields,3))goto abort;
        //【讲解 L622】检查本行条件，满足后执行紧随其后的语句或代码块；计算字符串字节长度；调用 tell，显示平台消息提示。
        if(strlen(fields[0])!=10||strlen(fields[1])!=10||strspn(fields[2],"0123456789")!=strlen(fields[2])||!*fields[2]||atoi(fields[2])>40){tell("请填写有效日期和设备编号。",MB_ICONINFORMATION);goto abort;}
        //【讲解 L623】按容量格式化 UTF-8 文本。
        snprintf(j->date,16,"%.10s",fields[0]);snprintf(j->endDate,16,"%.10s",fields[1]);j->metricDevice=atoi(fields[2]);
    //【讲解 L624】结束当前代码块。
    }
    //【讲解 L625】检查本行条件，满足后执行紧随其后的语句或代码块；调用 native_form，显示少量字段的原生输入表单，接受时写回 values。
    if(id==ID_INVITE){const char *labels[]={"有效天数：1、7 或 30"};char fields[1][256]={"7"};if(!native_form("生成管理员邀请码",labels,fields,1))goto abort;
        //【讲解 L626】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 tell，显示平台消息提示。
        if(strcmp(fields[0],"1")&&strcmp(fields[0],"7")&&strcmp(fields[0],"30")){tell("请选择 1、7 或 30 天。",MB_ICONINFORMATION);goto abort;}j->inviteDays=atoi(fields[0]);
    //【讲解 L627】结束当前代码块。
    }
    //【讲解 L628】检查本行条件，满足后执行紧随其后的语句或代码块；按容量格式化 UTF-8 文本；调用 native_form，显示少量字段的原生输入表单，接受时写回 values。
    if(id==ID_PROFILE){const char *labels[]={"姓名","学号 / 管理员邮箱"};char fields[2][256];snprintf(fields[0],256,"%s",client.session.name);snprintf(fields[1],256,"%s",client.session.login);if(!native_form("修改个人资料",labels,fields,2))goto abort;
        //【讲解 L629】检查本行条件，满足后执行紧随其后的语句或代码块；查找指定分隔字符；按容量格式化 UTF-8 文本。
        if(!*fields[0]||!*fields[1]||strchr(fields[0],'|')||strchr(fields[1],'|'))goto abort;snprintf(j->note,sizeof(j->note),"%s|%s",fields[0],fields[1]);
    //【讲解 L630】结束当前代码块。
    }
    //【讲解 L631】根据消息或任务类型选择处理分支。
    switch (id) {
    //【讲解 L632】处理 ID_LOGIN 对应的事件或任务。
    case ID_LOGIN:
    //【讲解 L633】处理 ID_REGISTER 对应的事件或任务。
    case ID_REGISTER:
        //【讲解 L634】调用 get_utf8，读取 Windows 控件宽字符文本，转换为业务层 UTF-8，写入有限容量缓冲区。
        get_utf8(identity, j->identity, sizeof(j->identity));
        //【讲解 L635】调用 get_utf8，读取 Windows 控件宽字符文本，转换为业务层 UTF-8，写入有限容量缓冲区。
        get_utf8(password, j->password, sizeof(j->password));
        //【讲解 L636】调用 get_utf8，读取 Windows 控件宽字符文本，转换为业务层 UTF-8，写入有限容量缓冲区。
        get_utf8(nameEdit, j->name, sizeof(j->name));
        //【讲解 L637】调用 get_utf8，读取 Windows 控件宽字符文本，转换为业务层 UTF-8，写入有限容量缓冲区。
        get_utf8(inviteEdit, j->invite, sizeof(j->invite));
        //【讲解 L638】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = id == ID_LOGIN ? JOB_LOGIN : JOB_REGISTER;
        //【讲解 L639】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!*j->identity || !*j->password) {
            //【讲解 L640】调用 tell，显示平台消息提示。
            tell("请填写学号 / 邮箱与密码。", MB_ICONINFORMATION);
            //【讲解 L641】转到 abort 统一处理，避免遗漏资源清理。
            goto abort;
        //【讲解 L642】结束当前代码块。
        }
        //【讲解 L643】结束当前最近的循环或 switch 分支。
        break;
    //【讲解 L644】处理 ID_REFRESH 对应的事件或任务。
    case ID_REFRESH:
        //【讲解 L645】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_REFRESH;
        //【讲解 L646】结束当前最近的循环或 switch 分支。
        break;
    //【讲解 L647】处理 ID_BOOK 对应的事件或任务。
    case ID_BOOK:
        //【讲解 L648】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_BOOK;
        //【讲解 L649】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!j->count || j->start >= j->end) {
            //【讲解 L650】调用 tell，显示平台消息提示。
            tell("请选择设备和有效时段。", MB_ICONINFORMATION);
            //【讲解 L651】转到 abort 统一处理，避免遗漏资源清理。
            goto abort;
        //【讲解 L652】结束当前代码块。
        }
        //【讲解 L653】按本行初值、条件和步长循环。
        for (int i = 0; i < 9; i++) {
            //【讲解 L654】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
            GUID g;
            //【讲解 L655】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (FAILED(CoCreateGuid(&g))) {
                //【讲解 L656】调用 tell，显示平台消息提示。
                tell("生成请求编号失败。", MB_ICONERROR);
                //【讲解 L657】转到 abort 统一处理，避免遗漏资源清理。
                goto abort;
            //【讲解 L658】结束当前代码块。
            }
            //【讲解 L659】按容量格式化 UTF-8 文本。
            snprintf(j->keys[i], 64, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     //【讲解 L660】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                     (unsigned long)g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
                     //【讲解 L661】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                     g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
        //【讲解 L662】结束当前代码块。
        }
        //【讲解 L663】按容量格式化 UTF-8 文本。
        snprintf(
            //【讲解 L664】执行 ，参数沿用本行给出的对象、标志和容量。
            confirmation, sizeof(confirmation),
            //【讲解 L665】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
            "预约 %s %02d:00–%02d:00，共 %d 台设备？多小时会逐时提交；若后续冲突，会保留先前成功的小时。",
            //【讲解 L666】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
            j->date, j->start, j->end, j->count);
        //【讲解 L667】转到 confirm 统一处理，避免遗漏资源清理。
        goto confirm;
    //【讲解 L668】处理 ID_ATTEND 对应的事件或任务。
    case ID_ATTEND:
    //【讲解 L669】处理 ID_CANCEL 对应的事件或任务。
    case ID_CANCEL:
    //【讲解 L670】处理 ID_REPAIR 对应的事件或任务。
    case ID_REPAIR:
    //【讲解 L671】处理 ID_ARCHIVE 对应的事件或任务。
    case ID_ARCHIVE:
    //【讲解 L672】处理 ID_DEACTIVATE 对应的事件或任务。
    case ID_DEACTIVATE:
        //【讲解 L673】调用 selected_row，从列表当前选择取回模型记录。
        row = selected_row();
        //【讲解 L674】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!row) {
            //【讲解 L675】调用 tell，显示平台消息提示。
            tell("请先选择列表中的一行。", MB_ICONINFORMATION);
            //【讲解 L676】转到 abort 统一处理，避免遗漏资源清理。
            goto abort;
        //【讲解 L677】结束当前代码块。
        }
        //【讲解 L678】深复制 JSON 节点，隔离所有权。
        j->row = cJSON_Duplicate(row, 1);
        //【讲解 L679】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id == ID_ATTEND) {
            //【讲解 L680】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            j->kind = JOB_ATTEND;
            //【讲解 L681】调用 lab_attendance_action，根据状态和时间给出可执行动作：0 不可操作，1 签到，2 签退；调用 now_ms，取得当前 Unix 毫秒。
            j->action = lab_attendance_action(row, now_ms());
            //【讲解 L682】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            if (strcmp(lab_str(row, "user_id"), client.session.uid) || !j->action) {
                //【讲解 L683】调用 tell，显示平台消息提示。
                tell("仅可操作自己的预约：在预约小时内签到，使用中可签退。请刷新核实当前状态。",
                     //【讲解 L684】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                     MB_ICONINFORMATION);
                //【讲解 L685】转到 abort 统一处理，避免遗漏资源清理。
                goto abort;
            //【讲解 L686】结束当前代码块。
            }
            //【讲解 L687】按容量格式化 UTF-8 文本。
            snprintf(confirmation, sizeof(confirmation), "确认%s设备 %02d（%s %02d:00）的预约？",
                     //【讲解 L688】调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
                     j->action == 1 ? "签到" : "签退", lab_int(row, "device_number"),
                     //【讲解 L689】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
                     lab_str(row, "reservation_date"), lab_int(row, "start_hour"));
        //【讲解 L690】处理另一分支。
        } else {
            //【讲解 L691】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            j->kind = id == ID_CANCEL    ? JOB_CANCEL
                      //【讲解 L692】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                      : id == ID_REPAIR  ? JOB_REPAIR
                      //【讲解 L693】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                      : id == ID_ARCHIVE ? JOB_ARCHIVE
                                         //【讲解 L694】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                                         : JOB_DEACTIVATE;
            //【讲解 L695】按容量格式化 UTF-8 文本。
            snprintf(confirmation, sizeof(confirmation), "确认%s所选记录？",
                     //【讲解 L696】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                     id == ID_CANCEL    ? "取消预约"
                     //【讲解 L697】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                     : id == ID_REPAIR  ? "完成维修"
                     //【讲解 L698】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                     : id == ID_ARCHIVE ? "归档故障"
                                        //【讲解 L699】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                                        : "停用学生账号");
        //【讲解 L700】结束当前代码块。
        }
        //【讲解 L701】转到 confirm 统一处理，避免遗漏资源清理。
        goto confirm;
    //【讲解 L702】处理 ID_REPORT 对应的事件或任务。
    case ID_REPORT:
        //【讲解 L703】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_REPORT;
        //【讲解 L704】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!j->count) {
            //【讲解 L705】调用 tell，显示平台消息提示。
            tell("请先到设备预约页选中一台设备。", MB_ICONINFORMATION);
            //【讲解 L706】转到 abort 统一处理，避免遗漏资源清理。
            goto abort;
        //【讲解 L707】结束当前代码块。
        }
        //【讲解 L708】按容量格式化 UTF-8 文本。
        snprintf(confirmation, sizeof(confirmation), "确认报告设备 %02d 故障？", j->devices[0]);
        //【讲解 L709】转到 confirm 统一处理，避免遗漏资源清理。
        goto confirm;
    //【讲解 L710】处理 ID_INVITE 对应的事件或任务。
    case ID_INVITE:
        //【讲解 L711】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_INVITE;
        //【讲解 L712】结束当前最近的循环或 switch 分支。
        break;
    //【讲解 L713】处理 ID_METRICS 对应的事件或任务。
    case ID_METRICS:
        //【讲解 L714】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_METRICS;
        //【讲解 L715】结束当前最近的循环或 switch 分支。
        break;
    //【讲解 L716】处理 ID_PROFILE 对应的事件或任务。
    case ID_PROFILE:
        //【讲解 L717】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_PROFILE;
        //【讲解 L718】按容量格式化 UTF-8 文本。
        snprintf(confirmation, sizeof(confirmation), "确认按说明框内容修改本人姓名和登录名？");
        //【讲解 L719】转到 confirm 统一处理，避免遗漏资源清理。
        goto confirm;
    //【讲解 L720】处理 ID_SUGGEST 对应的事件或任务。
    case ID_SUGGEST:
        //【讲解 L721】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_SUGGEST;
        //【讲解 L722】结束当前最近的循环或 switch 分支。
        break;
    //【讲解 L723】处理 ID_AGENT_CONFIRM 对应的事件或任务。
    case ID_AGENT_CONFIRM:
    //【讲解 L724】处理 ID_AGENT 对应的事件或任务。
    case ID_AGENT:
        //【讲解 L725】执行 FAILED、CoCreateGuid，参数沿用本行给出的对象、标志和容量。
        { GUID g; if(FAILED(CoCreateGuid(&g)))goto abort;
          //【讲解 L726】按容量格式化 UTF-8 文本。
          snprintf(j->keys[0],64,"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            //【讲解 L727】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
            (unsigned long)g.Data1,g.Data2,g.Data3,g.Data4[0],g.Data4[1],g.Data4[2],g.Data4[3],g.Data4[4],g.Data4[5],g.Data4[6],g.Data4[7]); }
        //【讲解 L728】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_AGENT;
        //【讲解 L729】调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
        j->action = admin();
        //【讲解 L730】结束当前最近的循环或 switch 分支。
        break;
    //【讲解 L731】处理 ID_UPDATE 对应的事件或任务。
    case ID_UPDATE:
        //【讲解 L732】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (checkingUpdate)
            //【讲解 L733】转到 abort 统一处理，避免遗漏资源清理。
            goto abort;
        //【讲解 L734】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        j->kind = JOB_UPDATE;
        //【讲解 L735】结束当前最近的循环或 switch 分支。
        break;
    //【讲解 L736】处理未被前面分支覆盖的情况。
    default:
        //【讲解 L737】转到 abort 统一处理，避免遗漏资源清理。
        goto abort;
    //【讲解 L738】结束当前代码块。
    }
    //【讲解 L739】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(j->kind==JOB_AGENT){
        //【讲解 L740】调用 lab_agent_bind，把助手状态绑定到账号。
        lab_agent_bind(&agent,client.session.uid);
        //【讲解 L741】检查本行条件，满足后执行紧随其后的语句或代码块；调用 tell，显示平台消息提示。
        if(id==ID_AGENT&&!*j->note){tell("请输入你的需求。",MB_ICONINFORMATION);goto abort;}
        //【讲解 L742】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        cJSON *confirmation=NULL;
        //【讲解 L743】检查本行条件，满足后执行紧随其后的语句或代码块。
        if(id==ID_AGENT_CONFIRM){
            //【讲解 L744】调用 lab_agent_confirmation，根据所选方案构造确认体；同步读写控件状态。
            confirmation=lab_agent_confirmation(&agent,(int)SendMessageW(agentOptions,CB_GETCURSEL,0,0),j->keys[0]);
            //【讲解 L745】检查本行条件，满足后执行紧随其后的语句或代码块。
            if(!confirmation)goto abort;
            //【讲解 L746】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            j->note[0]=0;
        //【讲解 L747】结束当前代码块。
        }
        //【讲解 L748】调用 lab_agent_payload，构造与网页版一致的 V3 助手请求字段。
        j->agentRequest=lab_agent_payload(&client.session,j->note,j->date,j->start,j->end,j->devices,(size_t)j->count,j->keys[0],agent.session,agent.history,confirmation);
        //【讲解 L749】释放 JSON 对象及其子节点。
        cJSON_Delete(confirmation);if(!j->agentRequest)goto abort;
    //【讲解 L750】结束当前代码块。
    }
    //【讲解 L751】调用 start_job，保存任务快照、设置忙状态并启动后台线程。
    start_job(j);
    //【讲解 L752】结束当前函数并返回 ；具体成功或失败含义见函数说明。
    return;
//【讲解 L753】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
confirm:
    //【讲解 L754】开始当前作用域。
    {
        //【讲解 L755】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
        wchar_t *w = wide(confirmation);
        //【讲解 L756】声明或更新 window（主窗口）；按本行表达式保存对应状态。
        int answer = MessageBoxW(window, w, L"请确认", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        //【讲解 L757】释放堆内存。
        free(w);
        //【讲解 L758】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (answer == IDYES) {
            //【讲解 L759】调用 start_job，保存任务快照、设置忙状态并启动后台线程。
            start_job(j);
            //【讲解 L760】结束当前函数并返回 ；具体成功或失败含义见函数说明。
            return;
        //【讲解 L761】结束当前代码块。
        }
    //【讲解 L762】结束当前代码块。
    }
//【讲解 L763】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
abort:
    //【讲解 L764】清除敏感临时数据。
    SecureZeroMemory(j->password, sizeof(j->password));
    //【讲解 L765】释放 JSON 对象及其子节点。
    cJSON_Delete(j->row);cJSON_Delete(j->agentRequest);cJSON_Delete(j->agentResult);
    //【讲解 L766】释放堆内存。
    free(j);
//【讲解 L767】结束当前代码块。
}
//【讲解 L768】函数 place：调整 Windows 控件位置，并扣除当前滚动偏移。
static void place(HWND h,int x,int y,int w,int height){MoveWindow(h,x,y-scrollY,w,height,TRUE);}
//【讲解 L769】函数 layout：按窗口、页面和滚动状态排列控件；这是原生布局，与浏览器 CSS 实现不同。
static void layout(void) {
    //【讲解 L770】读取窗口可用绘制区域。
    RECT r;GetClientRect(window,&r);int w=r.right,x=UI_SIDEBAR+32,cw=w-x-32,rail=w-324,labw=cw-308;
    //【讲解 L771】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(labw<420)labw=420;
    //【讲解 L772】声明或更新 logged（登录标记）、page（当前页面编号）；按本行表达式保存对应状态。
    SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS,0,logged?(page==0?1160:page==1?1080:900):r.bottom,(UINT)r.bottom,scrollY,0};
    //【讲解 L773】声明或更新 window（主窗口）；按本行表达式保存对应状态。
    SetScrollInfo(window,SB_VERT,&si,TRUE);GetScrollInfo(window,SB_VERT,&si);scrollY=si.nPos;
    //【讲解 L774】调用 visibility，根据登录态、页面、角色及 busy 状态切换控件可见性和可操作性。
    visibility();
    //【讲解 L775】调整控件位置和尺寸。
    MoveWindow(whoText,20,r.bottom-170,188,64,TRUE);
    //【讲解 L776】调整控件位置和尺寸。
    const int order[]={0,4,1,2,3};for(int k=0;k<5;k++)MoveWindow(navigation[order[k]],16,152+k*50,196,44,TRUE);
    //【讲解 L777】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int fx=w/2+36,fw=w-fx-70; if(fw>430)fw=430;
    //【讲解 L778】按本行初值、条件和步长循环；调整控件位置和尺寸。
    for(int i=0;i<4;i++){MoveWindow(labels[i],fx,170+i*70,fw,22,TRUE);}
    //【讲解 L779】调整控件位置和尺寸。
    HWND inputs[]={identity,password,nameEdit,inviteEdit};for(int i=0;i<4;i++)MoveWindow(inputs[i],fx,195+i*70,fw,36,TRUE);
    //【讲解 L780】调整控件位置和尺寸。
    MoveWindow(loginButton,fx,488,fw,40,TRUE);MoveWindow(registerButton,fx,540,fw,40,TRUE);MoveWindow(loginInfo,fx,600,fw,64,TRUE);
    //【讲解 L781】调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    place(helpText,x,82,cw,28);
    //【讲解 L782】调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    place(dateEdit,x+labw-200,294,175,34);place(startEdit,rail+16,330,120,300);place(endEdit,rail+156,330,120,300);
    //【讲解 L783】按本行初值、条件和步长循环；调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    for(int i=0;i<40;i++){int cell=(labw-48-7*8-32)/8;place(deviceButtons[i],x+24+(i%8)*(cell+8)+(i%8>=4?32:0),390+(i/8)*80,cell,68);}
    //【讲解 L784】调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    place(list,x,page==0?880:page==1?800:160,page==0?labw:cw,page==1?220:page==0?220:480);
    //【讲解 L785】调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    place(scheduleOptions,rail+22,548,246,300);
    //【讲解 L786】调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    place(agentOptions,x,478,cw,300);
    //【讲解 L787】调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    place(usageList,x,438,cw,220);place(agentReply,x+24,170,cw-48,280);
    //【讲解 L788】调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    place(noteEdit,page==0?rail+22:x,page==0?866:page==1?1040:page==4?532:666,page==0?246:cw,36);
    //【讲解 L789】调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
    place(profileFault,x+470,384,150,38);
    //【讲解 L790】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int slot=0;
    //【讲解 L791】按本行初值、条件和步长循环。
    for(int i=0;i<19;i++){
      //【讲解 L792】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
      int id=actionIds[i];
      //【讲解 L793】检查本行条件，满足后执行紧随其后的语句或代码块；调整控件位置和尺寸。
      if(id==ID_LOGOUT||id==ID_UPDATE||id==ID_REFRESH){int sy=id==ID_REFRESH?r.bottom-104:id==ID_UPDATE?r.bottom-68:r.bottom-36;MoveWindow(actions[i],20,sy,188,30,TRUE);continue;}
      //【讲解 L794】检查本行条件，满足后执行紧随其后的语句或代码块；读取窗口状态或附加指针。
      if(!(GetWindowLongPtrW(actions[i],GWL_STYLE)&WS_VISIBLE))continue;
      //【讲解 L795】检查本行条件，满足后执行紧随其后的语句或代码块；调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
      if(page==0){int sy=id==ID_BOOK?438:id==ID_SUGGEST?490:id==ID_CANCEL?862:id==ID_USE_CANDIDATE?592:814;place(actions[i],rail+22,sy,246,40);}
      //【讲解 L796】处理另一分支；调用 place，调整 Windows 控件位置，并扣除当前滚动偏移。
      else{int sy=page==1?384:page==4?594:720;place(actions[i],x+slot*150,sy,140,38);++slot;}
    //【讲解 L797】结束当前代码块。
    }
    //【讲解 L798】调整控件位置和尺寸；请求重绘界面。
    MoveWindow(statusText,x,r.bottom-26,cw,22,TRUE);InvalidateRect(window,NULL,TRUE);
//【讲解 L799】结束当前代码块。
}
//【讲解 L800】函数 box：绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷。
static void box(HDC dc,int x,int y,int w,int h,COLORREF fill){HBRUSH b=CreateSolidBrush(fill);HPEN pen=CreatePen(PS_SOLID,1,BORDER);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,pen);RoundRect(dc,x,y-scrollY,x+w,y+h-scrollY,32,32);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(pen);}
//【讲解 L801】函数 text：用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
static void text(HDC dc,int x,int y,int w,int h,const char *value,HFONT f,COLORREF color){RECT r={x,y-scrollY,x+w,y+h-scrollY};wchar_t *s=wide(value);SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextW(dc,s,-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);free(s);}
//【讲解 L802】函数 paint_workspace：绘制工作区背景、侧栏、卡片及标题；具体设备按钮等仍是原生控件。
static void paint_workspace(HDC dc){
    //【讲解 L803】读取窗口可用绘制区域。
    RECT r;GetClientRect(window,&r);FillRect(dc,&r,background);int w=r.right,x=UI_SIDEBAR+32,cw=w-x-32,rail=w-324,labw=cw-308;
    //【讲解 L804】检查本行条件，满足后执行紧随其后的语句或代码块；调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
    if(!logged){box(dc,w/2,100,w/2-40,620,RGB(255,255,255));text(dc,64,180,w/2-100,40,"NJUPT · 实验室预约",bold,MUTED);text(dc,64,245,w/2-100,100,"把时间留给探索",heroFont,INK);text(dc,64,370,w/2-100,80,"选定设备，约好时间。\n设备、时段和你的记录，都在这里。",font,MUTED);text(dc,w/2+36,120,400,40,"欢迎回来",headingFont,INK);return;}
    //【讲解 L805】执行 FillRect、GetStockObject，参数沿用本行给出的对象、标志和容量。
    RECT side={0,0,UI_SIDEBAR,r.bottom};FillRect(dc,&side,(HBRUSH)GetStockObject(WHITE_BRUSH));
    /* Sidebar stays anchored while the workspace scrolls. */
    //【讲解 L807】调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
    int old=scrollY;scrollY=0;DrawIconEx(dc,20,26,LoadIconW(GetModuleHandleW(NULL),MAKEINTRESOURCEW(1)),28,28,0,NULL,DI_NORMAL);text(dc,58,30,160,40,"NJUPT 实验室预约",bold,INK);scrollY=old;
    //【讲解 L808】调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串；调用 ui_page_title，按页面编号返回统一中文标题。
    text(dc,x,22,cw,20,"NJUPT · 实验室预约",font,MUTED);text(dc,x,48,cw,40,ui_page_title(page),headingFont,INK);
    //【讲解 L809】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(page==0){
      //【讲解 L810】调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串；调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷。
      text(dc,x,208,cw,56,"把时间留给探索",heroFont,INK);box(dc,rail,112,292,130,RGB(252,252,251));text(dc,rail+22,132,248,28,"预约流程 · 三步完成",bold,INK);text(dc,rail+22,180,248,46,"01 选择设备   02 设置时间\n03 确认预约",font,MUTED);
      //【讲解 L811】调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
      box(dc,x,272,labw,530,RGB(255,255,255));text(dc,x+24,296,250,28,"实验室平面图",bold,INK);text(dc,x+24,335,labw-48,28,"* 空闲    已预约    使用中    ! 故障    已选择",font,MUTED);
      //【讲解 L812】调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
      box(dc,rail,272,292,286,RGB(255,255,255));text(dc,rail+22,294,248,28,"预约时间",bold,INK);
      //【讲解 L813】按容量格式化 UTF-8 文本；调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
      int n=0;for(int i=0;i<40;i++)n+=selected[i];char selection[96];snprintf(selection,sizeof(selection),n?"已选择 %d 台设备":"尚未选择设备",n);box(dc,rail+22,382,246,38,RGB(239,240,255));text(dc,rail+34,391,218,24,selection,font,INK);int faulty=0;const cJSON *dv;cJSON_ArrayForEach(dv,model.devices)if(strcmp(lab_str(dv,"status"),"available"))++faulty;char count[100];snprintf(count,sizeof(count),"%d / 40\n台设备可预约",40-faulty);box(dc,rail,584,292,190,RGB(255,255,255));text(dc,rail+22,604,246,24,"实验室状态",bold,INK);text(dc,rail+22,646,246,100,count,headingFont,ACCENT);text(dc,x,838,labw,30,"近期预约",bold,INK);
    //【讲解 L814】上一条件不成立时检查下一条件。
    } else if(page==1){
      //【讲解 L815】调用 date_string，将日期控件的年月日格式化为 YYYY-MM-DD；调用 ui_summary，根据当前用户非取消预约统计当日记录、常用设备及时段；按容量格式化 UTF-8 文本。
      char today[160],prefs[256],identityText[256],date[16];date_string(date);ui_summary(&model,&client.session,date,today,sizeof(today),prefs,sizeof(prefs));snprintf(identityText,sizeof(identityText),"%s  ·  %s  ·  %s",client.session.name,admin()?"管理员":"学生",client.session.login);
      //【讲解 L816】调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
      box(dc,x,124,cw,94,RGB(255,255,255));text(dc,x+24,148,cw-48,32,identityText,headingFont,INK);text(dc,x+24,188,cw-48,22,"个人资料 · 使用与网页相同的云端账号",font,MUTED);
      //【讲解 L817】调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
      box(dc,x,238,cw/2-8,104,RGB(255,255,255));text(dc,x+22,254,cw/2-40,25,"今天的预约",bold,INK);text(dc,x+22,290,cw/2-40,44,today,font,MUTED);
      //【讲解 L818】调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
      box(dc,x+cw/2+8,238,cw/2-8,104,RGB(255,255,255));text(dc,x+cw/2+30,254,cw/2-40,25,"预约偏好",bold,INK);text(dc,x+cw/2+30,290,cw/2-48,44,prefs,font,MUTED);
      //【讲解 L819】调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
      text(dc,x,354,cw,28,"手动签到与签退",bold,INK);text(dc,x,748,cw,30,"我的预约",bold,INK);text(dc,x,780,cw,20,"完整记录与实际使用时间（北京时间）",font,MUTED);
    //【讲解 L820】上一条件不成立时检查下一条件；调用 box，绘制圆角背景区域，结束前恢复并释放 GDI 画笔、画刷；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
    } else if(page==4){box(dc,x,124,cw,350,RGB(255,255,255));text(dc,x+24,142,cw-48,24,"智能助手",bold,INK);text(dc,x,464,cw,24,"输入设备、日期或时段需求，获取预约建议。",font,MUTED);}
    //【讲解 L821】处理另一分支；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
    else text(dc,x,122,cw,24,page==2?"故障说明和维修进度":"用户和账号管理",font,MUTED);
    //【讲解 L822】检查本行条件，满足后执行紧随其后的语句或代码块；调用 text，用指定字体和颜色绘制 UTF-8 文本，转换后释放临时宽字符串。
    if(preview)text(dc,x,1112,cw,24,"界面预览 · 示例数据 · 未连接真实账号",font,MUTED);
//【讲解 L823】结束当前代码块。
}
/* Capture only our fixture window; never reads another application's pixels. */
//【讲解 L825】函数 preview_child：将需要显示的子控件绘制到预览内存画布，用于离线截图检查。
static BOOL CALLBACK preview_child(HWND child,LPARAM value) {
 //【讲解 L826】检查本行条件，满足后执行紧随其后的语句或代码块；读取窗口状态或附加指针。
 if(GetParent(child)!=window || !(GetWindowLongPtrW(child,GWL_STYLE)&WS_VISIBLE))return TRUE;
 //【讲解 L827】声明或更新 window（主窗口）；按本行表达式保存对应状态。
 HDC dc=(HDC)value;RECT rect;GetWindowRect(child,&rect);POINT pos={rect.left,rect.top};ScreenToClient(window,&pos);
 //【讲解 L828】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
 int saved=SaveDC(dc);SetViewportOrgEx(dc,pos.x,pos.y,NULL);
 //【讲解 L829】同步读写控件状态。
 SendMessageW(child,WM_PRINT,(WPARAM)dc,PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);return TRUE;
//【讲解 L830】结束当前代码块。
}
//【讲解 L831】函数 preview_capture：导出示例页面截图；使用预览夹具，不会证明真实登录、预约成功。
static int preview_capture(HWND h) {
 //【讲解 L832】读取窗口可用绘制区域；选择 GDI 绘图对象。
 RECT r;GetClientRect(h,&r);int w=r.right,height=r.bottom;HDC dc=GetDC(h),mem=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,w,height);HGDIOBJ old=SelectObject(mem,bitmap);
 //【讲解 L833】调用 paint_workspace，绘制工作区背景、侧栏、卡片及标题；选择 GDI 绘图对象。
 paint_workspace(mem);EnumChildWindows(h,preview_child,(LPARAM)mem);SelectObject(mem,old);
 //【讲解 L834】执行 ，参数沿用本行给出的对象、标志和容量。
 BITMAPINFO info={0};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 //【讲解 L835】申请并清零堆内存。
 DWORD bytes=(DWORD)(w*height*4);void *pixels=calloc(1,bytes);int ok=pixels&&GetDIBits(dc,bitmap,0,height,pixels,&info,DIB_RGB_COLORS);FILE *f=NULL;
 //【讲解 L836】检查本行条件，满足后执行紧随其后的语句或代码块。
 if(ok)f=_wfopen(previewOutput,L"wb");
 //【讲解 L837】检查本行条件，满足后执行紧随其后的语句或代码块；按字节写入文件；关闭文件。
 if(f){BITMAPFILEHEADER file={0};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+bytes;ok=fwrite(&file,sizeof(file),1,f)==1&&fwrite(&info.bmiHeader,sizeof(BITMAPINFOHEADER),1,f)==1&&fwrite(pixels,bytes,1,f)==1;fclose(f);}else ok=0;
 //【讲解 L838】释放堆内存；释放 GDI 绘图资源。
 free(pixels);DeleteObject(bitmap);DeleteDC(mem);ReleaseDC(h,dc);return ok;
//【讲解 L839】结束当前代码块。
}
//【讲解 L840】函数 wndproc：Windows 主消息入口，处理创建、点击、绘制、尺寸、滚动、后台结果及退出清理。
static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    //【讲解 L841】根据消息或任务类型选择处理分支。
    switch (msg) {
    //【讲解 L842】处理 WM_CREATE 对应的事件或任务。
    case WM_CREATE: {
        //【讲解 L843】声明或更新 window（主窗口）；按本行表达式保存对应状态。
        window = h;
        //【讲解 L844】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        LOGFONTW lf = {0};
        //【讲解 L845】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        lf.lfHeight = -15;
        //【讲解 L846】执行 wcscpy，参数沿用本行给出的对象、标志和容量。
        wcscpy(lf.lfFaceName, L"Microsoft YaHei");
        //【讲解 L847】创建字体对象。
        font = CreateFontIndirectW(&lf);
        //【讲解 L848】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        lf.lfWeight = FW_BOLD;
        //【讲解 L849】创建字体对象。
        bold = CreateFontIndirectW(&lf);
        //【讲解 L850】创建纯色画刷。
        background = CreateSolidBrush(BG);
        //【讲解 L851】创建字体对象。
        lf.lfHeight=-28;headingFont=CreateFontIndirectW(&lf);lf.lfHeight=-34;heroFont=CreateFontIndirectW(&lf);
        //【讲解 L852】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        const wchar_t *navNames[]={L"预约",L"我的",L"故障记录",L"用户管理",L"智能助手"};
        //【讲解 L853】按本行初值、条件和步长循环；调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        for(int i=0;i<5;i++)navigation[i]=control(L"BUTTON",navNames[i],BS_OWNERDRAW|WS_TABSTOP,600+i,16,152+i*50,196,44);
        //【讲解 L854】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        profileFault=control(L"BUTTON",L"我的故障记录",BS_OWNERDRAW|WS_TABSTOP,605,260,340,150,38);
        //【讲解 L855】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        scheduleOptions=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,0,940,548,246,300);
        //【讲解 L856】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        agentOptions=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,0,260,478,800,300);
        //【讲解 L857】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        agentReply=control(L"EDIT",L"告诉我你的预约需求。",ES_MULTILINE|ES_READONLY|WS_VSCROLL,0,260,170,800,280);
        //【讲解 L858】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        whoText = control(L"STATIC", L"", 0, 0, 500, 20, 570, 26);
        //【讲解 L859】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        const wchar_t *texts[] = {L"学号 / 邮箱", L"密码", L"姓名（注册）", L"管理员邀请码（可选）"};
        //【讲解 L860】按本行初值、条件和步长循环。
        for (int i = 0; i < 4; i++)
            //【讲解 L861】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
            labels[i] = control(L"STATIC", texts[i], 0, 0, 155, 120 + i * 65, 280, 24);
        //【讲解 L862】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        identity = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 120, 370, 30);
        //【讲解 L863】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        password = control(L"EDIT", L"", WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 185,
                           //【讲解 L864】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                           370, 30);
        //【讲解 L865】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        nameEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 250, 370, 30);
        //【讲解 L866】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        inviteEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 315, 370, 30);
        //【讲解 L867】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        loginButton = control(L"BUTTON", L"登录并继续", BS_OWNERDRAW | WS_TABSTOP, ID_LOGIN, 450, 380, 175, 38);
        //【讲解 L868】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        registerButton = control(L"BUTTON", L"创建账号", BS_OWNERDRAW | WS_TABSTOP, ID_REGISTER, 645, 380, 175, 38);
        //【讲解 L869】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        loginInfo = control(L"STATIC",
                            //【讲解 L870】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                            L"使用与网页相同的云端账号。\n学生使用学号；管理员使用邮箱。",
                            //【讲解 L871】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                            0, 0, 155, 460, 800, 80);
        //【讲解 L872】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        tabs = control(WC_TABCONTROLW, L"", WS_TABSTOP, ID_TAB, 24, 54, 440, 32);
        //【讲解 L873】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        const wchar_t *tabNames[] = {L"设备预约", L"我的预约", L"故障管理", L"账号管理"};
        //【讲解 L874】按本行初值、条件和步长循环。
        for (int i = 0; i < 4; i++) {
            //【讲解 L875】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            TCITEMW t = {0};
            //【讲解 L876】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            t.mask = TCIF_TEXT;
            //【讲解 L877】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            t.pszText = (LPWSTR)tabNames[i];
            //【讲解 L878】同步读写控件状态。
            SendMessageW(tabs, TCM_INSERTITEMW, (WPARAM)i, (LPARAM)&t);
        //【讲解 L879】结束当前代码块。
        }
        //【讲解 L880】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        dateEdit =
            //【讲解 L881】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
            control(DATETIMEPICK_CLASSW, L"", DTS_SHORTDATEFORMAT | WS_TABSTOP, ID_DATE, 490, 55, 180, 30);
        //【讲解 L882】执行 DateTime_SetFormat，参数沿用本行给出的对象、标志和容量。
        DateTime_SetFormat(dateEdit, L"yyyy-MM-dd");
        //【讲解 L883】调用 today_beijing，以 UTC 加八小时确定北京当天，再设置日期控件。
        today_beijing();
        //【讲解 L884】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        startEdit = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ID_START, 685, 55,
                            //【讲解 L885】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                            130, 300);
        //【讲解 L886】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        endEdit =
            //【讲解 L887】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
            control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ID_END, 830, 55, 130, 300);
        //【讲解 L888】按本行初值、条件和步长循环。
        for (int i = 0; i < 9; i++) {
            //【讲解 L889】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
            wchar_t b[50];
            //【讲解 L890】格式化宽字符文本。
            swprintf(b, 50, L"开始 %02d:00", i + 8);
            //【讲解 L891】同步读写控件状态。
            SendMessageW(startEdit, CB_ADDSTRING, 0, (LPARAM)b);
            //【讲解 L892】格式化宽字符文本。
            swprintf(b, 50, L"结束 %02d:00", i + 9);
            //【讲解 L893】同步读写控件状态。
            SendMessageW(endEdit, CB_ADDSTRING, 0, (LPARAM)b);
        //【讲解 L894】结束当前代码块。
        }
        //【讲解 L895】同步读写控件状态。
        SendMessageW(startEdit, CB_SETCURSEL, 0, 0);
        //【讲解 L896】同步读写控件状态。
        SendMessageW(endEdit, CB_SETCURSEL, 0, 0);
        //【讲解 L897】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        helpText = control(L"STATIC", L"", 0, 0, 24, 91, 1040, 28);
        //【讲解 L898】按本行初值、条件和步长循环。
        for (int i = 0; i < 40; i++)
            //【讲解 L899】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
            deviceButtons[i] = control(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, ID_FIRST_DEVICE + i,
                                       //【讲解 L900】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                                       24 + (i % 10) * 104, 127 + (i / 10) * 44, 95, 35);
        //【讲解 L901】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        list = control(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | WS_BORDER | WS_TABSTOP, ID_LIST, 24,
                       //【讲解 L902】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                       315, 1040, 230);
        //【讲解 L903】执行 ListView_SetExtendedListViewStyle，参数沿用本行给出的对象、标志和容量。
        ListView_SetExtendedListViewStyle(list,
                                          //【讲解 L904】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                                          LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        //【讲解 L905】执行 ListView_SetBkColor、RGB、ListView_SetTextBkColor，参数沿用本行给出的对象、标志和容量。
        ListView_SetBkColor(list,RGB(255,255,255));ListView_SetTextBkColor(list,RGB(255,255,255));ListView_SetTextColor(list,INK);
        //【讲解 L906】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        usageList=control(WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|WS_BORDER|WS_TABSTOP,210,260,438,800,220);
        //【讲解 L907】执行 ListView_SetExtendedListViewStyle，参数沿用本行给出的对象、标志和容量。
        ListView_SetExtendedListViewStyle(usageList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
        //【讲解 L908】执行 ListView_SetBkColor、RGB、ListView_SetTextBkColor，参数沿用本行给出的对象、标志和容量。
        ListView_SetBkColor(usageList,RGB(255,255,255));ListView_SetTextBkColor(usageList,RGB(255,255,255));ListView_SetTextColor(usageList,INK);
        //【讲解 L909】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        noteEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 24, 560, 1040, 28);
        //【讲解 L910】同步读写控件状态。
        SendMessageW(noteEdit, EM_SETCUEBANNER, TRUE,
                     //【讲解 L911】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                     (LPARAM)L"故障 / 维修说明或智能助手问题；修改资料：姓名|登录名");
        //【讲解 L912】按本行初值、条件和步长循环。
        for (int i = 0; i < 19; i++)
            //【讲解 L913】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
            actions[i] = control(L"BUTTON", actionNames[i], BS_OWNERDRAW|WS_TABSTOP, actionIds[i], 24, 600, 135, 30);
        //【讲解 L914】调用 control，创建 Windows 子控件并设置共用字体，返回 HWND 句柄。
        statusText = control(L"STATIC", L"欢迎使用。所有预约时段按北京时间计算。", 0, 0, 24, 650, 1040, 24);
        //【讲解 L915】检查本行条件，满足后执行紧随其后的语句或代码块；更新控件文字；调用 ui_preview_model，生成离线截图用示例用户、设备和预约，不读取生产数据库；调用 render，按页面、账号、日期和设备状态更新控件内容。
        if(preview){SetWindowTextW(statusText,L"界面预览 · 示例数据 · 未连接真实账号");ui_preview_model(&client,&model);logged=preview!=1;page=preview==3?1:preview==4?4:0;render();}
        //【讲解 L916】处理另一分支。
        else SetTimer(h,1,30000,NULL);
        //【讲解 L917】调用 layout，按窗口、页面和滚动状态排列控件。
        layout();
        //【讲解 L918】申请并清零堆内存。
        Job *j = calloc(1, sizeof(*j));
        //【讲解 L919】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (j && !preview) {
            //【讲解 L920】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            j->kind = JOB_UPDATE;
            //【讲解 L921】调用 start_job，保存任务快照、设置忙状态并启动后台线程。
            start_job(j);
        //【讲解 L922】处理另一分支；释放堆内存。
        }else free(j);
        //【讲解 L923】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L924】结束当前代码块。
    }
    //【讲解 L925】处理 WM_PRINTCLIENT 对应的事件或任务。
    case WM_PRINTCLIENT:paint_workspace((HDC)wp);return 0;
    //【讲解 L926】处理 WM_PAINT 对应的事件或任务。
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);paint_workspace(dc);EndPaint(h,&ps);return 0;}
    //【讲解 L927】处理 WM_MOUSEWHEEL 对应的事件或任务。
    case WM_MOUSEWHEEL:scrollY-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*80;if(scrollY<0)scrollY=0;layout();return 0;
    //【讲解 L928】处理 WM_VSCROLL 对应的事件或任务。
    case WM_VSCROLL:{SCROLLINFO si={0};si.cbSize=sizeof(si);si.fMask=SIF_ALL;GetScrollInfo(h,SB_VERT,&si);int code=LOWORD(wp);if(code==SB_THUMBTRACK)scrollY=si.nTrackPos;else if(code==SB_LINEUP)scrollY-=40;else if(code==SB_LINEDOWN)scrollY+=40;else if(code==SB_PAGEUP)scrollY-=si.nPage;else if(code==SB_PAGEDOWN)scrollY+=si.nPage;if(scrollY<0)scrollY=0;layout();return 0;}
    //【讲解 L929】处理 WM_SIZE 对应的事件或任务。
    case WM_SIZE:
        //【讲解 L930】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (list)
            //【讲解 L931】调用 layout，按窗口、页面和滚动状态排列控件。
            layout();
        //【讲解 L932】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L933】处理 WM_GETMINMAXINFO 对应的事件或任务。
    case WM_GETMINMAXINFO:
        //【讲解 L934】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        ((MINMAXINFO *)lp)->ptMinTrackSize.x = 1200;
        //【讲解 L935】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        ((MINMAXINFO *)lp)->ptMinTrackSize.y = 800;
        //【讲解 L936】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L937】处理 WM_COMMAND 对应的事件或任务。
    case WM_COMMAND: {
        //【讲解 L938】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        int id = LOWORD(wp);
        //【讲解 L939】检查本行条件，满足后执行紧随其后的语句或代码块；调用 admin，根据云端加载的 session.role 判断是否显示管理入口；调用 render，按页面、账号、日期和设备状态更新控件内容；调用 layout，按窗口、页面和滚动状态排列控件。
        if(id>=600&&id<=605){if(busy)return 0;page=id==605?2:id-600;if(page==3&&!admin())page=1;scrollY=0;render();layout();return 0;}
        //【讲解 L940】检查本行条件，满足后执行紧随其后的语句或代码块。
        if(preview&&!(id>=ID_FIRST_DEVICE&&id<ID_FIRST_DEVICE+40))return 0;
        //【讲解 L941】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (id >= ID_FIRST_DEVICE && id < ID_FIRST_DEVICE + 40) {
            //【讲解 L942】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
            int i = id - ID_FIRST_DEVICE;
            //【讲解 L943】声明或更新 selected（设备选择状态）；按本行表达式保存对应状态。
            selected[i] = !selected[i];
            //【讲解 L944】请求重绘界面。
            InvalidateRect(deviceButtons[i], NULL, TRUE);
            //【讲解 L945】请求重绘界面。
            InvalidateRect(window,NULL,TRUE);
        //【讲解 L946】上一条件不成立时检查下一条件。
        } else if (id == ID_START || id == ID_END) {
            //【讲解 L947】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (HIWORD(wp) == CBN_SELCHANGE)
                //【讲解 L948】调用 render，按页面、账号、日期和设备状态更新控件内容。
                render();
        //【讲解 L949】上一条件不成立时检查下一条件。
        } else if (HIWORD(wp) == BN_CLICKED)
            //【讲解 L950】调用 command，把按钮编号映射成具体操作。
            command(id);
        //【讲解 L951】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L952】结束当前代码块。
    }
    //【讲解 L953】处理 WM_NOTIFY 对应的事件或任务。
    case WM_NOTIFY: {
        //【讲解 L954】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        NMHDR *n = (NMHDR *)lp;
        //【讲解 L955】检查本行条件，满足后执行紧随其后的语句或代码块。
        if(!rendering&&(n->idFrom==ID_LIST||n->idFrom==210)&&n->code==LVN_ITEMCHANGED){
            //【讲解 L956】按容量格式化 UTF-8 文本；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
            NMLISTVIEW *v=(NMLISTVIEW *)lp;if((v->uNewState&LVIS_SELECTED)&&v->iItem>=0&&v->iItem<listedCount){selectedTable=n->idFrom==210;snprintf(selectedId,sizeof(selectedId),"%s",lab_str(listed[v->iItem],"id"));}
        //【讲解 L957】结束当前代码块。
        }
        //【讲解 L958】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (n->idFrom == ID_TAB && n->code == TCN_SELCHANGE) {
            //【讲解 L959】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
            page = TabCtrl_GetCurSel(tabs);
            //【讲解 L960】检查本行条件，满足后执行紧随其后的语句或代码块；调用 admin，根据云端加载的 session.role 判断是否显示管理入口。
            if (page == 3 && !admin()) {
                //【讲解 L961】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
                page = 1;
                //【讲解 L962】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
                TabCtrl_SetCurSel(tabs, page);
                //【讲解 L963】调用 tell，显示平台消息提示。
                tell("账号管理需要管理员权限。", MB_ICONINFORMATION);
            //【讲解 L964】结束当前代码块。
            }
            //【讲解 L965】调用 render，按页面、账号、日期和设备状态更新控件内容。
            render();
            //【讲解 L966】调用 visibility，根据登录态、页面、角色及 busy 状态切换控件可见性和可操作性。
            visibility();
        //【讲解 L967】结束当前代码块。
        }
        //【讲解 L968】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (n->idFrom == ID_DATE && n->code == DTN_DATETIMECHANGE && logged)
            //【讲解 L969】调用 render，按页面、账号、日期和设备状态更新控件内容。
            render();
        //【讲解 L970】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L971】结束当前代码块。
    }
    //【讲解 L972】处理 RESULT_MESSAGE 对应的事件或任务。
    case RESULT_MESSAGE: {
        //【讲解 L973】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        Job *j = (Job *)lp;
        //【讲解 L974】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (j->kind == JOB_UPDATE) {
            //【讲解 L975】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            checkingUpdate = 0;
            //【讲解 L976】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (j->ok) {
                //【讲解 L977】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
                wchar_t *w = wide(j->message);
                //【讲解 L978】检查本行条件，满足后执行紧随其后的语句或代码块。
                if (MessageBoxW(h, w, L"桌面版本更新", MB_YESNO | MB_ICONINFORMATION) == IDYES)
                    //【讲解 L979】执行 ShellExecuteW，参数沿用本行给出的对象、标志和容量。
                    ShellExecuteW(h, L"open",
                                  //【讲解 L980】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                                  L"https://alexis-zhang0503.github.io/intelligent-lab-booking-system/"
                                  //【讲解 L981】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                                  L"downloads/LabReservation-Windows-x64.zip",
                                  //【讲解 L982】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                                  NULL, NULL, SW_SHOWNORMAL);
                //【讲解 L983】释放堆内存。
                free(w);
            //【讲解 L984】结束当前代码块。
            }
            //【讲解 L985】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (!busy)
                //【讲解 L986】更新控件文字。
                SetWindowTextW(statusText, L"原生 C 3.1.0 · 云端数据每 30 秒刷新");
        //【讲解 L987】处理另一分支。
        } else {
            //【讲解 L988】声明或更新 busy（请求忙标记）；按本行表达式保存对应状态。
            busy = 0;
            //【讲解 L989】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
            client = j->client;
            //【讲解 L990】检查本行条件，满足后执行紧随其后的语句或代码块；释放 JSON 对象及其子节点；深复制 JSON 节点，隔离所有权。
            if(j->kind==JOB_SUGGEST&&j->agentResult){cJSON_Delete(scheduleCandidates);scheduleCandidates=cJSON_Duplicate(j->agentResult,1);}
            //【讲解 L991】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_agent_accept，拒绝其他账号的迟到响应，记录对话并最多保留 20 条消息。
            if(j->kind==JOB_AGENT && j->agentResult && !strcmp(client.session.uid,agent.owner))lab_agent_accept(&agent,j->client.session.uid,j->note,j->agentResult);
            //【讲解 L992】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (j->loaded) {
                //【讲解 L993】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体。
                lab_model_free(&model);
                //【讲解 L994】声明或更新 model（当前数据模型）；按本行表达式保存对应状态。
                model = j->model;
                //【讲解 L995】初始化或清空指定内存。
                memset(&j->model, 0, sizeof(j->model));
                //【讲解 L996】声明或更新 logged（登录标记）；按本行表达式保存对应状态。
                logged = 1;
                //【讲解 L997】检查本行条件，满足后执行紧随其后的语句或代码块。
                if (j->kind == JOB_LOGIN || j->kind == JOB_REGISTER) {
                    //【讲解 L998】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
                    page = 0;
                    //【讲解 L999】执行 TabCtrl_SetCurSel，参数沿用本行给出的对象、标志和容量。
                    TabCtrl_SetCurSel(tabs, 0);
                    //【讲解 L1000】初始化或清空指定内存。
                    memset(selected, 0, sizeof(selected));
                //【讲解 L1001】结束当前代码块。
                }
                //【讲解 L1002】更新控件文字。
                SetWindowTextW(password, L"");
                //【讲解 L1003】更新控件文字。
                SetWindowTextW(inviteEdit, L"");
                //【讲解 L1004】调用 render，按页面、账号、日期和设备状态更新控件内容。
                render();
            //【讲解 L1005】结束当前代码块。
            }
            //【讲解 L1006】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (!*client.session.access) {
                //【讲解 L1007】声明或更新 logged（登录标记）；按本行表达式保存对应状态。
                logged = 0;
                //【讲解 L1008】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体；调用 lab_agent_clear，释放助手历史及最近响应并清空状态。
                lab_model_free(&model);lab_agent_clear(&agent);
            //【讲解 L1009】结束当前代码块。
            }
            //【讲解 L1010】检查本行条件，满足后执行紧随其后的语句或代码块；调用 lab_agent_transcript，把内存中的对话拼成有容量限制的中文显示文本；调用 set_utf8，转换后设置 Windows 控件文字，并释放临时宽字符串。
            if(*j->message&&j->kind==JOB_AGENT&&page==4&&j->agentResult){char transcript[16384];lab_agent_transcript(&agent,transcript,sizeof(transcript));set_utf8(agentReply,transcript);}
            //【讲解 L1011】上一条件不成立时检查下一条件；调用 native_report，显示可滚动的原生文字报表窗口。
            else if(j->kind==JOB_METRICS&&j->ok)native_report("实验室使用情况",j->message);
            //【讲解 L1012】上一条件不成立时检查下一条件。
            else if (*j->message)
                //【讲解 L1013】调用 tell，显示平台消息提示。
                tell(j->message, j->ok ? MB_ICONINFORMATION : MB_ICONERROR);
            //【讲解 L1014】上一条件不成立时检查下一条件。
            else if (j->ok && j->kind != JOB_REFRESH && j->kind != JOB_LOGIN)
                //【讲解 L1015】调用 tell，显示平台消息提示。
                tell("操作成功，已刷新云端数据。", MB_ICONINFORMATION);
            //【讲解 L1016】更新控件文字。
            SetWindowTextW(statusText, j->loaded ? L"已同步云端数据 · 预约时段为北京时间 · 每 30 秒自动刷新"
                                                 //【讲解 L1017】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                                                 : L"请求未完成，请查看提示并刷新核实。请勿反复提交。");
        //【讲解 L1018】结束当前代码块。
        }
        //【讲解 L1019】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体。
        lab_model_free(&j->model);
        //【讲解 L1020】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
        lab_session_clear(&j->client.session);
        //【讲解 L1021】释放 JSON 对象及其子节点。
        cJSON_Delete(j->row);cJSON_Delete(j->agentRequest);cJSON_Delete(j->agentResult);
        //【讲解 L1022】释放堆内存。
        free(j);
        //【讲解 L1023】调用 layout，按窗口、页面和滚动状态排列控件。
        layout();
        //【讲解 L1024】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L1025】结束当前代码块。
    }
    //【讲解 L1026】处理 WM_TIMER 对应的事件或任务。
    case WM_TIMER:
        //【讲解 L1027】检查本行条件，满足后执行紧随其后的语句或代码块；调用 preview_capture，导出示例页面截图。
        if(wp==2){KillTimer(h,2);if(!preview_capture(h))PostQuitMessage(2);else DestroyWindow(h);return 0;}
        //【讲解 L1028】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (logged && !busy)
            //【讲解 L1029】调用 command，把按钮编号映射成具体操作。
            command(ID_REFRESH);
        //【讲解 L1030】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L1031】处理 WM_DRAWITEM 对应的事件或任务。
    case WM_DRAWITEM: {
        //【讲解 L1032】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT *)lp;
        //【讲解 L1033】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        int i = (int)d->CtlID - ID_FIRST_DEVICE;
        //【讲解 L1034】检查本行条件，满足后执行紧随其后的语句或代码块。
        if(i<0||i>=40){
            //【讲解 L1035】声明或更新 page（当前页面编号）；按本行表达式保存对应状态。
            int id=(int)d->CtlID,nav=id>=600&&id<=604,primary=id==ID_BOOK||id==ID_LOGIN||id==ID_AGENT,active=nav&&page==id-600;
            //【讲解 L1036】执行 RGB，参数沿用本行给出的对象、标志和容量。
            COLORREF bg=primary?ACCENT:active?RGB(242,242,239):RGB(255,255,255);
            //【讲解 L1037】创建纯色画刷；选择 GDI 绘图对象。
            HBRUSH brush=CreateSolidBrush(bg);HPEN pen=CreatePen(PS_SOLID,1,nav?bg:BORDER);HGDIOBJ oldb=SelectObject(d->hDC,brush),oldp=SelectObject(d->hDC,pen);
            //【讲解 L1038】选择 GDI 绘图对象；释放 GDI 绘图资源。
            RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,16,16);SelectObject(d->hDC,oldb);SelectObject(d->hDC,oldp);DeleteObject(brush);DeleteObject(pen);
            //【讲解 L1039】设置文字背景模式；设置绘图文字颜色；选择 GDI 绘图对象。
            RECT rect=d->rcItem;if(nav)rect.left+=14;SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,primary?RGB(255,255,255):INK);SelectObject(d->hDC,active?bold:font);wchar_t label[128];GetWindowTextW(d->hwndItem,label,128);DrawTextW(d->hDC,label,-1,&rect,(nav?DT_LEFT:DT_CENTER)|DT_VCENTER|DT_SINGLELINE);if(active){RECT line={d->rcItem.left,d->rcItem.top+9,d->rcItem.left+3,d->rcItem.bottom-9};HBRUSH a=CreateSolidBrush(ACCENT);FillRect(d->hDC,&line,a);DeleteObject(a);}if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&rect);return TRUE;
        //【讲解 L1040】结束当前代码块。
        }
        //【讲解 L1041】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        int occupied = 0, fault = 0, live = 0;
        //【讲解 L1042】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        const cJSON *r;
        //【讲解 L1043】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        char date[16];
        //【讲解 L1044】调用 date_string，将日期控件的年月日格式化为 YYYY-MM-DD。
        date_string(date);
        //【讲解 L1045】同步读写控件状态。
        int start = 8 + (int)SendMessageW(startEdit, CB_GETCURSEL, 0, 0),
            //【讲解 L1046】同步读写控件状态。
            end = 9 + (int)SendMessageW(endEdit, CB_GETCURSEL, 0, 0);
        //【讲解 L1047】遍历 JSON 数组；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
        cJSON_ArrayForEach(r, model.devices) if (lab_int(r, "number") == i + 1 &&
                                                 //【讲解 L1048】比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
                                                 strcmp(lab_str(r, "status"), "available")) fault = 1;
        //【讲解 L1049】遍历 JSON 数组；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
        cJSON_ArrayForEach(r, model.bookings) if (lab_int(r, "device_number") == i + 1 &&
                                                  //【讲解 L1050】比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
                                                  !strcmp(lab_str(r, "reservation_date"), date) &&
                                                  //【讲解 L1051】调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
                                                  lab_int(r, "start_hour") >= start &&
                                                  //【讲解 L1052】调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
                                                  lab_int(r, "start_hour") < end &&
                                                  //【讲解 L1053】比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
                                                  (!strcmp(lab_str(r, "status"), "reserved") ||
                                                   //【讲解 L1054】比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
                                                   !strcmp(lab_str(r, "status"), "confirmed") ||
                                                   //【讲解 L1055】比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
                                                   !strcmp(lab_str(r, "status"), "in_use"))) occupied = 1;
        //【讲解 L1056】遍历 JSON 数组；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围；调用 lab_booking_is_live，只有状态为 in_use 且当前时间在该小时内才显示正在使用，避免历史状态误亮。
        cJSON_ArrayForEach(r,model.bookings)if(lab_int(r,"device_number")==i+1&&lab_booking_is_live(r,now_ms()))live=1;
        //【讲解 L1057】声明或更新 selected（设备选择状态）；按本行表达式保存对应状态。
        COLORREF color = selected[i] ? RGB(239,240,255)
                         //【讲解 L1058】执行 RGB，参数沿用本行给出的对象、标志和容量。
                         : fault     ? RGB(252,240,239)
                         //【讲解 L1059】执行 RGB，参数沿用本行给出的对象、标志和容量。
                         : live      ? RGB(232,246,250)
                         //【讲解 L1060】执行 RGB，参数沿用本行给出的对象、标志和容量。
                         : occupied  ? RGB(242,242,239)
                                     //【讲解 L1061】执行 RGB，参数沿用本行给出的对象、标志和容量。
                                     : RGB(252,252,251);
        //【讲解 L1062】创建纯色画刷。
        HBRUSH brush = CreateSolidBrush(color);
        //【讲解 L1063】声明或更新 selected（设备选择状态）；按本行表达式保存对应状态。
        HPEN pen=CreatePen(PS_SOLID,selected[i]?2:1,selected[i]?ACCENT:fault?RGB(233,194,191):BORDER);
        //【讲解 L1064】选择 GDI 绘图对象；释放 GDI 绘图资源。
        HGDIOBJ ob=SelectObject(d->hDC,brush),op=SelectObject(d->hDC,pen);RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,16,16);SelectObject(d->hDC,ob);SelectObject(d->hDC,op);DeleteObject(pen);
        //【讲解 L1065】释放 GDI 绘图资源。
        DeleteObject(brush);
        //【讲解 L1066】设置文字背景模式。
        SetBkMode(d->hDC, TRANSPARENT);
        //【讲解 L1067】设置绘图文字颜色。
        SetTextColor(d->hDC, fault ? RGB(189,82,82) : INK);
        //【讲解 L1068】选择 GDI 绘图对象。
        SelectObject(d->hDC, bold);
        //【讲解 L1069】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
        wchar_t b[40];
        //【讲解 L1070】格式化宽字符文本。
        swprintf(b, 40, L"%02d\n%ls", i + 1, fault ? L"! 故障" : live ? L"使用中" : occupied ? L"已预约" : L"* 空闲");
        //【讲解 L1071】执行 DrawTextW，参数沿用本行给出的对象、标志和容量。
        RECT seatText=d->rcItem;seatText.top+=10;DrawTextW(d->hDC, b, -1, &seatText, DT_CENTER | DT_WORDBREAK);
        //【讲解 L1072】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (d->itemState & ODS_FOCUS)
            //【讲解 L1073】执行 DrawFocusRect，参数沿用本行给出的对象、标志和容量。
            DrawFocusRect(d->hDC, &d->rcItem);
        //【讲解 L1074】结束当前函数并返回 TRUE；具体成功或失败含义见函数说明。
        return TRUE;
    //【讲解 L1075】结束当前代码块。
    }
    //【讲解 L1076】处理 WM_CTLCOLOREDIT 对应的事件或任务。
    case WM_CTLCOLOREDIT:
        //【讲解 L1077】设置绘图文字颜色。
        SetBkColor((HDC)wp,RGB(255,255,255));SetTextColor((HDC)wp,INK);return (LRESULT)GetStockObject(WHITE_BRUSH);
    //【讲解 L1078】处理 WM_CTLCOLORSTATIC 对应的事件或任务。
    case WM_CTLCOLORSTATIC:
        //【讲解 L1079】设置文字背景模式。
        SetBkMode((HDC)wp,TRANSPARENT);
        //【讲解 L1080】执行 SetBkColor，参数沿用本行给出的对象、标志和容量。
        SetBkColor((HDC)wp, BG);
        //【讲解 L1081】设置绘图文字颜色。
        SetTextColor((HDC)wp, INK);
        //【讲解 L1082】结束当前函数并返回 (LRESULT)((HWND)lp==helpText||(HWND)lp==statusText?background:GetStockObject(WHITE_BRUSH))；具体成功或失败含义见函数说明。
        return (LRESULT)((HWND)lp==helpText||(HWND)lp==statusText?background:GetStockObject(WHITE_BRUSH));
    //【讲解 L1083】处理 WM_ERASEBKGND 对应的事件或任务。
    case WM_ERASEBKGND: {
        //【讲解 L1084】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
        RECT r;
        //【讲解 L1085】读取窗口可用绘制区域。
        GetClientRect(h, &r);
        //【讲解 L1086】声明或更新 background（背景画刷）；按本行表达式保存对应状态。
        FillRect((HDC)wp, &r, background);
        //【讲解 L1087】结束当前函数并返回 1；具体成功或失败含义见函数说明。
        return 1;
    //【讲解 L1088】结束当前代码块。
    }
    //【讲解 L1089】处理 WM_DESTROY 对应的事件或任务。
    case WM_DESTROY:
        //【讲解 L1090】执行 KillTimer，参数沿用本行给出的对象、标志和容量。
        KillTimer(h, 1);
        //【讲解 L1091】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体。
        lab_model_free(&model);
        //【讲解 L1092】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
        lab_session_clear(&client.session);
        //【讲解 L1093】执行 PostQuitMessage，参数沿用本行给出的对象、标志和容量。
        PostQuitMessage(0);
        //【讲解 L1094】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L1095】结束当前代码块。
    }
    //【讲解 L1096】交给 Windows 默认消息处理。
    return DefWindowProcW(h, msg, wp, lp);
//【讲解 L1097】结束当前代码块。
}
//【讲解 L1098】函数 wWinMain：Windows 程序入口：初始化公共控件及主窗口，处理预览参数，进入消息循环。
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR commandLine, int show) {
    //【讲解 L1099】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    (void)previous;
    //【讲解 L1100】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(wcsstr(commandLine,L"--preview=login"))preview=1;
    //【讲解 L1101】上一条件不成立时检查下一条件。
    else if(wcsstr(commandLine,L"--preview=booking"))preview=2;
    //【讲解 L1102】上一条件不成立时检查下一条件。
    else if(wcsstr(commandLine,L"--preview=profile"))preview=3;
    //【讲解 L1103】上一条件不成立时检查下一条件。
    else if(wcsstr(commandLine,L"--preview=agent"))preview=4;
    //【讲解 L1104】声明或更新 preview（离线预览标记）；按本行表达式保存对应状态。
    LPWSTR output=wcsstr(commandLine,L"--preview-output=");if(preview&&output){output+=17;wcsncpy(previewOutput,output,MAX_PATH-1);}
    //【讲解 L1105】执行 ，参数沿用本行给出的对象、标志和容量。
    INITCOMMONCONTROLSEX ic = {sizeof(ic), ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_TAB_CLASSES};
    //【讲解 L1106】执行 InitCommonControlsEx，参数沿用本行给出的对象、标志和容量。
    InitCommonControlsEx(&ic);
    //【讲解 L1107】执行 CoInitializeEx，参数沿用本行给出的对象、标志和容量。
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    //【讲解 L1108】声明或更新 transport（HTTP 传输回调）；按本行表达式保存对应状态。
    client.transport = lab_win_http;
    //【讲解 L1109】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    WNDCLASSEXW wc = {0};
    //【讲解 L1110】执行 ，参数沿用本行给出的对象、标志和容量。
    wc.cbSize = sizeof(wc);
    //【讲解 L1111】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    wc.lpfnWndProc = wndproc;
    //【讲解 L1112】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    wc.hInstance = instance;
    //【讲解 L1113】执行 LoadIconW、MAKEINTRESOURCEW，参数沿用本行给出的对象、标志和容量。
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    //【讲解 L1114】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    wc.hIconSm = wc.hIcon;
    //【讲解 L1115】执行 LoadCursorW，参数沿用本行给出的对象、标志和容量。
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    //【讲解 L1116】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    wc.lpszClassName = L"NJUPTNativeCLabReservation";
    //【讲解 L1117】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!RegisterClassExW(&wc))
        //【讲解 L1118】结束当前函数并返回 1；具体成功或失败含义见函数说明。
        return 1;
    //【讲解 L1119】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    HWND h = CreateWindowExW(0, wc.lpszClassName, L"实验室预约 · 原生 C", WS_OVERLAPPEDWINDOW|WS_VSCROLL, CW_USEDEFAULT,
                             //【讲解 L1120】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                             CW_USEDEFAULT, 1280, 940, NULL, NULL, instance, NULL);
    //【讲解 L1121】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!h)
        //【讲解 L1122】结束当前函数并返回 1；具体成功或失败含义见函数说明。
        return 1;
    //【讲解 L1123】设置窗口或控件显示状态。
    ShowWindow(h, show);
    //【讲解 L1124】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(preview&&*previewOutput)SetTimer(h,2,2500,NULL);
    //【讲解 L1125】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    MSG msg;
    //【讲解 L1126】条件成立时继续循环；等待下一个窗口消息。
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        //【讲解 L1127】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!IsDialogMessageW(h, &msg)) {
            //【讲解 L1128】转换键盘消息。
            TranslateMessage(&msg);
            //【讲解 L1129】派发消息给窗口过程。
            DispatchMessageW(&msg);
        //【讲解 L1130】结束当前代码块。
        }
    //【讲解 L1131】结束当前代码块。
    }
    //【讲解 L1132】释放 GDI 绘图资源。
    DeleteObject(font);
    //【讲解 L1133】释放 GDI 绘图资源。
    DeleteObject(bold);
    //【讲解 L1134】释放 GDI 绘图资源。
    DeleteObject(headingFont);DeleteObject(heroFont);
    //【讲解 L1135】释放 GDI 绘图资源。
    DeleteObject(background);
    //【讲解 L1136】执行 CoUninitialize，参数沿用本行给出的对象、标志和容量。
    CoUninitialize();
    //【讲解 L1137】结束当前函数并返回 (int)msg.wParam；具体成功或失败含义见函数说明。
    return (int)msg.wParam;
//【讲解 L1138】结束当前代码块。
}

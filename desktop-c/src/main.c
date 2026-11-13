/*
 * main.c —— Windows 桌面版的界面层：窗口、按钮、列表、弹窗都在这里。
 *
 * 用户看到的一切（登录页、预约页的 40 个设备按钮、"我的"页、故障记录、用户管理、智能助手）
 * 都由本文件创建和绘制；真正的业务（登录、预约、签到……）交给 client.c，联网交给 win_http.c。
 *
 * 整体运行流程（答辩必讲）：
 *   1. wWinMain 创建主窗口，进入"消息循环"：不停地 GetMessage → DispatchMessage。
 *   2. 用户点击按钮 → Windows 发来 WM_COMMAND 消息 → wndproc → command(按钮编号)。
 *   3. command 检查输入、弹确认框，把这次操作需要的数据装进一个 Job 结构体（"任务单"）。
 *   4. start_job 把登录状态复制进 Job，置 busy=1 让按钮变灰，用 CreateThread 启动后台线程 worker。
 *   5. worker 在后台调用 client.c 联网（可能要几秒，界面不会卡住），然后重新加载全部数据。
 *   6. worker 用 PostMessageW(RESULT_MESSAGE) 把 Job 指针"寄"回界面线程。
 *   7. wndproc 收到 RESULT_MESSAGE：换上新数据、弹出结果提示、重画界面、释放 Job。
 *   后台线程从不直接改全局变量和控件，数据都通过 Job 传递，所以两个线程之间不需要加锁。
 */

/* ===================== 模块 1：编译配置与头文件 =====================
 * 功能：让程序使用宽字符（能显示中文）版本的 Windows 函数，指定目标系统为 Windows 10，并引入要用的库。
 */
// 定义 UNICODE 后，SetWindowText 等宏会自动选用带 W 的宽字符版本，界面才能正常显示中文
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
// 0x0A00 表示 Windows 10，才能使用较新的控件功能
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
// windows.h：窗口、消息、线程等 Win32 基础函数；client.h / win_http.h：本项目的业务层与网络层
// commctrl.h：列表(ListView)、日期选择框等"通用控件"；commdlg.h："另存为"对话框
// objbase.h：生成 GUID（防重复编号）；shellapi.h：用浏览器打开下载页
// windowsx.h：Win32 辅助宏；ui_theme.h：状态翻译、时间格式化等界面小工具
#include <windows.h>
#include "client.h"
#include "win_http.h"
#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windowsx.h>
#include "ui_theme.h"

/* ===================== 模块 2：颜色与编号 =====================
 * 功能：给界面颜色、每个按钮、每种后台任务起名字。
 * 场景：Windows 通知"某个按钮被点了"时只给一个整数编号，enum 让这些编号有可读的名字，
 *       command 和 wndproc 里就能写 case ID_BOOK 而不是 case 104。
 */
// 自定义消息编号：后台线程完成后用它通知主窗口。WM_APP 以上的编号留给程序自用，不会和系统消息撞车；
// 外层括号防止宏展开后和别的运算符结合出错
#define RESULT_MESSAGE (WM_APP + 1)
// 界面配色（与网页版一致）：BG 背景灰、ACCENT 主题紫、BORDER 边框、MUTED 次要文字、INK 正文
#define BG RGB(247,247,245)
#define ACCENT RGB(92,99,214)
#define BORDER RGB(232,232,227)
#define MUTED RGB(104,107,115)
#define INK RGB(35,37,43)
// 按钮编号：从 100 开始依次加 1，每个按钮对应一种操作。WM_COMMAND 消息里 LOWORD(wp) 就是这个编号
enum {
    ID_LOGIN = 100,
    ID_REGISTER,
    ID_REFRESH,
    ID_LOGOUT,
    ID_BOOK,
    ID_ATTEND,
    ID_CANCEL,
    ID_REPORT,
    ID_REPAIR,
    ID_ARCHIVE,
    ID_DEACTIVATE,
    ID_INVITE,
    ID_METRICS,
    ID_UPDATE,
    ID_PROFILE,
    ID_SUGGEST,
    ID_AGENT,
    ID_AGENT_CONFIRM,
    ID_EXPORT_BOOKINGS,
    ID_EXPORT_ISSUES,
    ID_USE_CANDIDATE,
    // 200 段：标签页、记录列表、日期框、开始/结束小时下拉框（它们不是普通按钮，产生的通知不同）
    ID_TAB = 200,
    ID_LIST,
    ID_DATE,
    ID_START,
    ID_END,
    // 300~339：40 台设备按钮，第 n 号设备的编号是 ID_FIRST_DEVICE + n - 1（侧栏导航 600~605 直接写在 WM_CREATE 里）
    ID_FIRST_DEVICE = 300
};
// 后台任务种类：告诉 worker 线程这次要做什么。和按钮编号分开，是因为多个按钮可以对应同一种任务
// （例如"发送"和"选择此方案"都是 JOB_AGENT）
enum {
    JOB_LOGIN,
    JOB_REGISTER,
    JOB_REFRESH,
    JOB_BOOK,
    JOB_ATTEND,
    JOB_CANCEL,
    JOB_REPORT,
    JOB_REPAIR,
    JOB_ARCHIVE,
    JOB_DEACTIVATE,
    JOB_INVITE,
    JOB_METRICS,
    JOB_UPDATE,
    JOB_PROFILE,
    JOB_SUGGEST,
    JOB_AGENT
};

/* ===================== 模块 3：后台任务单 Job 与全局状态 =====================
 * 功能：Job 是界面线程交给后台线程的"任务单"：这次做什么、用哪些输入、结果写在哪。
 * 场景：每次点击需要联网的按钮都会 calloc 一个新 Job，任务完成、结果显示后再 free 掉。
 * 要点：Job 里放的都是数据的副本，后台线程只读写自己的 Job，不碰全局变量，因此不用加锁。
 */
typedef struct {
    int kind,         // 任务种类 JOB_xxx
        action,       // 签到时：1 签到 / 2 签退；助手请求时：1 表示管理员
        start, end,   // 预约开始小时、结束小时（9 和 11 表示 9:00–11:00）
        devices[40],  // 用户选中的设备编号
        count,        // 选中了几台设备
        metricDevice, // 统计报表要看哪台设备，0 表示全部
        inviteDays;   // 管理员邀请码有效天数
    LabClient client; // 登录状态的副本：结构体整体复制，后台线程用自己这份，不受界面线程影响
    LabModel model;   // 后台重新加载的最新数据，完成后交给界面线程替换全局 model
    cJSON *row,           // 列表里选中那条记录的深拷贝（防止刷新后原记录被释放）
          *agentRequest,  // 发给 AI 助手的请求
          *agentResult;   // AI 助手或"智能推荐"的回复
    char identity[256], password[256], name[256], invite[256], // 登录/注册框里填的学号、密码、姓名、邀请码
         date[16], endDate[16], // 日期 "YYYY-MM-DD"（统计报表需要起止两个日期）
         note[2048];            // 说明框文字：故障说明、给助手的问题、"姓名|学号"
    char keys[9][64],     // 每小时一个防重复编号（GUID），8~16 点共 9 个
         message[16384];  // 后台写好的结果提示，界面线程拿来弹窗
    int ok,     // 操作是否成功
        loaded; // 数据是否重新加载成功
    HWND target; // 完成后把结果寄给哪个窗口
} Job;
// —— 以下是全局状态，只有界面线程读写 ——
// HWND 是控件句柄：每个窗口、按钮、输入框创建后得到一个句柄，之后靠它读写内容、显示或隐藏。
// 登录页：学号/邮箱框、密码框、姓名框、邀请码框、登录按钮、注册按钮、说明文字
static HWND window, identity, password, nameEdit, inviteEdit, loginButton, registerButton, loginInfo;
// 工作区：标签页（已隐藏不用）、记录列表、日期框、开始/结束小时、说明框、底部状态栏、帮助文字、左下角"我是谁"、40 个设备按钮
static HWND tabs, list, dateEdit, startEdit, endEdit, noteEdit, statusText, helpText, whoText,
    deviceButtons[40];
// actions：19 个操作按钮（顺序与下面的 actionIds 一致）；labels：登录页 4 个输入框的标题
static HWND actions[19], labels[4];
// 四种字体：正文、粗体、页面标题、大标语
static HFONT font, bold, headingFont, heroFont;
// 两个下拉框：智能助手给出的方案、"智能推荐"给出的候选时段
static HWND agentOptions,scheduleOptions;
// 侧栏 5 个导航按钮、"我的故障记录"按钮、"我的"页的使用记录表、助手对话显示框
static HWND navigation[5],profileFault,usageList,agentReply;
// scrollY：页面向下滚了多少像素；preview：预览模式编号（0 为正常运行）；
// rendering：正在重填列表时为 1，用来忽略这期间的"选中变化"通知；selectedTable：用户选中的是哪张表
static int scrollY,preview,rendering,selectedTable;
// 当前选中记录的 id：列表重填后据此恢复选中
static char selectedId[80];
// 预览模式的截图保存路径
static wchar_t previewOutput[MAX_PATH];
// 提前声明 layout：command 里要用它，而它的定义在后面
static void layout(void);
// 背景色画刷，创建一次反复使用
static HBRUSH background;
// 当前登录的客户端（身份 + 联网函数）
static LabClient client;
// 当前显示的全部云端数据
static LabModel model;
// 智能助手的对话状态
static LabAgentState agent;
// "智能推荐"返回的候选方案
static cJSON *scheduleCandidates;
// busy：有任务在后台执行时为 1，按钮全部变灰，防止重复提交；checkingUpdate：正在检查新版本；logged：是否已登录；
// page：当前页面 0 预约 / 1 我的 / 2 故障 / 3 用户管理 / 4 助手；selected[i]：第 i+1 号设备是否被选中
static int busy, checkingUpdate, logged, page, selected[40];
// 列表第 i 行对应模型里的哪条记录（借用指针）。指针数组约 800KB，放在静态区不占栈；上限与 client.c 的 10 万条一致
static cJSON *listed[100000];
// 列表当前有几行
static int listedCount;
// 19 个操作按钮的编号和文字：两张表按下标一一对应，创建按钮时一起使用
static const int actionIds[] = {ID_REFRESH, ID_LOGOUT, ID_BOOK,    ID_ATTEND,     ID_CANCEL,
                                ID_REPORT,  ID_REPAIR, ID_ARCHIVE, ID_DEACTIVATE, ID_INVITE,
                                ID_METRICS, ID_UPDATE, ID_PROFILE, ID_SUGGEST,    ID_AGENT, ID_AGENT_CONFIRM, ID_EXPORT_BOOKINGS, ID_EXPORT_ISSUES, ID_USE_CANDIDATE};
static const wchar_t *actionNames[] = {L"刷新",         L"退出账号",     L"确认预约 →", L"签到 / 签退",
                                       L"取消预约",     L"报告设备故障",     L"完成维修",     L"归档故障",
                                       L"停用学生",     L"生成邀请码",   L"统计报表",     L"检查版本",
                                       L"修改本人资料", L"智能推荐", L"发送", L"选择此方案", L"导出完整预约", L"导出完整故障", L"采用推荐方案"};

/* ===================== 模块 4：文字编码与常用小工具 =====================
 * 功能：解决"业务层用 UTF-8、Windows 控件用 UTF-16"的转换，以及取时间、弹提示、创建控件、写列表格子等
 *       反复要做的小事。
 */
// UTF-8 → UTF-16：业务层和云端都用 UTF-8，Windows 控件要宽字符。返回的内存由调用者 free
static wchar_t *wide(const char *s) {
    // 第一次调用只问"需要多少个宽字符"（目标缓冲区传 NULL）
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    // 按需分配并清零；n 为 0 时也至少分配 1 个，保证返回空串而不是 NULL
    wchar_t *p = calloc(n ? n : 1, sizeof(wchar_t));
    // 第二次调用才真正转换
    if (p && n)
        MultiByteToWideChar(CP_UTF8, 0, s, -1, p, n);
    return p;
}
// 把 UTF-8 文字显示到控件上：转换 → 设置 → 释放临时内存
static void set_utf8(HWND h, const char *s) {
    wchar_t *p = wide(s);
    if (p) {
        SetWindowTextW(h, p);
        free(p);
    }
}
// 读出控件里的文字并转成 UTF-8，最多写 n 字节；转换失败（如超长）就给空串
static void get_utf8(HWND h, char *s, int n) {
    wchar_t b[2048];
    GetWindowTextW(h, b, 2048);
    if (!WideCharToMultiByte(CP_UTF8, 0, b, -1, s, n, NULL, NULL))
        s[0] = 0;
}
// 弹出提示框，成功和失败消息都靠它
static void tell(const char *s, UINT flags) {
    wchar_t *p = wide(s);
    if (p) {
        MessageBoxW(window, p, L"实验室预约 · 原生 C", flags);
        free(p);
    }
}
// 当前账号是不是管理员：只决定显示哪些按钮，真正的权限由云端检查
static int admin(void) { return !strcmp(client.session.role, "admin"); }
// 取当前时间（Unix 毫秒）。Windows 的 FILETIME 从 1601 年起每 100 纳秒计 1
static int64_t now_ms(void) {
    FILETIME f;
    ULARGE_INTEGER u;
    GetSystemTimeAsFileTime(&f);
    // FILETIME 是两个 32 位数，拼成一个 64 位整数
    u.LowPart = f.dwLowDateTime;
    u.HighPart = f.dwHighDateTime;
    // ÷10000 变成毫秒，再减去 1601 年到 1970 年之间的毫秒数，得到 Unix 时间
    return (int64_t)(u.QuadPart / 10000 - 11644473600000ULL);
}
// 把日期框里选的日期写成 "2026-11-05" 格式，之后比较和发请求都用这个格式
static void date_string(char out[16]) {
    SYSTEMTIME t;
    DateTime_GetSystemtime(dateEdit, &t);
    snprintf(out, 16, "%04u-%02u-%02u", t.wYear, t.wMonth, t.wDay);
}
// 启动时把日期框设成北京时间的今天：取 UTC 时间加 8 小时，不依赖电脑本地时区设置
static void today_beijing(void) {
    FILETIME f;
    ULARGE_INTEGER u;
    SYSTEMTIME t;
    GetSystemTimeAsFileTime(&f);
    u.LowPart = f.dwLowDateTime;
    u.HighPart = f.dwHighDateTime;
    // 8 小时 = 8×3600 秒 × 10^7（FILETIME 的单位是 100 纳秒）
    u.QuadPart += 8ULL * 3600 * 10000000;
    f.dwLowDateTime = u.LowPart;
    f.dwHighDateTime = u.HighPart;
    FileTimeToSystemTime(&f, &t);
    DateTime_SetSystemtime(dateEdit, GDT_VALID, &t);
}
// 创建一个子控件（按钮、输入框、列表……）并设好字体。id 就是以后 WM_COMMAND 里收到的编号
static HWND control(const wchar_t *cls, const wchar_t *text, DWORD style, int id, int x, int y, int w,
                    int h) {
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, window,
                             (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
    return c;
}
// 往列表第 row 行第 col 列写一个格子：第 0 列要"插入新行"，其他列是"修改已有行"
static void add_cell(int row, int col, const char *s) {
    wchar_t *w = wide(s);
    LVITEMW item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = col;
    item.pszText = w;
    if (col == 0)
        SendMessageW(list, LVM_INSERTITEMW, 0, (LPARAM)&item);
    else
        SendMessageW(list, LVM_SETITEMTEXTW, (WPARAM)row, (LPARAM)&item);
    // "我的"页有第二张使用记录表，内容同步写一份
    if (page == 1 && usageList) {
        if(col==0)SendMessageW(usageList,LVM_INSERTITEMW,0,(LPARAM)&item);
        else SendMessageW(usageList,LVM_SETITEMTEXTW,(WPARAM)row,(LPARAM)&item);
    }
    free(w);
}
// 给列表加一列表头（标题和宽度），"我的"页的第二张表同步加
static void column(int i, const wchar_t *title, int width) {
    LVCOLUMNW c = {0};
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = (LPWSTR)title;
    c.cx = width;
    SendMessageW(list, LVM_INSERTCOLUMNW, (WPARAM)i, (LPARAM)&c);
    if(page==1&&usageList)SendMessageW(usageList,LVM_INSERTCOLUMNW,(WPARAM)i,(LPARAM)&c);
}

/* ===================== 模块 5：刷新界面内容 render =====================
 * 功能：根据当前页面，把内存里的数据（model）填进列表和下拉框，并更新左下角"我是谁"。
 * 场景：登录成功、切换页面、改日期或时段、后台刷新完成后都会调用。它只读本地数据，不联网。
 */
static void render(void) {
    const cJSON *r;
    char b[1024], d[16];
    int i = 0;
    // 步骤 1：取日期框当前日期，预约页只显示这一天的记录
    date_string(d);
    // 步骤 2：智能助手——未登录就清空对话；已登录就绑定到当前账号（换了账号会自动清掉旧对话）
    if(!logged)lab_agent_clear(&agent);else lab_agent_bind(&agent,client.session.uid);
    char transcript[16384];                                     // 对话文本缓冲区
    lab_agent_transcript(&agent,transcript,sizeof(transcript)); // 把历史拼成"我：… 助手：…"
    set_utf8(agentReply,transcript);                            // 显示到助手对话框
    // 步骤 3：重填"助手方案"下拉框：先清空，再把每个候选方案写成一行文字加进去
    SendMessageW(agentOptions,CB_RESETCONTENT,0,0);const cJSON *proposal;int option=0;
    cJSON_ArrayForEach(proposal,lab_agent_proposals(&agent)){
        char label[1024];
        lab_agent_option(proposal,++option,label,sizeof(label)); // 生成"方案 1 · 日期 时间 · 设备 #03"
        wchar_t *value=wide(label);
        SendMessageW(agentOptions,CB_ADDSTRING,0,(LPARAM)value);
        free(value);
    }
    SendMessageW(agentOptions,CB_SETCURSEL,option?0:(WPARAM)-1,0); // 有方案默认选第一个，没有就不选
    // 步骤 4：重填"智能推荐"下拉框。推荐结果的格式和助手方案不同，
    // 这里临时包一层 {"candidate":{"items":[候选]}}，就能复用 lab_agent_option 生成文字，用完立刻删除
    SendMessageW(scheduleOptions,CB_RESETCONTENT,0,0);
    const cJSON *candidate;int candidateCount=0;cJSON_ArrayForEach(candidate,scheduleCandidates){
        char label[1024];
        cJSON *wrapper=cJSON_CreateObject(),*body=cJSON_CreateObject(),*items=cJSON_CreateArray();
        cJSON_AddItemToArray(items,cJSON_Duplicate(candidate,1)); // 放进去的是深拷贝，删除 wrapper 时不会误删原数据
        cJSON_AddItemToObject(body,"items",items);
        cJSON_AddItemToObject(wrapper,"candidate",body);
        lab_agent_option(wrapper,++candidateCount,label,sizeof(label));
        cJSON_Delete(wrapper); // 删外层会连带删除 body、items 和那份拷贝
        wchar_t *value=wide(label);SendMessageW(scheduleOptions,CB_ADDSTRING,0,(LPARAM)value);free(value);}
    SendMessageW(scheduleOptions,CB_SETCURSEL,candidateCount?0:(WPARAM)-1,0);
    // 步骤 5：清空两张列表的旧行和旧列。rendering=1 期间，删行产生的"选中变化"通知会被忽略
    rendering=1;
    ListView_DeleteAllItems(usageList);
    while(ListView_DeleteColumn(usageList,0)){}
    // 暂停列表重画，全部填完再一次性画出来，避免闪烁
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list);
    while (ListView_DeleteColumn(list, 0)) {
    }
    listedCount = 0;
    // 步骤 6：按页面填表。页面 0（预约）和 1（我的）都显示预约记录，只是列和筛选条件不同
    if (page < 2) {
        column(0, L"设备", 75);
        column(1, L"日期（北京）", 120);
        column(2, L"时段", 100);
        column(3, L"状态", 95);
        column(4, page==1?L"实际签到":L"预约人", 160);
        if(page==1){column(5,L"实际签退",160);column(6,L"实际使用",100);}
        // 逐条检查预约记录
        cJSON_ArrayForEach(r, model.bookings) {
            // "我的"页：只显示自己的预约
            if (page == 1 && strcmp(lab_str(r, "user_id"), client.session.uid))
                continue;
            // 预约页：只显示所选日期的；学生只看到自己的，管理员看到所有人的
            if (page == 0 && (strcmp(lab_str(r, "reservation_date"), d)||(!admin()&&strcmp(lab_str(r,"user_id"),client.session.uid))))
                continue;
            // 超过数组容量就停，防止 listed 越界
            if (i >= 100000)
                break;
            // 记下第 i 行对应哪条记录，之后点按钮时能找回它
            listed[i] = (cJSON *)r;
            snprintf(b, sizeof(b), "%02d", lab_int(r, "device_number"));
            add_cell(i, 0, b);
            add_cell(i, 1, lab_str(r, "reservation_date"));
            // 时段显示成 "09:00–10:00"（每条预约记录就是一小时）
            snprintf(b, sizeof(b), "%02d:00–%02d:00", lab_int(r, "start_hour"), lab_int(r, "start_hour") + 1);
            add_cell(i, 2, b);
            add_cell(i, 3, ui_status(lab_str(r, "status")));
            // 预约人：先拿到 user_id，再去用户表里找对应的姓名
            const char *owner = lab_str(r, "user_id");
            const cJSON *p;
            cJSON_ArrayForEach(p, model.profiles) if (!strcmp(lab_str(p, "id"), owner)) {
                owner = lab_str(p, "display_name");
                break;
            }
            // "我的"页额外显示实际签到、签退时间和使用时长
            if(page==1){
                const cJSON *u=ui_usage(&model,r);char value[96];
                ui_time(lab_str(u,"checked_in_at"),value,sizeof(value));add_cell(i,4,value);
                ui_time(lab_str(u,"checked_out_at"),value,sizeof(value));add_cell(i,5,value);
                ui_duration(u,value,sizeof(value));add_cell(i,6,value);
            }else add_cell(i, 4, owner);
            i++;
        }
    // 页面 2（故障记录）：设备、状态、故障说明、处理记录、报告时间
    } else if (page == 2) {
        column(0, L"设备", 65);
        column(1, L"状态", 85);
        column(2, L"故障说明", 290);
        column(3, L"处理记录", 230);
        column(4, L"报告时间", 190);
        cJSON_ArrayForEach(r, model.issues) {
            if (i >= 100000)
                break;
            listed[i] = (cJSON *)r;
            snprintf(b, sizeof(b), "%02d", lab_int(r, "device_number"));
            add_cell(i, 0, b);
            add_cell(i, 1, ui_status(lab_str(r, "status")));
            add_cell(i, 2, lab_str(r, "description"));
            add_cell(i, 3, lab_str(r, "resolution_note"));
            add_cell(i, 4, lab_str(r, "created_at"));
            i++;
        }
    // 页面 3（用户管理，仅管理员）：姓名、学号、角色、是否已停用
    } else if(page==3) {
        column(0, L"姓名", 180);
        column(1, L"学号 / 登录名", 220);
        column(2, L"角色", 90);
        column(3, L"账号状态", 110);
        cJSON_ArrayForEach(r, model.profiles) {
            if (i >= 100000)
                break;
            listed[i] = (cJSON *)r;
            add_cell(i, 0, lab_str(r, "display_name"));
            add_cell(i, 1, lab_str(r, "login_id"));
            add_cell(i, 2, lab_str(r, "role"));
            add_cell(i, 3, cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(r, "active")) ? "已停用" : "可用");
            i++;
        }
    }
    // 步骤 7：之前选中过某条记录的话，按 id 找到它并恢复选中（刷新后用户的选择不会丢）
    listedCount = i;
    for(int row=0;row<i;row++)if(*selectedId&&!strcmp(lab_str(listed[row],"id"),selectedId)){
        ListView_SetItemState(list,row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
        if(page==1)ListView_SetItemState(usageList,row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    }
    // 步骤 8：恢复重画
    rendering=0;
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, NULL, TRUE);
    // 步骤 9：左下角显示"姓名 · 学生/管理员 · 版本号"
    snprintf(b, sizeof(b), "%s  ·  %s  ·  原生 C %s", client.session.name, admin() ? "管理员" : "学生",
             LAB_NATIVE_VERSION);
    set_utf8(whoText, b);
    // 步骤 10：通知 40 个设备按钮和整个窗口重画（设备颜色取决于最新数据）
    for (i = 0; i < 40; i++)
        InvalidateRect(deviceButtons[i], NULL, TRUE);
    InvalidateRect(window,NULL,TRUE);
}

/* ===================== 模块 6：控件显示与隐藏 visibility =====================
 * 功能：决定"现在该看到哪些控件、哪些按钮能点"。未登录只看到登录表单；登录后按页面和角色显示。
 * 场景：所有页面共用同一个窗口。切换页面不是换窗口，而是把不属于当前页的控件藏起来。
 */
static void visibility(void) {
    // 登录表单：未登录时显示，登录后隐藏
    HWND auth[] = {identity, password, nameEdit, inviteEdit, loginButton, registerButton, loginInfo};
    for (size_t i = 0; i < sizeof(auth) / sizeof(auth[0]); i++)
        ShowWindow(auth[i], logged ? SW_HIDE : SW_SHOW);
    for (int i = 0; i < 4; i++)
        ShowWindow(labels[i], logged ? SW_HIDE : SW_SHOW);
    // 顶部标签页是旧版导航，已改用左侧侧栏，始终隐藏
    ShowWindow(tabs,SW_HIDE);
    // 侧栏 5 个按钮：第 3、4 个（故障记录、用户管理）只有管理员能看到
    for(int i=0;i<5;i++)ShowWindow(navigation[i],logged&&((i!=2&&i!=3)||admin())?SW_SHOW:SW_HIDE);
    // 下面每一行：某个控件只在特定页面出现，写法是"条件 ? SW_SHOW : SW_HIDE"
    ShowWindow(profileFault,logged&&page==1?SW_SHOW:SW_HIDE);
    ShowWindow(usageList,logged&&page==1?SW_SHOW:SW_HIDE);
    // 推荐方案、助手方案两个下拉框：有内容时才出现
    ShowWindow(scheduleOptions,logged&&page==0&&cJSON_GetArraySize(scheduleCandidates)>0?SW_SHOW:SW_HIDE);
    ShowWindow(agentOptions,logged&&page==4&&cJSON_GetArraySize(lab_agent_proposals(&agent))>0?SW_SHOW:SW_HIDE);
    ShowWindow(agentReply,logged&&page==4?SW_SHOW:SW_HIDE);
    ShowWindow(list, logged&&page!=4 ? SW_SHOW : SW_HIDE);
    ShowWindow(whoText, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(dateEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    ShowWindow(startEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    ShowWindow(endEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    ShowWindow(noteEdit, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(helpText, logged ? SW_SHOW : SW_HIDE);
    for (int i = 0; i < 40; i++)
        ShowWindow(deviceButtons[i], logged && page == 0 ? SW_SHOW : SW_HIDE);
    // 19 个操作按钮逐个判断：默认登录后可见，再按按钮种类覆盖规则
    for (int i = 0; i < 19; i++) {
        int show = logged;
        int id = actionIds[i];
        // 导出：管理员在故障页或用户管理页
        if(id==ID_EXPORT_BOOKINGS||id==ID_EXPORT_ISSUES)show=logged&&admin()&&(page==2||page==3);
        // "采用推荐方案"：预约页且已有推荐结果
        if(id==ID_USE_CANDIDATE)show=logged&&page==0&&cJSON_GetArraySize(scheduleCandidates)>0;
        // "选择此方案"：助手页且助手给出了方案
        if(id==ID_AGENT_CONFIRM)show=logged&&page==4&&cJSON_GetArraySize(lab_agent_proposals(&agent))>0;
        // "发送"：助手页；管理员在用户管理页也能用
        if (id == ID_AGENT)
            show = logged && (page == 4 || (page == 3 && admin()));
        if (id == ID_PROFILE)show=logged&&page==1;
        // "确认预约""智能推荐"：只在预约页
        if (id == ID_BOOK || id == ID_SUGGEST)
            show = logged && page == 0;
        // "签到/签退"在我的页；"取消预约"在我的页，管理员在预约页也能取消别人的
        if (id == ID_ATTEND || id == ID_CANCEL)
            show = logged && (page==1||(id==ID_CANCEL&&page==0&&admin()));
        // "报告设备故障"：故障页或预约页
        if (id == ID_REPORT)
            show = logged && (page == 2 || page == 0);
        // "完成维修""归档故障"：管理员在故障页
        if (id == ID_REPAIR || id == ID_ARCHIVE)
            show = logged && page == 2 && admin();
        // "停用学生""生成邀请码""统计报表"：管理员在用户管理页
        if (id == ID_DEACTIVATE || id == ID_INVITE || id == ID_METRICS)
            show = logged && page == 3 && admin();
        ShowWindow(actions[i], show ? SW_SHOW : SW_HIDE);
        // 有后台任务时全部变灰，防止重复点击；检查版本期间"检查版本"按钮单独变灰
        EnableWindow(actions[i], !busy && (id != ID_UPDATE || !checkingUpdate));
    }
    EnableWindow(loginButton, !busy);
    EnableWindow(registerButton, !busy);
    EnableWindow(tabs, !busy);
    // 页面顶部的帮助文字随页面变化
    SetWindowTextW(
        helpText, page == 0 ? L"选择日期、开始/结束小时和设备；多小时预约逐时提交。设备颜色对应所选整个时段。"
                  : page == 1
                      ? L"选择自己的预约，再点击签到 / 签退。签到仅限预约时段（北京时间），签退需先签到。"
                  : page == 4 ? L"根据设备、日期和时段获取建议；确认后再预约。"
                  : page == 2 ? L"在说明框输入故障/维修说明。报告故障使用预约页中选中的第一台设备。"
                              : L"管理操作由云端验证权限。修改本人资料：说明框输入“姓名|学号或邮箱”。");
}

/* ===================== 模块 7：后台任务 worker =====================
 * 功能：在后台线程里真正执行联网操作，完成后把结果寄回界面线程。
 * 场景：登录、注册、预约、签到签退、报故障、管理操作、问助手、检查新版本，全都在这里联网。
 * 为什么要后台线程：联网可能要好几秒，如果放在界面线程里做，这期间窗口无法响应，会显示"未响应"。
 * 规则：这里只读写参数 j 指向的 Job，不碰任何全局变量和控件。
 */
// 取用户在列表里选中的那条记录（"我的"页如果选的是使用记录表，就从那张表取）。
// 返回的是模型内部指针，交给后台线程前必须深拷贝
static cJSON *selected_row(void) {
    int i = ListView_GetNextItem(page==1&&selectedTable?usageList:list, -1, LVNI_SELECTED);
    return i >= 0 && i < listedCount ? listed[i] : NULL;
}
// 往请求参数里加一个字符串字段；字段名必须和云端函数的参数名一致
static void parameter(cJSON *p, const char *key, const char *s) { cJSON_AddStringToObject(p, key, s); }
// 后台线程入口。CreateThread 要求函数形如 DWORD WINAPI f(void *)，所以参数是 void *，进来后转回 Job *
static DWORD WINAPI worker(void *arg) {
    Job *j = arg;
    cJSON *p = NULL, *r = NULL;
    // committed：多小时预约中已经成功提交了几个小时
    int success = 0, committed = 0;
    // 【任务：检查新版本】从官网下载 native-desktop.json 比较版本号。它不需要登录、也不刷新数据，最后直接跳到结尾
    if (j->kind == JOB_UPDATE) {
        int status = 0;
        // 这里直接调用 Windows 联网函数（不经过业务层），因为官网不是云端数据库，不需要令牌
        char *raw = lab_win_http(NULL, LAB_SITE_HOST, "GET", LAB_SITE_BASE "native-desktop.json", "", "", "",
                                 &status, j->client.error, sizeof(j->client.error));
        if (raw) {
            r = cJSON_Parse(raw);
            free(raw);
        }
        // 下载成功、解析成功、且线上版本更新时，才准备提示文字
        if (status == 200 && r && lab_semver_newer(lab_str(r, "version"), LAB_NATIVE_VERSION)) {
            snprintf(j->message, sizeof(j->message), "发现原生桌面新版本 %s。是否打开官网下载页？",
                     lab_str(r, "version"));
            j->ok = 1;
        }
        cJSON_Delete(r);
        // goto post：跳过下面的业务和数据刷新，直接去结尾寄回结果
        goto post;
    }
    // 【任务：登录 / 注册 / 刷新】刷新本身什么都不用做，成功后统一走下面的"重新加载数据"
    if (j->kind == JOB_LOGIN)
        success = lab_login(&j->client, j->identity, j->password);
    else if (j->kind == JOB_REGISTER)
        success = lab_register(&j->client, j->name, j->identity, j->password, j->invite);
    else if (j->kind == JOB_REFRESH)
        success = 1;
    // 【任务：预约】把 Job 里的 9 个编号转成 lab_reserve_range 需要的指针数组，然后按小时逐个提交
    else if (j->kind == JOB_BOOK) {
        const char *keys[9];
        for (int i = 0; i < 9; i++)
            keys[i] = j->keys[i];
        success = lab_reserve_range(&j->client, j->date, j->start, j->end, j->devices, (size_t)j->count, keys,
                                    &committed);
        // 中途失败时告诉用户前面已经约上了几个小时（逐小时提交不是一个整体事务）
        if (!success)
            snprintf(j->message, sizeof(j->message),
                     "已确认成功 %d 个小时；本次未能完成全部提交，请刷新核实剩余小时。%s", committed,
                     j->client.error);
    // 【任务：签到/签退】当前时间在这里现取，因为用户点"确认"到真正发请求之间可能过了一会儿
    } else if (j->kind == JOB_ATTEND)
        success = lab_attend(&j->client, j->row, j->action, now_ms());
    // 【任务：智能助手】把请求发给云端 AI 函数；回复留在 Job 里，由界面线程记进对话历史
    else if (j->kind == JOB_AGENT) {
        r = lab_request(&j->client,"POST","/functions/v1/deepseek-booking-agent",j->agentRequest);
        j->agentResult=r;
        // 回复里 outcome 为 failed 才算失败
        success=r && strcmp(lab_str(r,"outcome"),"failed");
        // 有错误就显示第一条错误，否则显示 AI 的回复文字
        if(r){const cJSON *errors=cJSON_GetObjectItemCaseSensitive(r,"errors");
            snprintf(j->message,sizeof(j->message),"%s",cJSON_GetArraySize(errors)?lab_str(cJSON_GetArrayItem(errors,0),"message"):lab_str(r,"message"));}

    // 【其余任务：调用云端数据库函数】先按任务种类决定函数名 rpc 和参数 p，最后统一发送
    } else {
        const char *rpc = NULL;
        p = cJSON_CreateObject();
        switch (j->kind) {
        // 取消预约：参数是预约 id
        case JOB_CANCEL:
            rpc = "v2_cancel_reservation";
            parameter(p, "p_reservation_id", lab_str(j->row, "id"));
            break;
        // 报告故障：用预约页选中的第一台设备 + 说明框文字
        case JOB_REPORT:
            rpc = "report_device_fault";
            cJSON_AddNumberToObject(p, "p_device_number", j->devices[0]);
            parameter(p, "p_description", j->note);
            break;
        // 完成维修：故障 id + 维修说明
        case JOB_REPAIR:
            rpc = "v2_complete_fault_repair";
            parameter(p, "p_issue_id", lab_str(j->row, "id"));
            parameter(p, "p_note", j->note);
            break;
        // 归档已修复的故障
        case JOB_ARCHIVE:
            rpc = "archive_resolved_issue";
            parameter(p, "p_issue_id", lab_str(j->row, "id"));
            break;
        // 停用学生账号
        case JOB_DEACTIVATE:
            rpc = "deactivate_student";
            parameter(p, "target_user", lab_str(j->row, "id"));
            break;
        // 生成管理员邀请码：参数是有效天数
        case JOB_INVITE:
            rpc = "create_admin_invite_for_days";
            cJSON_AddNumberToObject(p, "p_valid_days", j->inviteDays);
            break;
        // 统计报表：不走通用 rpc，下面单独调用 lab_usage_metrics
        case JOB_METRICS:
            rpc = NULL;
            parameter(p, "p_start_date", j->date);
            parameter(p, "p_end_date", j->date);
            break;
        // 修改本人资料：说明框格式是 "姓名|学号"，在 | 处切成两段
        case JOB_PROFILE: {
            char *sep = strchr(j->note, '|');
            if (!sep) {
                snprintf(j->client.error, sizeof(j->client.error), "请在说明框输入：姓名|学号或邮箱");
                break;
            }
            // 把 | 改成字符串结束符 0：note 只剩姓名，sep 指向后半段学号。*sep++ = 0 是先写 0、再让指针后移一位
            *sep++ = 0;
            rpc = "update_profile_identity";
            parameter(p, "target_user", j->client.session.uid);
            parameter(p, "next_display_name", j->note);
            parameter(p, "next_login_id", sep);
            break;
        }
        // 智能推荐：给定日期、偏好的小时、需要几台设备，最多返回 3 个方案
        case JOB_SUGGEST:
            rpc = "v2_schedule_candidates";
            parameter(p, "p_date", j->date);
            cJSON_AddNumberToObject(p, "p_preferred_hour", j->start);
            cJSON_AddNumberToObject(p, "p_required_device_count", j->count ? j->count : 1);
            cJSON_AddItemToObject(p, "p_preferred_devices", cJSON_CreateArray());
            cJSON_AddNumberToObject(p, "p_limit", 3);
            break;
        }
        // 统一发送：统计报表走专用函数，其他有函数名的走 lab_rpc；发完就释放参数对象
        if (j->kind == JOB_METRICS)
            r = lab_usage_metrics(&j->client,j->date,j->endDate,j->metricDevice);
        else if (rpc)
            r = lab_rpc(&j->client, rpc, p);
        cJSON_Delete(p);
        // 云端返回了数据就算成功
        success = r != NULL;
        // 邀请码：从返回结果里取出邀请码和过期时间，拼成提示
        if (success && j->kind == JOB_INVITE) {
            const cJSON *v = cJSON_IsArray(r) ? cJSON_GetArrayItem(r, 0) : r;
            snprintf(j->message, sizeof(j->message),
                     "管理员邀请码：%s\n有效期至：%s\n请妥善保管，仅分享给授权管理员。",
                     lab_str(v, "invite_code"), lab_str(v, "expires_at"));
        }
        // 统计报表：把结果排版成文字，界面线程用报表窗口显示
        if(success && j->kind==JOB_METRICS)lab_metrics_report(r,j->message,sizeof(j->message));
        // 智能推荐：把每个方案拼成"日期 时间，设备：03 05"一行。每次 snprintf 后检查剩余空间，写满就停，不会越界
        if (success && j->kind == JOB_SUGGEST) {
            const cJSON *v;
            size_t used = 0;
            snprintf(j->message, sizeof(j->message), "可预约方案（请选择设备后确认预约）：\n");
            used = strlen(j->message);
            cJSON_ArrayForEach(v, r) {
                int n = snprintf(j->message + used, sizeof(j->message) - used, "%s %02d:00，设备：",
                                 lab_str(v, "reservation_date"), lab_int(v, "start_hour"));
                if (n < 0 || (size_t)n >= sizeof(j->message) - used)
                    break;
                used += (size_t)n;
                const cJSON *device;
                cJSON_ArrayForEach(device, cJSON_GetObjectItemCaseSensitive(v, "device_numbers")) {
                    n = snprintf(j->message + used, sizeof(j->message) - used, "%02d ", device->valueint);
                    if (n < 0 || (size_t)n >= sizeof(j->message) - used)
                        break;
                    used += (size_t)n;
                }
                if (used + 2 < sizeof(j->message)) {
                    j->message[used++] = '\n';
                    j->message[used] = 0;
                }
            }
            if (cJSON_GetArraySize(r) == 0)
                snprintf(j->message, sizeof(j->message), "当前条件下没有可预约的方案。");
        }
        // 推荐结果要交给界面线程放进下拉框，所以把所有权转给 Job（r 置 NULL，下一行就不会删掉它）
        if(j->kind==JOB_SUGGEST && success){j->agentResult=r;r=NULL;}
        cJSON_Delete(r);
    }
    // 【所有任务共同的收尾】记录成败；失败且没有专门提示时，用业务层写好的错误原因
    j->ok = success;
    if (!success && !*j->message)
        snprintf(j->message, sizeof(j->message), "%s", j->client.error);
    // 只要还处于登录状态，就重新加载全部数据，让界面立刻看到操作结果
    if (*j->client.session.access) {
        j->loaded = lab_load_model(&j->client, &j->model);
        // 操作成功但刷新失败：明确告诉用户，避免误以为没约上而重复提交
        if (success && !j->loaded) {
            snprintf(j->message, sizeof(j->message), "操作已完成，但刷新失败：%s。请刷新核实后再操作。",
                     j->client.error);
        }
    }
// post：所有分支最终都到这里
post:
    // 密码用完立刻擦除。SecureZeroMemory 保证这次清零不会被编译器当成"写了没人读"而优化掉
    SecureZeroMemory(j->password, sizeof(j->password));
    // 把 Job 寄回主窗口。PostMessageW 只把消息放进主窗口的队列就返回，由界面线程稍后处理，这是跨线程交还数据的安全方式。
    // 投递失败（例如窗口已关闭）就在这里自己释放 Job，避免内存泄漏
    if (!PostMessageW(j->target, RESULT_MESSAGE, 0, (LPARAM)j)) {
        lab_model_free(&j->model);
        lab_session_clear(&j->client.session);
        cJSON_Delete(j->row);cJSON_Delete(j->agentRequest);cJSON_Delete(j->agentResult);
        free(j);
    }
    return 0;
}

/* ===================== 模块 8：启动后台任务 start_job =====================
 * 功能：把 Job 交给新线程执行，并让界面进入"请稍候"状态。
 */
static void start_job(Job *j) {
    HANDLE thread;
    // 记下结果要寄回的窗口
    j->target = window;
    // 结构体整体复制：LabSession 里全是数组，这一句就把令牌等完整复制进 Job，后台线程用自己这份
    j->client = client;
    // 检查版本是悄悄进行的，不锁住界面；其他任务置 busy=1，按钮全部变灰
    if (j->kind == JOB_UPDATE)
        checkingUpdate = 1;
    else
        busy = 1;
    visibility();
    if (j->kind != JOB_UPDATE)
        SetWindowTextW(statusText, L"正在连接云端，请稍候…");
    // 创建线程运行 worker(j)。我们不需要等它结束，线程句柄立即关闭即可（线程本身照常运行）
    thread = CreateThread(NULL, 0, worker, j, 0, NULL);
    if (thread)
        CloseHandle(thread);
    // 创建线程失败：恢复界面状态，释放 Job 里的所有资源，再提示用户
    else {
        if (j->kind == JOB_UPDATE)
            checkingUpdate = 0;
        else
            busy = 0;
        visibility();
        cJSON_Delete(j->row);cJSON_Delete(j->agentRequest);cJSON_Delete(j->agentResult);
        lab_session_clear(&j->client.session);
        free(j);
        tell("无法创建请求线程，请重试。", MB_ICONERROR);
    }
}

/* ===================== 模块 9：弹出窗口——报表、输入表单、导出 CSV =====================
 * 功能：管理员点"统计报表"后显示一个可滚动的文字窗口；点"生成邀请码""统计报表""修改本人资料"时
 *       先弹出一个小表单让用户填写；点"导出"时弹出"另存为"对话框，把记录写成 CSV。
 * 写法：Win32 没有现成的输入框函数，这里自己注册窗口类，并在函数里跑一个小消息循环，
 *       直到用户关掉窗口才返回，效果相当于一个模态对话框。
 */
static LRESULT CALLBACK report_proc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
 // 报表窗口的消息处理：创建时放一个只读多行文本框，改变大小时让文本框铺满，关闭时销毁
 if(msg==WM_CREATE){
     wchar_t *text=wide((const char*)((CREATESTRUCTW*)lp)->lpCreateParams); // 报表文字是创建窗口时最后一个参数传进来的
     HWND edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",text,WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,12,12,760,520,h,(HMENU)210,NULL,NULL);
     free(text);
     SendMessageW(edit,WM_SETFONT,(WPARAM)font,TRUE);
     return 0;}
 if(msg==WM_SIZE){MoveWindow(GetDlgItem(h,210),12,12,LOWORD(lp)-24,HIWORD(lp)-24,TRUE);return 0;} // 窗口缩放时，文本框四周留 12 像素边距
 if(msg==WM_CLOSE){DestroyWindow(h);return 0;}return DefWindowProcW(h,msg,wp,lp); // 其他消息交给系统默认处理
}
// 显示一个可滚动的文字报表窗口，用户关闭后才返回
static void native_report(const char *title,const char *text){
 // 步骤 1：注册报表窗口类（指定消息处理函数 report_proc）
 WNDCLASSW cls={0};cls.lpfnWndProc=report_proc;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"LabNativeReport";cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&cls);wchar_t *caption=wide(title);
 // 步骤 2：创建窗口，把报表文字通过最后一个参数传给 WM_CREATE
 HWND dialog=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,caption,WS_OVERLAPPEDWINDOW,100,100,840,640,window,NULL,cls.hInstance,(void*)text);free(caption);if(!dialog)return;
 // 步骤 3：禁用主窗口，跑一个小消息循环直到报表窗口关闭；然后恢复主窗口和 busy 原值
 int previousBusy=busy;busy=1;EnableWindow(window,FALSE);ShowWindow(dialog,SW_SHOW);
 MSG message;while(IsWindow(dialog)&&GetMessageW(&message,NULL,0,0)>0){if(!IsDialogMessageW(dialog,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
 EnableWindow(window,TRUE);SetActiveWindow(window);busy=previousBusy;
}
// 表单状态：每个字段的标题、值（每个最多 256 字节）、字段数、用户是否点了确定、输入框句柄（最多 3 个）
typedef struct {const char **labels;char (*values)[256];int count,accepted;HWND fields[3];} NativeForm;
static LRESULT CALLBACK form_proc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
 NativeForm *f=(NativeForm*)GetWindowLongPtrW(h,GWLP_USERDATA); // 取出挂在窗口上的表单状态
 if(msg==WM_CREATE){
     // 创建时：把 native_form 传来的表单状态指针挂到窗口上，之后每条消息都能取回
     f=(NativeForm*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)f);
     // 每个字段一行：上面是标题，下面是输入框（填好默认值）
     for(int i=0;i<f->count;i++){wchar_t *caption=wide(f->labels[i]),*value=wide(f->values[i]);
         HWND label=CreateWindowW(L"STATIC",caption,WS_CHILD|WS_VISIBLE,20,16+i*76,450,24,h,NULL,NULL,NULL);
         f->fields[i]=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",value,WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,20,42+i*76,450,30,h,(HMENU)(INT_PTR)(200+i),NULL,NULL);
         SendMessageW(label,WM_SETFONT,(WPARAM)font,TRUE);SendMessageW(f->fields[i],WM_SETFONT,(WPARAM)font,TRUE);
         SendMessageW(f->fields[i],EM_SETLIMITTEXT,255,0); // 最多 255 个字符
         free(caption);free(value);}
     // 底部"确定""取消"两个按钮
     HWND ok=CreateWindowW(L"BUTTON",L"确定",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,260,24+f->count*76,100,34,h,(HMENU)IDOK,NULL,NULL);HWND cancel=CreateWindowW(L"BUTTON",L"取消",WS_CHILD|WS_VISIBLE|WS_TABSTOP,370,24+f->count*76,100,34,h,(HMENU)IDCANCEL,NULL,NULL);SendMessageW(ok,WM_SETFONT,(WPARAM)font,TRUE);SendMessageW(cancel,WM_SETFONT,(WPARAM)font,TRUE);return 0;}
 // 点确定：把每个输入框的内容读回 values 并标记 accepted；确定或取消都关闭窗口
 if(msg==WM_COMMAND&&(LOWORD(wp)==IDOK||LOWORD(wp)==IDCANCEL)){if(LOWORD(wp)==IDOK){for(int i=0;i<f->count;i++)get_utf8(f->fields[i],f->values[i],256);f->accepted=1;}DestroyWindow(h);return 0;}
 if(msg==WM_CLOSE){DestroyWindow(h);return 0;}return DefWindowProcW(h,msg,wp,lp);
}
// 显示一个小表单并等待用户填写：点确定返回 1 并把内容写回 values，点取消返回 0。
// 流程和 native_report 一样：注册窗口类 → 创建窗口 → 禁用主窗口跑小消息循环 → 恢复
static int native_form(const char *title,const char *labels[],char values[][256],int count){
 WNDCLASSW cls={0};cls.lpfnWndProc=form_proc;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"LabNativeForm";cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&cls);
 // 表单状态放在本函数的栈上，把指针交给窗口；窗口关闭前本函数不会返回，所以这个指针一直有效
 NativeForm f={labels,values,count,0,{0}};wchar_t *caption=wide(title);RECT r;GetWindowRect(window,&r);
 HWND dialog=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,caption,WS_CAPTION|WS_SYSMENU,r.left+150,r.top+100,510,115+count*76,window,NULL,cls.hInstance,&f);free(caption);if(!dialog)return 0;
 // 光标默认放在第一个输入框
 int previousBusy=busy;busy=1;EnableWindow(window,FALSE);ShowWindow(dialog,SW_SHOW);SetFocus(f.fields[0]);MSG message;
 while(IsWindow(dialog)&&GetMessageW(&message,NULL,0,0)>0){if(!IsDialogMessageW(dialog,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
 EnableWindow(window,TRUE);SetActiveWindow(window);busy=previousBusy;return f.accepted;
}
// 导出 CSV：弹出"另存为"对话框（默认文件名 reservations.csv 或 issues.csv），写入当前已加载的全部记录
static void export_records(int issues){
 wchar_t path[MAX_PATH];wcscpy(path,issues?L"issues.csv":L"reservations.csv"); // 默认文件名
 // 配置另存为对话框：只显示 CSV 文件、自动补 .csv 后缀、覆盖已有文件前先确认
 OPENFILENAMEW save={0};save.lStructSize=sizeof(save);save.hwndOwner=window;save.lpstrFile=path;save.nMaxFile=MAX_PATH;save.lpstrFilter=L"CSV 文件\0*.csv\0\0";save.lpstrDefExt=L"csv";save.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 // 用户点了保存：以二进制方式打开并写入；写入失败或关闭失败（如磁盘已满）都算导出失败
 if(GetSaveFileNameW(&save)){FILE *out=_wfopen(path,L"wb");int ok=out&&lab_export_csv(&model,issues,out);if(out&&fclose(out)!=0)ok=0;tell(ok?"完整记录已导出。":"导出失败，请检查保存位置。",ok?MB_ICONINFORMATION:MB_ICONERROR);}
}

/* ===================== 模块 10：按钮分派 command =====================
 * 功能：用户点了某个按钮后决定具体做什么：检查输入 → 必要时弹确认框 → 打包成 Job → 交给后台。
 * 场景：参数 id 就是被点按钮的编号（模块 2 的 ID_xxx）。30 秒自动刷新也会调用 command(ID_REFRESH)。
 * 写法：中途发现输入有问题就 goto abort，在函数末尾统一释放 Job；需要用户确认的 goto confirm。
 *       C 没有异常和析构函数，跳到统一出口是避免漏释放内存的常见写法。
 */
static void command(int id) {
    Job *j;
    cJSON *row;
    char confirmation[512];
    // 有任务在执行或处于预览模式时，忽略所有点击
    if (busy || preview)
        return;
    // 【退出账号】不需要联网：清空助手对话、推荐结果、令牌（安全擦除）和数据，回到登录页
    if (id == ID_LOGOUT) {
        lab_agent_clear(&agent);cJSON_Delete(scheduleCandidates);scheduleCandidates=NULL;
        lab_session_clear(&client.session);
        lab_model_free(&model);
        logged = 0;
        scrollY=0;selectedId[0]=0;
        listedCount = 0;
        ListView_DeleteAllItems(list);
        SetWindowTextW(password, L"");
        layout();
        return;
    }
    // 【采用推荐方案】也不联网：把选中的推荐方案填回日期框、小时框和设备选择，用户再点"确认预约"
    if(id==ID_USE_CANDIDATE){
 // 下拉框当前选的第几项，就取第几个候选方案
 const cJSON *candidate=cJSON_GetArrayItem(scheduleCandidates,(int)SendMessageW(scheduleOptions,CB_GETCURSEL,0,0));
 const cJSON *numbers=cJSON_GetObjectItemCaseSensitive(candidate,"device_numbers");int hour=lab_int(candidate,"start_hour");const char *date=lab_str(candidate,"reservation_date");
 // 数据不合法（日期长度不对、小时超出 8~16、没有设备）就什么都不做
 if(strlen(date)!=10||hour<8||hour>16||!cJSON_GetArraySize(numbers))return;
 SYSTEMTIME time={0};int year,month,day;if(sscanf(date,"%d-%d-%d",&year,&month,&day)!=3)return; // 从 "2026-11-05" 拆出年月日
 time.wYear=(WORD)year;time.wMonth=(WORD)month;time.wDay=(WORD)day;DateTime_SetSystemtime(dateEdit,GDT_VALID,&time); // 设置日期框
 SendMessageW(startEdit,CB_SETCURSEL,hour-8,0);SendMessageW(endEdit,CB_SETCURSEL,hour-8,0); // 开始框第 0 项是 8 点、结束框第 0 项是 9 点，下标都取 hour-8 正好选中一小时
 memset(selected,0,sizeof(selected));const cJSON *n;cJSON_ArrayForEach(n,numbers)if(n->valueint>=1&&n->valueint<=40)selected[n->valueint-1]=1; // 清掉原来的设备选择，换成方案里的设备
 // 重画，让用户看到填好的结果
 render();return;
 }
    // 【导出】只有管理员能导出；读的是本地已有数据，不需要后台线程
    if(id==ID_EXPORT_BOOKINGS||id==ID_EXPORT_ISSUES){if(admin())export_records(id==ID_EXPORT_ISSUES);return;}
    // —— 以下按钮都要联网，先创建 Job。calloc 会把所有字段清零，没填的字段就是 0 或空串 ——
    j = calloc(1, sizeof(*j));
    if (!j)
        return;
    // 把当前界面上的输入统一抄进 Job：日期、开始/结束小时、说明框、选中的设备。后台线程只用这份副本
    date_string(j->date);
    // 开始框第 0 项是 8 点；结束框第 0 项是 9 点
    j->start = 8 + (int)SendMessageW(startEdit, CB_GETCURSEL, 0, 0);
    j->end = 9 + (int)SendMessageW(endEdit, CB_GETCURSEL, 0, 0);
    get_utf8(noteEdit, j->note, sizeof(j->note));
    // selected[i] 为 1 表示第 i+1 号设备被选中
    for (int i = 0; i < 40; i++)
        if (selected[i])
            j->devices[j->count++] = i + 1;
    // 【统计报表】先弹表单问起止日期和设备编号（默认今天、全部设备），检查格式后存进 Job
    if(id==ID_METRICS){const char *labels[]={"开始日期 YYYY-MM-DD","结束日期 YYYY-MM-DD","设备编号（0 为全部，1–40）"};char fields[3][256];snprintf(fields[0],256,"%s",j->date);snprintf(fields[1],256,"%s",j->date);snprintf(fields[2],256,"0");
        if(!native_form("实验室使用情况",labels,fields,3))goto abort;
        // 日期必须是 10 个字符；设备编号必须全是数字、不能为空、不超过 40
        if(strlen(fields[0])!=10||strlen(fields[1])!=10||strspn(fields[2],"0123456789")!=strlen(fields[2])||!*fields[2]||atoi(fields[2])>40){tell("请填写有效日期和设备编号。",MB_ICONINFORMATION);goto abort;}
        snprintf(j->date,16,"%.10s",fields[0]);snprintf(j->endDate,16,"%.10s",fields[1]);j->metricDevice=atoi(fields[2]);
    }
    // 【生成邀请码】弹表单问有效天数，只接受 1、7、30
    if(id==ID_INVITE){const char *labels[]={"有效天数：1、7 或 30"};char fields[1][256]={"7"};if(!native_form("生成管理员邀请码",labels,fields,1))goto abort;
        if(strcmp(fields[0],"1")&&strcmp(fields[0],"7")&&strcmp(fields[0],"30")){tell("请选择 1、7 或 30 天。",MB_ICONINFORMATION);goto abort;}j->inviteDays=atoi(fields[0]);
    }
    // 【修改本人资料】弹表单，默认填当前姓名和学号；任一项为空或含 | 就放弃；拼成 "姓名|学号" 交给 worker 再拆开
    if(id==ID_PROFILE){const char *labels[]={"姓名","学号 / 管理员邮箱"};char fields[2][256];snprintf(fields[0],256,"%s",client.session.name);snprintf(fields[1],256,"%s",client.session.login);if(!native_form("修改个人资料",labels,fields,2))goto abort;
        if(!*fields[0]||!*fields[1]||strchr(fields[0],'|')||strchr(fields[1],'|'))goto abort;snprintf(j->note,sizeof(j->note),"%s|%s",fields[0],fields[1]);
    }
    // 按按钮种类设置任务种类，并做各自的输入检查
    switch (id) {
    // 登录和注册：读出登录页 4 个输入框；学号和密码不能为空
    case ID_LOGIN:
    case ID_REGISTER:
        get_utf8(identity, j->identity, sizeof(j->identity));
        get_utf8(password, j->password, sizeof(j->password));
        get_utf8(nameEdit, j->name, sizeof(j->name));
        get_utf8(inviteEdit, j->invite, sizeof(j->invite));
        j->kind = id == ID_LOGIN ? JOB_LOGIN : JOB_REGISTER;
        if (!*j->identity || !*j->password) {
            tell("请填写学号 / 邮箱与密码。", MB_ICONINFORMATION);
            goto abort;
        }
        break;
    // 刷新：不需要额外输入
    case ID_REFRESH:
        j->kind = JOB_REFRESH;
        break;
    // 确认预约：至少选一台设备，结束时间必须晚于开始时间
    case ID_BOOK:
        j->kind = JOB_BOOK;
        if (!j->count || j->start >= j->end) {
            tell("请选择设备和有效时段。", MB_ICONINFORMATION);
            goto abort;
        }
        // 为 8~16 点每小时生成一个 GUID 作为防重复编号。网络超时后用户再点一次，云端看到相同编号就不会重复预约
        for (int i = 0; i < 9; i++) {
            GUID g;
            if (FAILED(CoCreateGuid(&g))) {
                tell("生成请求编号失败。", MB_ICONERROR);
                goto abort;
            }
            // 把 GUID 格式化成标准的 xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx 文本
            snprintf(j->keys[i], 64, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     (unsigned long)g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
                     g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
        }
        // 准备确认框文字，然后跳到 confirm 弹框
        snprintf(
            confirmation, sizeof(confirmation),
            "预约 %s %02d:00–%02d:00，共 %d 台设备？多小时会逐时提交；若后续冲突，会保留先前成功的小时。",
            j->date, j->start, j->end, j->count);
        goto confirm;
    // 签到、取消、完成维修、归档、停用：都要先在列表里选中一行
    case ID_ATTEND:
    case ID_CANCEL:
    case ID_REPAIR:
    case ID_ARCHIVE:
    case ID_DEACTIVATE:
        row = selected_row();
        if (!row) {
            tell("请先选择列表中的一行。", MB_ICONINFORMATION);
            goto abort;
        }
        // 深拷贝选中的记录：30 秒自动刷新会释放旧数据，如果 Job 只存原指针，后台线程就会访问已释放的内存（悬空指针）
        j->row = cJSON_Duplicate(row, 1);
        // 签到/签退：先在本地判断现在能签到还是签退、是不是自己的预约，给出友好提示（最终由云端再校验）
        if (id == ID_ATTEND) {
            j->kind = JOB_ATTEND;
            j->action = lab_attendance_action(row, now_ms());
            if (strcmp(lab_str(row, "user_id"), client.session.uid) || !j->action) {
                tell("仅可操作自己的预约：在预约小时内签到，使用中可签退。请刷新核实当前状态。",
                     MB_ICONINFORMATION);
                goto abort;
            }
            snprintf(confirmation, sizeof(confirmation), "确认%s设备 %02d（%s %02d:00）的预约？",
                     j->action == 1 ? "签到" : "签退", lab_int(row, "device_number"),
                     lab_str(row, "reservation_date"), lab_int(row, "start_hour"));
        // 其他四种：根据按钮选任务种类和确认文字
        } else {
            j->kind = id == ID_CANCEL    ? JOB_CANCEL
                      : id == ID_REPAIR  ? JOB_REPAIR
                      : id == ID_ARCHIVE ? JOB_ARCHIVE
                                         : JOB_DEACTIVATE;
            snprintf(confirmation, sizeof(confirmation), "确认%s所选记录？",
                     id == ID_CANCEL    ? "取消预约"
                     : id == ID_REPAIR  ? "完成维修"
                     : id == ID_ARCHIVE ? "归档故障"
                                        : "停用学生账号");
        }
        goto confirm;
    // 报告故障：用预约页选中的第一台设备
    case ID_REPORT:
        j->kind = JOB_REPORT;
        if (!j->count) {
            tell("请先到设备预约页选中一台设备。", MB_ICONINFORMATION);
            goto abort;
        }
        snprintf(confirmation, sizeof(confirmation), "确认报告设备 %02d 故障？", j->devices[0]);
        goto confirm;
    // 邀请码、统计报表：前面已用表单收好参数，直接执行
    case ID_INVITE:
        j->kind = JOB_INVITE;
        break;
    case ID_METRICS:
        j->kind = JOB_METRICS;
        break;
    // 修改资料：需要确认
    case ID_PROFILE:
        j->kind = JOB_PROFILE;
        snprintf(confirmation, sizeof(confirmation), "确认按说明框内容修改本人姓名和登录名？");
        goto confirm;
    // 智能推荐：直接执行
    case ID_SUGGEST:
        j->kind = JOB_SUGGEST;
        break;
    // 智能助手的"发送"和"选择此方案"：生成一个 GUID 作为本次请求编号；action 记录是否管理员（决定用哪种助手）
    case ID_AGENT_CONFIRM:
    case ID_AGENT:
        { GUID g; if(FAILED(CoCreateGuid(&g)))goto abort;
          snprintf(j->keys[0],64,"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            (unsigned long)g.Data1,g.Data2,g.Data3,g.Data4[0],g.Data4[1],g.Data4[2],g.Data4[3],g.Data4[4],g.Data4[5],g.Data4[6],g.Data4[7]); }
        j->kind = JOB_AGENT;
        j->action = admin();
        break;
    // 检查版本：已经在检查就不重复发起
    case ID_UPDATE:
        if (checkingUpdate)
            goto abort;
        j->kind = JOB_UPDATE;
        break;
    // 未知编号：直接放弃
    default:
        goto abort;
    }
    // 智能助手的额外准备：组装发给 AI 的请求
    if(j->kind==JOB_AGENT){
        lab_agent_bind(&agent,client.session.uid);
        // "发送"时说明框不能为空
        if(id==ID_AGENT&&!*j->note){tell("请输入你的需求。",MB_ICONINFORMATION);goto abort;}
        cJSON *confirmation=NULL;
        // "选择此方案"：按下拉框选中的方案生成确认信息（同一方案重试复用原来的防重复编号）；确认时不再附带用户文字
        if(id==ID_AGENT_CONFIRM){
            confirmation=lab_agent_confirmation(&agent,(int)SendMessageW(agentOptions,CB_GETCURSEL,0,0),j->keys[0]);
            if(!confirmation)goto abort;
            j->note[0]=0;
        }
        // 把用户输入、当前选择、对话历史打包成请求 JSON
        j->agentRequest=lab_agent_payload(&client.session,j->note,j->date,j->start,j->end,j->devices,(size_t)j->count,j->keys[0],agent.session,agent.history,confirmation);
        // 确认信息已被深拷贝进请求，这里释放原件
        cJSON_Delete(confirmation);if(!j->agentRequest)goto abort;
    }
    // 不需要确认的任务：直接启动
    start_job(j);
    return;
// confirm：弹出"是/否"确认框（默认按钮是"否"，防止误按回车）；选"是"才启动任务，选"否"会顺着走到 abort
confirm:
    {
        wchar_t *w = wide(confirmation);
        int answer = MessageBoxW(window, w, L"请确认", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        free(w);
        if (answer == IDYES) {
            start_job(j);
            return;
        }
    }
// abort：取消或检查失败都到这里：擦除密码、释放 Job 和其中的 JSON。cJSON_Delete(NULL) 什么都不做，所以不用逐个判断
abort:
    SecureZeroMemory(j->password, sizeof(j->password));
    cJSON_Delete(j->row);cJSON_Delete(j->agentRequest);cJSON_Delete(j->agentResult);
    free(j);
}

/* ===================== 模块 11：控件布局 layout =====================
 * 功能：根据窗口大小、当前页面和滚动位置，算出每个控件的位置并移过去。
 * 场景：窗口缩放、滚动、切换页面、登录前后都会调用。Win32 没有 CSS 那样的自动布局，坐标都要自己算。
 */
// 移动控件时减去 scrollY：页面向下滚多少，控件就往上移多少，看起来就像在滚动
static void place(HWND h,int x,int y,int w,int height){MoveWindow(h,x,y-scrollY,w,height,TRUE);}
static void layout(void) {
    // 算几条基准线：w 窗口宽；x 工作区左边（侧栏右侧）；cw 工作区宽；rail 右侧栏左边；labw 平面图卡片宽（至少 420）
    RECT r;GetClientRect(window,&r);int w=r.right,x=UI_SIDEBAR+32,cw=w-x-32,rail=w-324,labw=cw-308;
    if(labw<420)labw=420;
    // 设置滚动条：内容总高度按页面估算（预约页最长），每页高度等于窗口高度；再读回系统修正后的位置
    SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS,0,logged?(page==0?1160:page==1?1080:900):r.bottom,(UINT)r.bottom,scrollY,0};
    SetScrollInfo(window,SB_VERT,&si,TRUE);GetScrollInfo(window,SB_VERT,&si);scrollY=si.nPos;
    // 先按当前状态决定显示/隐藏
    visibility();
    // 左侧栏：底部是"我是谁"，上面是 5 个导航按钮（显示顺序：预约、智能助手、我的、故障、用户管理）
    MoveWindow(whoText,20,r.bottom-170,188,64,TRUE);
    const int order[]={0,4,1,2,3};for(int k=0;k<5;k++)MoveWindow(navigation[order[k]],16,152+k*50,196,44,TRUE);
    // 登录页：表单放在窗口右半边，宽度最多 430
    int fx=w/2+36,fw=w-fx-70; if(fw>430)fw=430;
    for(int i=0;i<4;i++){MoveWindow(labels[i],fx,170+i*70,fw,22,TRUE);}
    HWND inputs[]={identity,password,nameEdit,inviteEdit};for(int i=0;i<4;i++)MoveWindow(inputs[i],fx,195+i*70,fw,36,TRUE);
    MoveWindow(loginButton,fx,488,fw,40,TRUE);MoveWindow(registerButton,fx,540,fw,40,TRUE);MoveWindow(loginInfo,fx,600,fw,64,TRUE);
    // 工作区：帮助文字、日期框、开始/结束小时
    place(helpText,x,82,cw,28);
    place(dateEdit,x+labw-200,294,175,34);place(startEdit,rail+16,330,120,300);place(endEdit,rail+156,330,120,300);
    // 40 个设备按钮排成 5 行 8 列，第 4、5 列之间多留 32 像素当作"过道"，模拟实验室平面图
    for(int i=0;i<40;i++){int cell=(labw-48-7*8-32)/8;place(deviceButtons[i],x+24+(i%8)*(cell+8)+(i%8>=4?32:0),390+(i/8)*80,cell,68);}
    // 记录列表：预约页在平面图下方，其他页在上方
    place(list,x,page==0?880:page==1?800:160,page==0?labw:cw,page==1?220:page==0?220:480);
    place(scheduleOptions,rail+22,548,246,300);
    place(agentOptions,x,478,cw,300);
    place(usageList,x,438,cw,220);place(agentReply,x+24,170,cw-48,280);
    place(noteEdit,page==0?rail+22:x,page==0?866:page==1?1040:page==4?532:666,page==0?246:cw,36);
    place(profileFault,x+470,384,150,38);
    // 19 个操作按钮："刷新""检查版本""退出账号"固定在侧栏底部；其余只排可见的——预约页竖着放在右侧栏，其他页从左到右横排
    int slot=0;
    for(int i=0;i<19;i++){
      int id=actionIds[i];
      if(id==ID_LOGOUT||id==ID_UPDATE||id==ID_REFRESH){int sy=id==ID_REFRESH?r.bottom-104:id==ID_UPDATE?r.bottom-68:r.bottom-36;MoveWindow(actions[i],20,sy,188,30,TRUE);continue;}
      if(!(GetWindowLongPtrW(actions[i],GWL_STYLE)&WS_VISIBLE))continue;
      if(page==0){int sy=id==ID_BOOK?438:id==ID_SUGGEST?490:id==ID_CANCEL?862:id==ID_USE_CANDIDATE?592:814;place(actions[i],rail+22,sy,246,40);}
      else{int sy=page==1?384:page==4?594:720;place(actions[i],x+slot*150,sy,140,38);++slot;}
    }
    MoveWindow(statusText,x,r.bottom-26,cw,22,TRUE);InvalidateRect(window,NULL,TRUE);
}

/* ===================== 模块 12：绘制背景和卡片 paint_workspace =====================
 * 功能：画出页面上不属于控件的部分：背景、白色侧栏、圆角卡片、标题说明、"已选择几台设备"等统计。
 * 场景：窗口需要重画（WM_PAINT）时调用；预览截图时也用它画到内存里。
 * 要点：GDI 画刷、画笔是系统资源，Create 之后必须 DeleteObject，否则每次重画都泄漏一点，最终界面画不出来。
 *       SelectObject 会返回原来的对象，用完要先选回去，再删除自己创建的。
 */
static void box(HDC dc,int x,int y,int w,int h,COLORREF fill){ // 画一个圆角卡片：fill 填充色，边框用 BORDER
    HBRUSH b=CreateSolidBrush(fill);HPEN pen=CreatePen(PS_SOLID,1,BORDER);      // 创建填充画刷和边框画笔
    HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,pen);                      // 选进画布，同时记下原来的
    RoundRect(dc,x,y-scrollY,x+w,y+h-scrollY,32,32);                            // 画圆角矩形（随滚动上移）
    SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(pen);} // 换回原来的，再删除自己创建的
static void text(HDC dc,int x,int y,int w,int h,const char *value,HFONT f,COLORREF color){ // 在指定矩形里画一段 UTF-8 文字，太长自动换行
    RECT r={x,y-scrollY,x+w,y+h-scrollY};wchar_t *s=wide(value);
    SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color); // 字体、透明背景、颜色
    DrawTextW(dc,s,-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);free(s);}     // DT_NOPREFIX：不把 & 当成快捷键标记
static void paint_workspace(HDC dc){
    // 铺满背景色，并算出和 layout 相同的基准线
    RECT r;GetClientRect(window,&r);FillRect(dc,&r,background);int w=r.right,x=UI_SIDEBAR+32,cw=w-x-32,rail=w-324,labw=cw-308;
    // 未登录：画登录页——左边标语，右边白色卡片放登录表单
    if(!logged){box(dc,w/2,100,w/2-40,620,RGB(255,255,255));text(dc,64,180,w/2-100,40,"NJUPT · 实验室预约",bold,MUTED);text(dc,64,245,w/2-100,100,"把时间留给探索",heroFont,INK);text(dc,64,370,w/2-100,80,"选定设备，约好时间。\n设备、时段和你的记录，都在这里。",font,MUTED);text(dc,w/2+36,120,400,40,"欢迎回来",headingFont,INK);return;}
    // 已登录：左侧白色侧栏
    RECT side={0,0,UI_SIDEBAR,r.bottom};FillRect(dc,&side,(HBRUSH)GetStockObject(WHITE_BRUSH));
    /* 侧栏不跟随滚动：临时把 scrollY 设为 0 画图标和标题，画完再恢复 */
    int old=scrollY;scrollY=0;DrawIconEx(dc,20,26,LoadIconW(GetModuleHandleW(NULL),MAKEINTRESOURCEW(1)),28,28,0,NULL,DI_NORMAL);text(dc,58,30,160,40,"NJUPT 实验室预约",bold,INK);scrollY=old;
    // 工作区顶部：小标题和当前页面名称
    text(dc,x,22,cw,20,"NJUPT · 实验室预约",font,MUTED);text(dc,x,48,cw,40,ui_page_title(page),headingFont,INK);
    // 预约页：标语、"三步完成"卡片、实验室平面图卡片、预约时间卡片
    if(page==0){
      text(dc,x,208,cw,56,"把时间留给探索",heroFont,INK);box(dc,rail,112,292,130,RGB(252,252,251));text(dc,rail+22,132,248,28,"预约流程 · 三步完成",bold,INK);text(dc,rail+22,180,248,46,"01 选择设备   02 设置时间\n03 确认预约",font,MUTED);
      box(dc,x,272,labw,530,RGB(255,255,255));text(dc,x+24,296,250,28,"实验室平面图",bold,INK);text(dc,x+24,335,labw-48,28,"* 空闲    已预约    使用中    ! 故障    已选择",font,MUTED);
      box(dc,rail,272,292,286,RGB(255,255,255));text(dc,rail+22,294,248,28,"预约时间",bold,INK);
      int n=0;for(int i=0;i<40;i++)n+=selected[i];char selection[96];snprintf(selection,sizeof(selection),n?"已选择 %d 台设备":"尚未选择设备",n); // 统计选了几台设备
      box(dc,rail+22,382,246,38,RGB(239,240,255));text(dc,rail+34,391,218,24,selection,font,INK);
      int faulty=0;const cJSON *dv;cJSON_ArrayForEach(dv,model.devices)if(strcmp(lab_str(dv,"status"),"available"))++faulty; // 统计不可用的设备
      char count[100];snprintf(count,sizeof(count),"%d / 40\n台设备可预约",40-faulty);
      box(dc,rail,584,292,190,RGB(255,255,255));text(dc,rail+22,604,246,24,"实验室状态",bold,INK);text(dc,rail+22,646,246,100,count,headingFont,ACCENT); // "实验室状态"卡片
      text(dc,x,838,labw,30,"近期预约",bold,INK); // 平面图下方列表的标题
    // 我的页：身份卡片、今天的预约数、预约偏好（常用设备和时段）、签到区和记录区标题
    } else if(page==1){
      char today[160],prefs[256],identityText[256],date[16];date_string(date);ui_summary(&model,&client.session,date,today,sizeof(today),prefs,sizeof(prefs));snprintf(identityText,sizeof(identityText),"%s  ·  %s  ·  %s",client.session.name,admin()?"管理员":"学生",client.session.login);
      box(dc,x,124,cw,94,RGB(255,255,255));text(dc,x+24,148,cw-48,32,identityText,headingFont,INK);text(dc,x+24,188,cw-48,22,"个人资料 · 使用与网页相同的云端账号",font,MUTED);
      box(dc,x,238,cw/2-8,104,RGB(255,255,255));text(dc,x+22,254,cw/2-40,25,"今天的预约",bold,INK);text(dc,x+22,290,cw/2-40,44,today,font,MUTED);
      box(dc,x+cw/2+8,238,cw/2-8,104,RGB(255,255,255));text(dc,x+cw/2+30,254,cw/2-40,25,"预约偏好",bold,INK);text(dc,x+cw/2+30,290,cw/2-48,44,prefs,font,MUTED);
      text(dc,x,354,cw,28,"手动签到与签退",bold,INK);text(dc,x,748,cw,30,"我的预约",bold,INK);text(dc,x,780,cw,20,"完整记录与实际使用时间（北京时间）",font,MUTED);
    // 助手页：对话卡片和输入提示；故障页和用户管理页：一行说明
    } else if(page==4){box(dc,x,124,cw,350,RGB(255,255,255));text(dc,x+24,142,cw-48,24,"智能助手",bold,INK);text(dc,x,464,cw,24,"输入设备、日期或时段需求，获取预约建议。",font,MUTED);}
    else text(dc,x,122,cw,24,page==2?"故障说明和维修进度":"用户和账号管理",font,MUTED);
    // 预览模式下在页面底部注明"示例数据"
    if(preview)text(dc,x,1112,cw,24,"界面预览 · 示例数据 · 未连接真实账号",font,MUTED);
}
/* ===================== 模块 13：预览截图 =====================
 * 功能：用 --preview=xxx --preview-output=路径 启动时，载入示例数据，2.5 秒后把窗口画成 BMP 图片保存并退出。
 * 场景：开发时自动检查界面效果，不需要真实账号。只截本程序自己的窗口，不读取其他程序的画面。
 */
// 把一个可见的子控件画到内存画布上对应的位置
static BOOL CALLBACK preview_child(HWND child,LPARAM value) {
 // 只处理主窗口的直接子控件，且必须可见
 if(GetParent(child)!=window || !(GetWindowLongPtrW(child,GWL_STYLE)&WS_VISIBLE))return TRUE;
 // 算出子控件在主窗口里的坐标
 HDC dc=(HDC)value;RECT rect;GetWindowRect(child,&rect);POINT pos={rect.left,rect.top};ScreenToClient(window,&pos);
 // 临时把画布原点移到控件左上角，让控件以为自己在 (0,0) 作画
 int saved=SaveDC(dc);SetViewportOrgEx(dc,pos.x,pos.y,NULL);
 // WM_PRINT：请控件把自己画到我们给的画布上；画完恢复画布原点
 SendMessageW(child,WM_PRINT,(WPARAM)dc,PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);return TRUE;
}
// 整窗截图：先在内存画布上画背景，再逐个叠上子控件，最后写成 BMP 文件
static int preview_capture(HWND h) {
 RECT r;GetClientRect(h,&r);int w=r.right,height=r.bottom;HDC dc=GetDC(h),mem=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,w,height);HGDIOBJ old=SelectObject(mem,bitmap);
 paint_workspace(mem);EnumChildWindows(h,preview_child,(LPARAM)mem);SelectObject(mem,old);
 // BMP 图像信息：32 位色；高度写成负数表示像素从上往下存放
 BITMAPINFO info={0};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 // 把内存画布的像素取到 pixels 数组
 DWORD bytes=(DWORD)(w*height*4);void *pixels=calloc(1,bytes);int ok=pixels&&GetDIBits(dc,bitmap,0,height,pixels,&info,DIB_RGB_COLORS);FILE *f=NULL;
 if(ok)f=_wfopen(previewOutput,L"wb");
 // 写文件：文件头（"BM" 标记 0x4d42）→ 图像信息头 → 像素数据，三次写入都成功才算成功
 if(f){BITMAPFILEHEADER file={0};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+bytes;ok=fwrite(&file,sizeof(file),1,f)==1&&fwrite(&info.bmiHeader,sizeof(BITMAPINFOHEADER),1,f)==1&&fwrite(pixels,bytes,1,f)==1;fclose(f);}else ok=0;
 // 释放内存和所有 GDI 资源
 free(pixels);DeleteObject(bitmap);DeleteDC(mem);ReleaseDC(h,dc);return ok;
}

/* ===================== 模块 14：主窗口消息处理 wndproc =====================
 * 功能：Windows 把发生在窗口上的一切（创建、点击、重画、滚动、定时器、后台结果……）都以"消息"送到这里，
 *       switch(msg) 按消息种类分别处理。返回 0 表示"已处理"，没处理的交给 DefWindowProcW 做默认处理。
 */
static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    // 【WM_CREATE】窗口刚创建：建好字体和所有控件，然后启动定时刷新和版本检查
    case WM_CREATE: {
        window = h;
        // 字体：微软雅黑 15 像素，再派生粗体、28 像素标题、34 像素大标语
        LOGFONTW lf = {0};
        lf.lfHeight = -15;
        wcscpy(lf.lfFaceName, L"Microsoft YaHei");
        font = CreateFontIndirectW(&lf);
        lf.lfWeight = FW_BOLD;
        bold = CreateFontIndirectW(&lf);
        background = CreateSolidBrush(BG);
        lf.lfHeight=-28;headingFont=CreateFontIndirectW(&lf);lf.lfHeight=-34;heroFont=CreateFontIndirectW(&lf);
        // 侧栏 5 个导航按钮（编号 600~604）和"我的故障记录"按钮（605，点击后跳到故障页）
        const wchar_t *navNames[]={L"预约",L"我的",L"故障记录",L"用户管理",L"智能助手"};
        for(int i=0;i<5;i++)navigation[i]=control(L"BUTTON",navNames[i],BS_OWNERDRAW|WS_TABSTOP,600+i,16,152+i*50,196,44);
        profileFault=control(L"BUTTON",L"我的故障记录",BS_OWNERDRAW|WS_TABSTOP,605,260,340,150,38);
        // 两个方案下拉框、助手对话显示框（只读多行）、左下角"我是谁"
        scheduleOptions=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,0,940,548,246,300);
        agentOptions=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,0,260,478,800,300);
        agentReply=control(L"EDIT",L"告诉我你的预约需求。",ES_MULTILINE|ES_READONLY|WS_VSCROLL,0,260,170,800,280);
        whoText = control(L"STATIC", L"", 0, 0, 500, 20, 570, 26);
        // 登录页：4 个标题 + 4 个输入框（密码框用 ES_PASSWORD 显示成圆点）+ 登录/注册按钮 + 说明文字
        const wchar_t *texts[] = {L"学号 / 邮箱", L"密码", L"姓名（注册）", L"管理员邀请码（可选）"};
        for (int i = 0; i < 4; i++)
            labels[i] = control(L"STATIC", texts[i], 0, 0, 155, 120 + i * 65, 280, 24);
        identity = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 120, 370, 30);
        password = control(L"EDIT", L"", WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 185,
                           370, 30);
        nameEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 250, 370, 30);
        inviteEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 315, 370, 30);
        loginButton = control(L"BUTTON", L"登录并继续", BS_OWNERDRAW | WS_TABSTOP, ID_LOGIN, 450, 380, 175, 38);
        registerButton = control(L"BUTTON", L"创建账号", BS_OWNERDRAW | WS_TABSTOP, ID_REGISTER, 645, 380, 175, 38);
        loginInfo = control(L"STATIC",
                            L"使用与网页相同的云端账号。\n学生使用学号；管理员使用邮箱。",
                            0, 0, 155, 460, 800, 80);
        // 旧版标签页（现在隐藏不用，保留是因为通知处理里还在引用它）
        tabs = control(WC_TABCONTROLW, L"", WS_TABSTOP, ID_TAB, 24, 54, 440, 32);
        const wchar_t *tabNames[] = {L"设备预约", L"我的预约", L"故障管理", L"账号管理"};
        for (int i = 0; i < 4; i++) {
            TCITEMW t = {0};
            t.mask = TCIF_TEXT;
            t.pszText = (LPWSTR)tabNames[i];
            SendMessageW(tabs, TCM_INSERTITEMW, (WPARAM)i, (LPARAM)&t);
        }
        // 日期框：显示成 yyyy-MM-dd，默认北京时间今天
        dateEdit =
            control(DATETIMEPICK_CLASSW, L"", DTS_SHORTDATEFORMAT | WS_TABSTOP, ID_DATE, 490, 55, 180, 30);
        DateTime_SetFormat(dateEdit, L"yyyy-MM-dd");
        today_beijing();
        // 开始/结束小时下拉框：开始 08:00~16:00，结束 09:00~17:00，默认都选第一项
        startEdit = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ID_START, 685, 55,
                            130, 300);
        endEdit =
            control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ID_END, 830, 55, 130, 300);
        for (int i = 0; i < 9; i++) {
            wchar_t b[50];
            swprintf(b, 50, L"开始 %02d:00", i + 8);
            SendMessageW(startEdit, CB_ADDSTRING, 0, (LPARAM)b);
            swprintf(b, 50, L"结束 %02d:00", i + 9);
            SendMessageW(endEdit, CB_ADDSTRING, 0, (LPARAM)b);
        }
        SendMessageW(startEdit, CB_SETCURSEL, 0, 0);
        SendMessageW(endEdit, CB_SETCURSEL, 0, 0);
        // 帮助文字、40 个设备按钮（BS_OWNERDRAW：外观由我们在 WM_DRAWITEM 里自己画）
        helpText = control(L"STATIC", L"", 0, 0, 24, 91, 1040, 28);
        for (int i = 0; i < 40; i++)
            deviceButtons[i] = control(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, ID_FIRST_DEVICE + i,
                                       24 + (i % 10) * 104, 127 + (i / 10) * 44, 95, 35);
        // 记录列表和"我的"页的使用记录表（编号 210）：整行选中、双缓冲防闪烁、白底
        list = control(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | WS_BORDER | WS_TABSTOP, ID_LIST, 24,
                       315, 1040, 230);
        ListView_SetExtendedListViewStyle(list,
                                          LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(list,RGB(255,255,255));ListView_SetTextBkColor(list,RGB(255,255,255));ListView_SetTextColor(list,INK);
        usageList=control(WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|WS_BORDER|WS_TABSTOP,210,260,438,800,220);
        ListView_SetExtendedListViewStyle(usageList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(usageList,RGB(255,255,255));ListView_SetTextBkColor(usageList,RGB(255,255,255));ListView_SetTextColor(usageList,INK);
        // 说明框（多用途输入框，空的时候显示灰色提示文字）、19 个操作按钮、底部状态栏
        noteEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 24, 560, 1040, 28);
        SendMessageW(noteEdit, EM_SETCUEBANNER, TRUE,
                     (LPARAM)L"故障 / 维修说明或智能助手问题；修改资料：姓名|登录名");
        for (int i = 0; i < 19; i++)
            actions[i] = control(L"BUTTON", actionNames[i], BS_OWNERDRAW|WS_TABSTOP, actionIds[i], 24, 600, 135, 30);
        statusText = control(L"STATIC", L"欢迎使用。所有预约时段按北京时间计算。", 0, 0, 24, 650, 1040, 24);
        // 预览模式：载入示例数据，直接进入指定页面；正常模式：每 30 秒触发一次定时器自动刷新
        if(preview){SetWindowTextW(statusText,L"界面预览 · 示例数据 · 未连接真实账号");ui_preview_model(&client,&model);logged=preview!=1;page=preview==3?1:preview==4?4:0;render();}
        else SetTimer(h,1,30000,NULL);
        layout();
        // 启动时在后台检查一次新版本
        Job *j = calloc(1, sizeof(*j));
        if (j && !preview) {
            j->kind = JOB_UPDATE;
            start_job(j);
        }else free(j);
        return 0;
    }
    // 【WM_PRINTCLIENT / WM_PAINT】需要重画时，画背景和卡片（控件会自己画自己）
    case WM_PRINTCLIENT:paint_workspace((HDC)wp);return 0;
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);paint_workspace(dc);EndPaint(h,&ps);return 0;}
    // 【WM_MOUSEWHEEL】滚轮每格滚动 80 像素，不能滚到顶部以上；然后重新布局
    case WM_MOUSEWHEEL:scrollY-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*80;if(scrollY<0)scrollY=0;layout();return 0;
    // 【WM_VSCROLL】拖动或点击滚动条：拖动时跟随滑块，点箭头每次 40 像素，点空白处翻一页
    case WM_VSCROLL:{SCROLLINFO si={0};si.cbSize=sizeof(si);si.fMask=SIF_ALL;GetScrollInfo(h,SB_VERT,&si);int code=LOWORD(wp);if(code==SB_THUMBTRACK)scrollY=si.nTrackPos;else if(code==SB_LINEUP)scrollY-=40;else if(code==SB_LINEDOWN)scrollY+=40;else if(code==SB_PAGEUP)scrollY-=si.nPage;else if(code==SB_PAGEDOWN)scrollY+=si.nPage;if(scrollY<0)scrollY=0;layout();return 0;}
    // 【WM_SIZE】窗口大小改变：重新布局（list 还没创建时跳过）
    case WM_SIZE:
        if (list)
            layout();
        return 0;
    // 【WM_GETMINMAXINFO】限制窗口最小 1200×800，太小布局会挤在一起
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lp)->ptMinTrackSize.x = 1200;
        ((MINMAXINFO *)lp)->ptMinTrackSize.y = 800;
        return 0;
    // 【WM_COMMAND】按钮被点击或下拉框选择变化
    case WM_COMMAND: {
        int id = LOWORD(wp);
        // 侧栏导航：切换页面。605 是"我的故障记录"，跳到故障页；学生不能进用户管理页
        if(id>=600&&id<=605){if(busy)return 0;page=id==605?2:id-600;if(page==3&&!admin())page=1;scrollY=0;render();layout();return 0;}
        // 预览模式只允许点设备按钮
        if(preview&&!(id>=ID_FIRST_DEVICE&&id<ID_FIRST_DEVICE+40))return 0;
        // 设备按钮：在选中/未选中之间切换，并重画该按钮和窗口（右侧"已选择 N 台"要跟着变）
        if (id >= ID_FIRST_DEVICE && id < ID_FIRST_DEVICE + 40) {
            int i = id - ID_FIRST_DEVICE;
            selected[i] = !selected[i];
            InvalidateRect(deviceButtons[i], NULL, TRUE);
            InvalidateRect(window,NULL,TRUE);
        // 改了开始/结束小时：重画，设备颜色要反映新时段是否被占用
        } else if (id == ID_START || id == ID_END) {
            if (HIWORD(wp) == CBN_SELCHANGE)
                render();
        // 其他按钮：交给 command 处理
        } else if (HIWORD(wp) == BN_CLICKED)
            command(id);
        return 0;
    }
    // 【WM_NOTIFY】列表、标签页、日期框这类通用控件用 WM_NOTIFY 报告变化
    case WM_NOTIFY: {
        NMHDR *n = (NMHDR *)lp;
        // 用户在列表里选中了一行：记下这条记录的 id 和来自哪张表。render 重填期间（rendering=1）产生的通知忽略
        if(!rendering&&(n->idFrom==ID_LIST||n->idFrom==210)&&n->code==LVN_ITEMCHANGED){
            NMLISTVIEW *v=(NMLISTVIEW *)lp;if((v->uNewState&LVIS_SELECTED)&&v->iItem>=0&&v->iItem<listedCount){selectedTable=n->idFrom==210;snprintf(selectedId,sizeof(selectedId),"%s",lab_str(listed[v->iItem],"id"));}
        }
        // 旧标签页切换（已隐藏，基本不会触发）
        if (n->idFrom == ID_TAB && n->code == TCN_SELCHANGE) {
            page = TabCtrl_GetCurSel(tabs);
            if (page == 3 && !admin()) {
                page = 1;
                TabCtrl_SetCurSel(tabs, page);
                tell("账号管理需要管理员权限。", MB_ICONINFORMATION);
            }
            render();
            visibility();
        }
        // 日期改了：重画列表和设备颜色
        if (n->idFrom == ID_DATE && n->code == DTN_DATETIMECHANGE && logged)
            render();
        return 0;
    }
    // 【RESULT_MESSAGE】后台任务完成，worker 寄回了 Job。这里运行在界面线程，可以安全地改全局数据和控件
    case RESULT_MESSAGE: {
        Job *j = (Job *)lp;
        // 版本检查的结果：有新版本就询问是否用浏览器打开下载页
        if (j->kind == JOB_UPDATE) {
            checkingUpdate = 0;
            if (j->ok) {
                wchar_t *w = wide(j->message);
                if (MessageBoxW(h, w, L"桌面版本更新", MB_YESNO | MB_ICONINFORMATION) == IDYES)
                    ShellExecuteW(h, L"open",
                                  L"https://alexis-zhang0503.github.io/intelligent-lab-booking-system/"
                                  L"downloads/LabReservation-Windows-x64.zip",
                                  NULL, NULL, SW_SHOWNORMAL);
                free(w);
            }
            if (!busy)
                SetWindowTextW(statusText, L"原生 C 3.1.0 · 云端数据每 30 秒刷新");
        // 其他任务的结果
        } else {
            // 解除忙碌；采用后台线程更新过的登录状态（令牌可能被自动刷新过）
            busy = 0;
            client = j->client;
            // 智能推荐：保存一份候选方案的深拷贝，供下拉框使用
            if(j->kind==JOB_SUGGEST&&j->agentResult){cJSON_Delete(scheduleCandidates);scheduleCandidates=cJSON_Duplicate(j->agentResult,1);}
            // 智能助手：确认这个回复属于当前账号（防止切换账号后旧请求的回复才到），再记进对话历史
            if(j->kind==JOB_AGENT && j->agentResult && !strcmp(client.session.uid,agent.owner))lab_agent_accept(&agent,j->client.session.uid,j->note,j->agentResult);
            // 数据重新加载成功：释放旧数据，换上新数据。model = j->model 只复制了 5 个指针（浅拷贝），
            // 所以紧接着把 Job 里的指针清零，表示所有权已转给全局 model；否则后面释放 Job 时会把同一份数据再释放一次
            if (j->loaded) {
                lab_model_free(&model);
                model = j->model;
                memset(&j->model, 0, sizeof(j->model));
                logged = 1;
                // 刚登录或注册：回到预约页并清空设备选择
                if (j->kind == JOB_LOGIN || j->kind == JOB_REGISTER) {
                    page = 0;
                    TabCtrl_SetCurSel(tabs, 0);
                    memset(selected, 0, sizeof(selected));
                }
                // 清空密码框和邀请码框
                SetWindowTextW(password, L"");
                SetWindowTextW(inviteEdit, L"");
                render();
            }
            // 令牌没了（登录失败、账号被停用、刷新令牌失效）：退回未登录状态
            if (!*client.session.access) {
                logged = 0;
                lab_model_free(&model);lab_agent_clear(&agent);
            }
            // 显示结果：助手回复显示在对话框里；统计报表用报表窗口；其他有提示的弹窗；
            // 成功但没有提示的给一句通用成功提示（刷新和登录不打扰用户）
            if(*j->message&&j->kind==JOB_AGENT&&page==4&&j->agentResult){char transcript[16384];lab_agent_transcript(&agent,transcript,sizeof(transcript));set_utf8(agentReply,transcript);}
            else if(j->kind==JOB_METRICS&&j->ok)native_report("实验室使用情况",j->message);
            else if (*j->message)
                tell(j->message, j->ok ? MB_ICONINFORMATION : MB_ICONERROR);
            else if (j->ok && j->kind != JOB_REFRESH && j->kind != JOB_LOGIN)
                tell("操作成功，已刷新云端数据。", MB_ICONINFORMATION);
            // 底部状态栏
            SetWindowTextW(statusText, j->loaded ? L"已同步云端数据 · 预约时段为北京时间 · 每 30 秒自动刷新"
                                                 : L"请求未完成，请查看提示并刷新核实。请勿反复提交。");
        }
        // 释放 Job：剩下的数据、令牌副本（安全擦除）、JSON，最后 free Job 本身
        lab_model_free(&j->model);
        lab_session_clear(&j->client.session);
        cJSON_Delete(j->row);cJSON_Delete(j->agentRequest);cJSON_Delete(j->agentResult);
        free(j);
        // 按新状态重新布局
        layout();
        return 0;
    }
    // 【WM_TIMER】定时器 2：预览截图，成功就关窗退出；定时器 1：每 30 秒，若已登录且空闲就自动刷新
    case WM_TIMER:
        if(wp==2){KillTimer(h,2);if(!preview_capture(h))PostQuitMessage(2);else DestroyWindow(h);return 0;}
        if (logged && !busy)
            command(ID_REFRESH);
        return 0;
    // 【WM_DRAWITEM】自绘按钮：设备按钮按状态上色，其他按钮画成圆角样式
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT *)lp;
        // 算出是第几台设备；不在 0~39 就是普通按钮
        int i = (int)d->CtlID - ID_FIRST_DEVICE;
        if(i<0||i>=40){
            // nav 是否侧栏导航；primary 是否主要按钮（确认预约、登录、发送，紫底白字）；active 是否当前所在页面的导航
            int id=(int)d->CtlID,nav=id>=600&&id<=604,primary=id==ID_BOOK||id==ID_LOGIN||id==ID_AGENT,active=nav&&page==id-600;
            COLORREF bg=primary?ACCENT:active?RGB(242,242,239):RGB(255,255,255);
            // 画圆角背景，用完立即恢复并删除画刷、画笔
            HBRUSH brush=CreateSolidBrush(bg);HPEN pen=CreatePen(PS_SOLID,1,nav?bg:BORDER);HGDIOBJ oldb=SelectObject(d->hDC,brush),oldp=SelectObject(d->hDC,pen);
            RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,16,16);SelectObject(d->hDC,oldb);SelectObject(d->hDC,oldp);DeleteObject(brush);DeleteObject(pen);
            // 写按钮文字（导航左对齐，其他居中）；当前页的导航左边画一条紫色竖线；有键盘焦点时画虚线框
            RECT rect=d->rcItem;if(nav)rect.left+=14;SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,primary?RGB(255,255,255):INK);SelectObject(d->hDC,active?bold:font);wchar_t label[128];GetWindowTextW(d->hwndItem,label,128);DrawTextW(d->hDC,label,-1,&rect,(nav?DT_LEFT:DT_CENTER)|DT_VCENTER|DT_SINGLELINE);if(active){RECT line={d->rcItem.left,d->rcItem.top+9,d->rcItem.left+3,d->rcItem.bottom-9};HBRUSH a=CreateSolidBrush(ACCENT);FillRect(d->hDC,&line,a);DeleteObject(a);}if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&rect);return TRUE;
        }
        // 设备按钮：判断这台设备的三种状态
        int occupied = 0, fault = 0, live = 0;
        const cJSON *r;
        char date[16];
        date_string(date);
        int start = 8 + (int)SendMessageW(startEdit, CB_GETCURSEL, 0, 0),
            end = 9 + (int)SendMessageW(endEdit, CB_GETCURSEL, 0, 0);
        // 故障：设备表里的状态不是 available
        cJSON_ArrayForEach(r, model.devices) if (lab_int(r, "number") == i + 1 &&
                                                 strcmp(lab_str(r, "status"), "available")) fault = 1;
        // 已预约：所选日期、所选时段内有有效预约
        cJSON_ArrayForEach(r, model.bookings) if (lab_int(r, "device_number") == i + 1 &&
                                                  !strcmp(lab_str(r, "reservation_date"), date) &&
                                                  lab_int(r, "start_hour") >= start &&
                                                  lab_int(r, "start_hour") < end &&
                                                  (!strcmp(lab_str(r, "status"), "reserved") ||
                                                   !strcmp(lab_str(r, "status"), "confirmed") ||
                                                   !strcmp(lab_str(r, "status"), "in_use"))) occupied = 1;
        // 使用中：此刻正有人签到使用
        cJSON_ArrayForEach(r,model.bookings)if(lab_int(r,"device_number")==i+1&&lab_booking_is_live(r,now_ms()))live=1;
        // 颜色优先级：已选择 > 故障 > 使用中 > 已预约 > 空闲
        COLORREF color = selected[i] ? RGB(239,240,255)
                         : fault     ? RGB(252,240,239)
                         : live      ? RGB(232,246,250)
                         : occupied  ? RGB(242,242,239)
                                     : RGB(252,252,251);
        // 画圆角底色，选中时边框加粗变紫；画完删除 GDI 对象
        HBRUSH brush = CreateSolidBrush(color);
        HPEN pen=CreatePen(PS_SOLID,selected[i]?2:1,selected[i]?ACCENT:fault?RGB(233,194,191):BORDER);
        HGDIOBJ ob=SelectObject(d->hDC,brush),op=SelectObject(d->hDC,pen);RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,16,16);SelectObject(d->hDC,ob);SelectObject(d->hDC,op);DeleteObject(pen);
        DeleteObject(brush);
        // 两行文字：设备编号 + 状态
        SetBkMode(d->hDC, TRANSPARENT);
        SetTextColor(d->hDC, fault ? RGB(189,82,82) : INK);
        SelectObject(d->hDC, bold);
        wchar_t b[40];
        swprintf(b, 40, L"%02d\n%ls", i + 1, fault ? L"! 故障" : live ? L"使用中" : occupied ? L"已预约" : L"* 空闲");
        RECT seatText=d->rcItem;seatText.top+=10;DrawTextW(d->hDC, b, -1, &seatText, DT_CENTER | DT_WORDBREAK);
        if (d->itemState & ODS_FOCUS)
            DrawFocusRect(d->hDC, &d->rcItem);
        return TRUE;
    }
    // 【WM_CTLCOLOREDIT / WM_CTLCOLORSTATIC】系统画输入框、文字标签前会问我们要颜色：
    // 输入框白底；帮助文字和状态栏用背景灰，其他标签用白色
    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wp,RGB(255,255,255));SetTextColor((HDC)wp,INK);return (LRESULT)GetStockObject(WHITE_BRUSH);
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wp,TRANSPARENT);
        SetBkColor((HDC)wp, BG);
        SetTextColor((HDC)wp, INK);
        return (LRESULT)((HWND)lp==helpText||(HWND)lp==statusText?background:GetStockObject(WHITE_BRUSH));
    // 【WM_ERASEBKGND】擦背景时用自己的背景色；返回 1 表示已处理，避免系统先刷白再重画造成闪烁
    case WM_ERASEBKGND: {
        RECT r;
        GetClientRect(h, &r);
        FillRect((HDC)wp, &r, background);
        return 1;
    }
    // 【WM_DESTROY】窗口关闭：停定时器，释放数据，擦除令牌，结束消息循环
    case WM_DESTROY:
        KillTimer(h, 1);
        lab_model_free(&model);
        lab_session_clear(&client.session);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

/* ===================== 模块 15：程序入口 wWinMain =====================
 * 功能：程序从这里开始：读命令行参数 → 初始化 → 注册并创建主窗口 → 进入消息循环 → 退出时释放资源。
 * 写法：Windows 图形程序的入口是 wWinMain 而不是 main；开头的 w 表示命令行参数是宽字符。
 */
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR commandLine, int show) {
    // 没用到的参数，强转 void 避免编译器警告
    (void)previous;
    // 解析预览参数：--preview=login/booking/profile/agent 选择预览哪个页面
    if(wcsstr(commandLine,L"--preview=login"))preview=1;
    else if(wcsstr(commandLine,L"--preview=booking"))preview=2;
    else if(wcsstr(commandLine,L"--preview=profile"))preview=3;
    else if(wcsstr(commandLine,L"--preview=agent"))preview=4;
    // --preview-output= 后面是截图保存路径（17 是这个前缀的长度）
    LPWSTR output=wcsstr(commandLine,L"--preview-output=");if(preview&&output){output+=17;wcsncpy(previewOutput,output,MAX_PATH-1);}
    // 初始化通用控件库（列表、日期框、标签页），否则这些控件创建不出来
    INITCOMMONCONTROLSEX ic = {sizeof(ic), ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_TAB_CLASSES};
    InitCommonControlsEx(&ic);
    // 初始化 COM，生成 GUID 要用
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    // 把 Windows 联网函数装进业务层客户端：client.c 之后通过这个函数指针发请求，自己不依赖 Windows
    client.transport = lab_win_http;
    // 注册主窗口类：指定消息处理函数 wndproc、图标、鼠标指针
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wndproc;
    wc.hInstance = instance;
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = L"NJUPTNativeCLabReservation";
    if (!RegisterClassExW(&wc))
        return 1;
    // 创建 1280×940 的主窗口（带垂直滚动条）；创建过程中会先收到 WM_CREATE
    HWND h = CreateWindowExW(0, wc.lpszClassName, L"实验室预约 · 原生 C", WS_OVERLAPPEDWINDOW|WS_VSCROLL, CW_USEDEFAULT,
                             CW_USEDEFAULT, 1280, 940, NULL, NULL, instance, NULL);
    if (!h)
        return 1;
    ShowWindow(h, show);
    // 预览且需要截图：2.5 秒后触发截图定时器（留时间让界面画完）
    if(preview&&*previewOutput)SetTimer(h,2,2500,NULL);
    // 消息循环：GetMessageW 等待下一条消息，收到退出消息时返回 0，循环结束。IsDialogMessageW 让 Tab 键能在控件间切换
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(h, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    // 退出前释放字体、画刷和 COM
    DeleteObject(font);
    DeleteObject(bold);
    DeleteObject(headingFont);DeleteObject(heroFont);
    DeleteObject(background);
    CoUninitialize();
    // 返回值来自 PostQuitMessage 的参数
    return (int)msg.wParam;
}

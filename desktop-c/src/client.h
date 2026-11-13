/*
 * client.h —— 业务层的"说明书"
 *
 * 这个文件回答一个问题：桌面程序要和云端打交道，需要哪些数据、能做哪些事？
 *   - 数据：登录后的身份（LabSession）、从云端拉下来的全部表格（LabModel）、
 *           一个能发请求的客户端（LabClient）、AI 助手的对话状态（LabAgentState）
 *   - 能做的事：登录注册、加载数据、预约、签到签退、问 AI 助手、导出表格、看统计
 *
 * 这里只有"声明"，具体怎么做写在 client.c。界面（main.c）只通过这里列出的函数使用业务层。
 *
 * 贯穿全文件的三条约定：
 *   1. 返回 cJSON * 的函数：调用者用完要 cJSON_Delete；返回 const cJSON * 的：只是借看，不能删。
 *   2. 返回 0 / NULL / -1 表示失败，失败原因写在 LabClient.error 里。
 *   3. 凡是往 char * 里写字符串的函数，后面都跟一个 size_t 容量参数，保证不写越界。
 */

/* ===================== 模块 1：头文件保护与依赖 =====================
 * 功能：保证这个头文件被包含多少次都只生效一次，并引入要用到的类型。
 * 场景：main.c 包含了 client.h，又包含了 win_http.h，而 win_http.h 里也包含 client.h，
 *       没有保护的话 LabSession 等类型会被定义两次，编译报错。
 */
#ifndef LAB_NATIVE_CLIENT_H          // 第一次包含时这个宏还不存在，于是往下编译
#define LAB_NATIVE_CLIENT_H          // 立刻定义它，第二次包含时就会整段跳过
#include "../vendor/cJSON.h"         // 第三方 JSON 库：云端收发的数据都是 JSON
#include <stddef.h>                  // 提供 size_t：表示缓冲区容量
#include <stdio.h>                   // 提供 FILE：导出 CSV 时写文件
#include <stdint.h>                  // 提供 int64_t：毫秒时间戳超过 32 位 int 的范围

/* ===================== 模块 2：程序配置 =====================
 * 功能：集中写死程序要连接的地址和当前版本号，改地址或发新版只需改这里。
 */
#define LAB_NATIVE_VERSION "3.1.0"                                   // 当前版本，启动时和官网上的版本比较，判断是否提示更新
#define LAB_API_HOST "ikfqcappastopzjbncea.supabase.co"              // 云端数据库服务器，所有登录、预约请求都发到这里
#define LAB_PUBLIC_KEY "sb_publishable_5Q5xIYHsAeEfO195kJ7jNg_Z5uBwxkV" // 公开的项目标识，每个请求都要带上；它本身没有数据权限
#define LAB_SITE_HOST "alexis-zhang0503.github.io"                   // 项目官网，用来下载最新版本信息
#define LAB_SITE_BASE "/intelligent-lab-booking-system/"             // 官网下的项目路径

/* ===================== 模块 3：网络传输接口 =====================
 * 功能：规定"发一个 HTTPS 请求"的函数必须长什么样，但不规定具体怎么发。
 * 场景：Windows 上由 win_http.c 的 lab_win_http 实现；测试时换成一个假函数，
 *       不联网也能检查业务逻辑。业务层因此完全不依赖 Windows。
 * 写法：这是一个"函数指针类型"。从名字往外读：LabTransport 是指针 → 指向一个函数 →
 *       这个函数接收下面 10 个参数 → 返回 char *（响应内容，用完要 free，失败返回 NULL）。
 */
typedef char *(*LabTransport)(void *,        // 1. 上下文：留给具体实现自定义使用
                              const char *,  // 2. 服务器地址
                              const char *,  // 3. 请求方法 "GET" / "POST"
                              const char *,  // 4. 路径，例如 /rest/v1/rpc/v2_check_in
                              const char *,  // 5. 公开项目标识 apikey
                              const char *,  // 6. 用户访问令牌（证明"我是谁"）
                              const char *, int *, char *, size_t); // 7. 请求体 JSON  8. 写回 HTTP 状态码  9. 写回错误信息  10. 错误缓冲区大小

/* ===================== 模块 4：核心数据结构 =====================
 * 功能：定义程序运行时要记住的三类东西——"我是谁"、"云端有什么数据"、"用什么发请求"。
 */

/* 4.1 LabSession：登录成功后的身份信息。
 * 全部用定长数组而不是指针，所以可以用 = 整体复制一份（交给后台线程用），
 * 也不需要 free，退出登录时整块清零即可。 */
typedef struct {
    char access[8192],   // 访问令牌：每个请求都带上它，服务器据此识别用户；约 1 小时后过期
         refresh[4096],  // 刷新令牌：访问令牌过期后，用它换一个新的，用户不用重新登录
         uid[64],        // 用户唯一 ID：判断"这条预约是不是我的"
         name[128],      // 姓名：显示在界面左下角
         login[128],     // 学号或管理员邮箱
         role[16];       // 角色 "student" 或 "admin"：决定界面上显示哪些管理按钮
} LabSession;

/* 4.2 LabModel：从云端拉下来的全部数据，界面上看到的列表都来自这里。
 * 每个字段是一个 cJSON 数组，对应云端的一张表。这里是指针，复制结构体只复制地址（浅拷贝）。 */
typedef struct {
    cJSON *devices,   // 设备表：40 台设备的编号和状态（可用 / 故障）
          *bookings,  // 预约表：谁在哪天几点约了哪台设备
          *profiles,  // 用户表：姓名、学号、角色、是否被停用
          *issues,    // 故障表：设备故障报告和维修记录
          *usage;     // 使用记录表：每条预约的实际签到、签退时间
} LabModel;

/* 4.3 LabClient：一个"客户端对象"。几乎所有业务函数的第一个参数都是它，
 * 相当于面向对象里的 this：函数通过它拿到身份、发出请求、写回错误。 */
typedef struct {
    LabTransport transport; // 用哪个函数发网络请求（Windows 上是 lab_win_http）
    void *context;          // 原样传给 transport 的第 1 个参数
    LabSession session;     // 当前登录身份（结构体里直接嵌着一份，不是指针）
    char error[1024];       // 最近一次失败的中文原因，界面弹窗显示的就是它
    int status;             // 最近一次请求的 HTTP 状态码，例如 200 成功、401 令牌过期
} LabClient;

/* ===================== 模块 5：基础工具 =====================
 * 功能：安全地读取 JSON 字段，以及在退出登录、刷新数据时释放和清空内存。
 */
const char *lab_str(const cJSON *, const char *); // 读字符串字段；字段不存在时返回 ""，调用方可以直接 strcmp 不怕空指针
int lab_int(const cJSON *, const char *);         // 读整数字段；字段不存在或不是数字时返回 0
void lab_model_free(LabModel *);                  // 释放 5 张表并把指针清零；刷新数据前先释放旧数据
void lab_session_clear(LabSession *);             // 把令牌等身份信息从内存里彻底擦掉；退出登录时调用

/* ===================== 模块 6：与云端通信 =====================
 * 功能：所有业务操作最终都经过这两个函数发出请求。
 */
cJSON *lab_request(LabClient *, const char *, const char *, const cJSON *); // 通用请求（客户端, 方法, 路径, 请求体）；令牌过期时自动刷新并重试一次
cJSON *lab_rpc(LabClient *, const char *, const cJSON *);                   // 调用云端数据库里的业务函数（客户端, 函数名, 参数），例如 "v2_check_in"

/* ===================== 模块 7：账号 =====================
 * 功能：登录界面上的"登录并继续"和"创建账号"两个按钮。返回 1 成功、0 失败。
 */
int lab_login(LabClient *, const char *, const char *);                             // (客户端, 学号或邮箱, 密码)
int lab_register(LabClient *, const char *, const char *, const char *, const char *); // (客户端, 姓名, 学号或邮箱, 密码, 管理员邀请码；学生留空)

/* ===================== 模块 8：加载数据 =====================
 * 功能：登录后、每次操作后、每 30 秒自动刷新时，把 5 张表全部重新拉一遍。
 */
int lab_load_model(LabClient *, LabModel *); // 成功时把新数据写进第二个参数；任何一张表失败都不改动原数据

/* ===================== 模块 9：预约时间与签到签退 =====================
 * 功能："我的"页面的签到 / 签退按钮，以及设备按钮上"使用中"的显示。
 * 当前时间由调用者传入，函数自己不取时间，所以测试时可以随意指定时间。
 */
int64_t lab_booking_start(const cJSON *);              // 算出一条预约的开始时刻（北京时间转成 Unix 毫秒）；数据不合法返回 -1
int lab_attendance_action(const cJSON *, int64_t);     // 现在能对这条预约做什么：0 不能操作，1 可以签到，2 可以签退
int lab_attend(LabClient *, const cJSON *, int, int64_t); // 真正执行签到(1)或签退(2)，最终是否允许由云端判断
                                                       // （同类函数 lab_booking_is_live 声明在模块 12 末尾）

/* ===================== 模块 10：提交预约 =====================
 * 功能：预约页的"确认预约"按钮。用户选几台设备、几个小时，按小时逐个提交。
 * 参数：(客户端, 日期, 开始小时, 结束小时, 设备编号数组, 设备数量, 每小时一个防重复编号, 写回已成功的小时数)
 * keys 有 9 个，因为可约时段是 8 点到 17 点共 9 小时。同一个编号重复提交，云端只算一次，网络重试不会重复预约。
 */
int lab_reserve_range(LabClient *, const char *, int, int, const int *, size_t, const char *keys[9], int *);

/* ===================== 模块 11：版本检查 =====================
 * 功能：程序启动时检查官网有没有新版本，有就弹窗提示下载。
 */
int lab_semver_newer(const char *, const char *); // 第一个版本号比第二个新时返回 1，只支持 "数字.数字.数字"

/* ===================== 模块 12：AI 助手与管理员统计 =====================
 * 功能："智能助手"页面：用户用自然语言描述需求，云端 AI 给出预约方案，用户选中后确认预约。
 *       管理员的"统计报表"按钮也在这里。
 */
cJSON *lab_agent_payload(const LabSession *, const char *message, const char *date, int start, int end, const int *devices, size_t count, const char *request_id, const char *session_id, const cJSON *history, const cJSON *confirmation); // 把用户输入、当前选择和对话历史打包成发给 AI 的请求
cJSON *lab_usage_metrics(LabClient *, const char *start_date, const char *end_date, int device); // 管理员查询某段日期的设备使用统计；device 为 0 表示全部设备
int lab_booking_is_live(const cJSON *, int64_t now); // 属于模块 9：这条预约此刻是否正在使用中（状态 in_use 且在预约的那一小时内），用于设备按钮显示"使用中"

typedef struct {
    char owner[64],             // 这段对话属于哪个用户；换账号时据此清空，防止看到别人的对话
         session[80],           // 云端分配的对话编号，多轮对话靠它串起来
         proposal[80],          // 用户当前选中的方案编号
         confirmation_key[80];  // 确认该方案用的防重复编号；同一方案重试时不变
    cJSON *history,             // 对话历史，最多保留 20 条
          *response;            // AI 最近一次的完整回复，里面有候选方案
} LabAgentState;

void lab_agent_clear(LabAgentState *);                    // 退出登录时清空对话
void lab_agent_bind(LabAgentState *, const char *owner);  // 把对话绑定到当前用户；用户变了就先清空旧对话
void lab_agent_accept(LabAgentState *, const char *owner, const char *message, const cJSON *response); // 收到 AI 回复后记进对话历史
cJSON *lab_agent_confirmation(LabAgentState *, int option, const char *new_key); // 用户选中第 option 个方案后，生成确认请求
const cJSON *lab_agent_proposals(const LabAgentState *);  // 取出 AI 最近给出的候选方案列表
void lab_agent_transcript(const LabAgentState *, char *out, size_t cap); // 把对话历史拼成"我：… 助手：…"的文本显示在界面上
void lab_agent_option(const cJSON *, int number, char *out, size_t cap);  // 把一个方案写成"方案 1 · 日期 时间 · 设备 #03"显示在下拉框里

/* ===================== 模块 13：导出与报表 =====================
 * 功能：管理员的"导出完整预约 / 故障"按钮，以及统计报表窗口里的文字。
 */
int lab_export_csv(const LabModel *,int issues,FILE *); // 把预约(issues=0)或故障(issues=1)写成 Excel 能打开的 CSV 文件
void lab_metrics_report(const cJSON *,char *,size_t);   // 把云端统计结果排版成可读的文字报表
#endif // 对应开头的 #ifndef，头文件保护到此结束

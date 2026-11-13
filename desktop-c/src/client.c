/*
 * client.c —— 业务层：程序"会做的所有事情"都在这里
 *
 * 功能：把用户在界面上的操作（登录、预约、签到、问 AI、导出……）变成发给云端的请求，
 *       再把云端的回复整理成程序能用的数据或中文提示。
 * 这个文件不包含任何 Windows 代码：发网络请求时调用的是 LabClient 里的 transport 函数指针，
 *       所以它能被测试程序直接拿去测试，也能被其他平台的界面复用。
 *
 * 模块一览：
 *   模块 1  安全工具：读 JSON、复制字符串、清空内存
 *   模块 2  与云端通信：发请求、令牌过期自动续期、错误翻译成中文
 *   模块 3  账号：登录、注册
 *   模块 4  加载数据：分页拉取 5 张表
 *   模块 5  时间换算：北京时间日期 → Unix 毫秒
 *   模块 6  签到签退
 *   模块 7  提交预约
 *   模块 8  版本比较
 *   模块 9  AI 助手请求与管理员统计
 *   模块 10 AI 助手对话状态
 *   模块 11 导出 CSV
 *   模块 12 统计报表排版
 */
#include "client.h"   // 本文件实现的全部函数声明和数据结构
#include <ctype.h>    // isalnum / isdigit / tolower：检查学号、函数名里的字符
#include <limits.h>   // INT_MIN / INT_MAX：读整数前检查范围
#include <stdio.h>    // snprintf：安全地拼字符串
#include <stdarg.h>   // va_list：实现可变参数的 report_append
#include <stdlib.h>   // free
#include <string.h>   // strlen / strcmp / memcpy / memset

/* ===================== 模块 1：安全工具 =====================
 * 功能：云端的数据可能缺字段、类型不对，字符串可能超长。这些小函数把这些意外挡在门外，
 *       让其他代码可以放心地读数据、复制字符串，不会因为一个空指针或越界就崩溃。
 */

// 读字符串字段：例如 lab_str(预约, "status") 得到 "confirmed"
const char *lab_str(const cJSON *j, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);   // 按字段名查找（区分大小写）；j 为 NULL 时也安全返回 NULL
    // 确认找到了、而且确实是字符串，才返回它的内容；否则返回空字符串 ""。
    // 返回 "" 而不是 NULL 的好处：调用方可以直接 strcmp(lab_str(...), "xxx")，不用先判空
    return cJSON_IsString(v) && v->valuestring ? v->valuestring : "";
}

// 读整数字段：例如 lab_int(预约, "device_number") 得到 12
int lab_int(const cJSON *j, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);
    // cJSON 里数字存成 double；先确认是数字、且在 int 能表示的范围内，再取整数值，否则返回 0
    return cJSON_IsNumber(v) && v->valuedouble >= INT_MIN && v->valuedouble <= INT_MAX ? v->valueint : 0;
}

// 安全复制字符串：放得下才复制，放不下就拒绝（返回 0），绝不写出 dst 的边界。
// 代替 strcpy：strcpy 不检查目标大小，是缓冲区溢出漏洞最常见的来源
static int copy_text(char *dst, size_t cap, const char *src) {
    if (!src || strlen(src) >= cap)    // 源为空，或长度 ≥ 容量（还要留 1 字节给结尾 \0）
        return 0;
    memcpy(dst, src, strlen(src) + 1); // +1 把结尾的 \0 一起复制过去
    return 1;
}
// 清空登录身份：退出登录、登录失败、账号被停用时调用，把令牌从内存里彻底抹掉
void lab_session_clear(LabSession *s) {
    // 把结构体当成一串字节逐个写 0。volatile 告诉编译器"每次写入都必须真的执行"：
    // 否则编译器可能发现这块内存之后没人再读，把整个清零循环当成无用代码删掉，令牌就残留在内存里
    volatile unsigned char *p = (volatile unsigned char *)s;
    size_t n = sizeof(*s);   // 整个结构体的字节数（约 12.5 KB）
    while (n--)              // 循环 n 次
        *p++ = 0;            // 当前字节写 0，指针后移一位
}

// 释放全部数据：刷新数据、退出登录、关闭程序时调用
void lab_model_free(LabModel *m) {
    cJSON_Delete(m->devices);    // 删除一个 cJSON 数组会连同它里面的每条记录一起释放
    cJSON_Delete(m->bookings);   // 指针是 NULL 时 cJSON_Delete 什么都不做，所以重复调用也安全
    cJSON_Delete(m->profiles);
    cJSON_Delete(m->issues);
    cJSON_Delete(m->usage);
    memset(m, 0, sizeof(*m));    // 5 个指针全部置为 NULL，防止之后误用已经释放的内存（悬空指针）
}

/* ===================== 模块 2：与云端通信 =====================
 * 功能：所有业务请求的必经之路。负责：把 JSON 转成文字发出去 → 把回复转回 JSON →
 *       令牌过期时自动换新令牌并重试 → 出错时把英文错误翻译成用户能看懂的中文。
 */

// 错误翻译：云端出错时，把回复里的英文错误整理成一句中文，写进 c->error，界面弹窗显示的就是它
static void explain(LabClient *c, cJSON *j) {
    // 步骤 1：不同的云端服务把错误放在不同字段里，按顺序挨个找，找到第一个非空的为止
    const char *msg = lab_str(j, "message"), *code = lab_str(j, "code");
    if (!*msg)                                  // *msg 是第一个字符；为 0 说明是空字符串
        msg = lab_str(j, "error_description");
    if (!*msg)
        msg = lab_str(j, "msg");
    if (!*msg)
        msg = lab_str(j, "error");
    // 步骤 2：识别几种常见错误，换成具体的中文提示
    if (!strcmp(code, "PGRST202") || !strcmp(code, "42883"))   // 数据库里找不到这个函数
        msg = "云端接口尚未安装，请管理员执行对应数据库迁移。";
    else if (strstr(msg, "during the booked hour"))            // strstr：在 msg 里查找这段英文
        msg = "仅可在预约时段内签到（北京时间）。";
    else if (strstr(msg, "Check-in is required"))
        msg = "请先签到，再签退。";
    else if (strstr(msg, "Invalid login credentials"))
        msg = "账号或密码不正确。";
    // 步骤 3：拼成最终提示，例如"账号或密码不正确。（HTTP 400）"；有错误码时在中间加上" · 错误码"
    snprintf(c->error, sizeof(c->error), "%s%s%s（HTTP %d）", *msg ? msg : "云端请求失败", *code ? " · " : "",
             code, c->status);
}

// 接收新令牌：登录、注册、令牌续期成功后，从回复里取出新令牌存进 c->session
static int accept_tokens(LabClient *c, cJSON *j) {
    // 步骤 1：先复制一份当前身份到临时变量 next，在 next 上修改。
    //         这样如果回复不完整中途失败，c->session 还是原样，不会留下"写了一半"的身份
    LabSession next = c->session;
    const cJSON *u = cJSON_GetObjectItemCaseSensitive(j, "user");   // 回复里的用户信息
    // 步骤 2：访问令牌、刷新令牌、用户 ID 三样必须都有且放得下，任何一样不满足都拒绝
    if (!*lab_str(j, "access_token") ||
        !copy_text(next.access, sizeof(next.access), lab_str(j, "access_token")) ||
        !copy_text(next.refresh, sizeof(next.refresh), lab_str(j, "refresh_token")) ||
        !copy_text(next.uid, sizeof(next.uid), lab_str(u, "id")) || !*next.uid) {
        snprintf(c->error, sizeof(c->error), "登录响应不完整，请重新登录。");
        return 0;
    }
    // 步骤 3：全部检查通过，整体替换（结构体可以用 = 一次性复制所有数组）
    c->session = next;
    lab_session_clear(&next);   // 临时变量里也有令牌，用完擦掉
    return 1;
}
// 发一次请求的核心函数。retry = 1 表示令牌过期时允许续期并重试一次；返回的 JSON 由调用者 cJSON_Delete
static cJSON *raw_request(LabClient *c, const char *method, const char *path, const cJSON *body, int retry) {
    // 步骤 1：有请求体就把 JSON 对象转成紧凑的文字（没有多余空格换行），这块文字由 cJSON 申请
    char *serialized = body ? cJSON_PrintUnformatted(body) : NULL;
    char *response;
    cJSON *j;
    c->error[0] = 0;      // 清掉上一次的错误，避免把旧错误当成这次的
    c->status = 0;
    if (body && !serialized) {   // 有请求体但转换失败，只可能是内存不足
        snprintf(c->error, sizeof(c->error), "内存不足。");
        return NULL;
    }
    // 步骤 2：通过函数指针把请求交给传输层（Windows 上是 lab_win_http）。
    //         业务层只知道"有个函数能发请求"，不关心它是怎么发的
    response = c->transport(c->context, LAB_API_HOST, method, path, LAB_PUBLIC_KEY, c->session.access,
                            serialized ? serialized : "", &c->status, c->error, sizeof(c->error));
    // 步骤 3：请求体里可能有密码，发完立即清零再释放
    if (serialized) {
        memset(serialized, 0, strlen(serialized));
        cJSON_free(serialized);   // cJSON 申请的内存要用 cJSON_free 释放
    }
    if (!response)                // 网络层失败（断网、超时），错误信息已由传输层写好
        return NULL;
    // 步骤 4：把回复文字解析成 JSON；回复为空（例如成功但无内容）时造一个 null 节点代表"成功、没数据"
    j = *response ? cJSON_Parse(response) : cJSON_CreateNull();
    memset(response, 0, strlen(response));   // 回复里可能有令牌，清零后释放
    free(response);
    // 步骤 5：401 表示访问令牌过期。如果允许重试且有刷新令牌，就自动续期，用户感觉不到
    if (c->status == 401 && retry && *c->session.refresh) {
        cJSON *payload = cJSON_CreateObject(), *fresh;
        cJSON_Delete(j);                                                   // 这次的 401 回复没用了
        cJSON_AddStringToObject(payload, "refresh_token", c->session.refresh);
        // 5.1 用刷新令牌换新令牌。这里递归调用自己，但传 retry = 0：续期本身失败时不会再续期，避免无限递归
        fresh = raw_request(c, "POST", "/auth/v1/token?grant_type=refresh_token", payload, 0);
        cJSON_Delete(payload);
        if (!fresh)
            return NULL;
        // 5.2 保存新令牌
        if (!accept_tokens(c, fresh)) {
            cJSON_Delete(fresh);
            return NULL;
        }
        cJSON_Delete(fresh);
        // 5.3 带着新令牌把原来的请求再发一次，同样 retry = 0，最多只重试这一次
        return raw_request(c, method, path, body, 0);
    }
    // 步骤 6：状态码不在 200~299 之间都算失败，把错误翻译成中文
    if (c->status < 200 || c->status >= 300) {
        explain(c, j);
        cJSON_Delete(j);
        return NULL;
    }
    // 步骤 7：状态码成功但内容不是合法 JSON（cJSON_Parse 返回了 NULL）
    if (!j)
        snprintf(c->error, sizeof(c->error), "云端返回了无法识别的数据，请稍后重试。");
    return j;
}

// 对外的通用请求入口：默认允许令牌过期时自动续期一次
cJSON *lab_request(LabClient *c, const char *method, const char *path, const cJSON *body) {
    return raw_request(c, method, path, body, 1);
}

// 调用云端数据库里的业务函数（例如 v2_check_in 签到），地址固定为 /rest/v1/rpc/函数名
cJSON *lab_rpc(LabClient *c, const char *name, const cJSON *body) {
    char path[180];
    const char *p;
    // 步骤 1：函数名会被拼进网址，所以只允许字母、数字、下划线，防止混入 / ? 等字符改变请求地址。
    //         (unsigned char) 转换：中文等字节在 char 里可能是负数，直接传给 isalnum 属于未定义行为
    for (p = name; *p; p++)
        if (!(isalnum((unsigned char)*p) || *p == '_'))
            return NULL;
    if (strlen(name) > 100)   // 步骤 2：限制长度，保证 path[180] 一定放得下
        return NULL;
    snprintf(path, sizeof(path), "/rest/v1/rpc/%s", name);   // 步骤 3：拼出完整路径
    return lab_request(c, "POST", path, body);               // 步骤 4：发出 POST 请求
}
/* ===================== 模块 3：账号 =====================
 * 功能：登录界面的"登录并继续"和"创建账号"。
 * 背景：云端的账号系统只认邮箱，学生却习惯用学号登录，所以学号要先换成一个内部邮箱
 *       （例如 b23010101@lab.njupt.local），再用它去登录。
 */

// 学号 → 内部邮箱。只在注册时、或者云端没有"查邮箱"函数的旧部署里使用
static int student_email(const char *id, char *email, size_t cap) {
    size_t n = strlen(id), i;
    if (n < 2 || n > 20)          // 步骤 1：学号长度必须在 2~20 位
        return 0;
    for (i = 0; i < n; i++)       // 步骤 2：只允许字母、数字、点、下划线、横线，防止拼出奇怪的邮箱
        if (!(isalnum((unsigned char)id[i]) || id[i] == '.' || id[i] == '_' || id[i] == '-'))
            return 0;
    if (n + 17 >= cap)            // 步骤 3：确认缓冲区放得下"学号 + @lab.njupt.local（16 字符）+ \0"
        return 0;
    for (i = 0; i < n; i++)       // 步骤 4：学号转小写写进 email，这样 B230101 和 b230101 是同一个账号
        email[i] = (char)tolower((unsigned char)id[i]);
    strcpy(email + n, "@lab.njupt.local");   // 步骤 5：接上后缀。上面已检查过长度，这里用 strcpy 是安全的
    return 1;
}

// 登录：identity 是用户输入的学号或邮箱
int lab_login(LabClient *c, const char *identity, const char *password) {
    char email[256];
    cJSON *p, *j;
    int ok;
    lab_session_clear(&c->session);   // 步骤 1：先清掉旧身份，防止登录失败后还残留上一个账号的令牌
    // 步骤 2：确定用哪个邮箱登录
    if (strchr(identity, '@')) {      // 2a. 输入里有 @，说明是邮箱（管理员），直接用
        if (!copy_text(email, sizeof(email), identity))
            return 0;
    } else {                          // 2b. 是学号：请云端函数查出这个学号对应的登录邮箱
        p = cJSON_CreateObject();
        cJSON_AddStringToObject(p, "entered_login_id", identity);
        j = lab_rpc(c, "resolve_student_auth_email", p);
        cJSON_Delete(p);
        if (j) {                      // 查询成功：回复应该就是一个字符串（邮箱）
            ok = cJSON_IsString(j) && copy_text(email, sizeof(email), j->valuestring);
            cJSON_Delete(j);
            if (!ok) {                // 查不到也只说"账号或密码不正确"，不透露"这个学号不存在"
                snprintf(c->error, sizeof(c->error), "账号或密码不正确。");
                return 0;
            }
        } else if (c->status == 404 && (strstr(c->error, "PGRST202") || strstr(c->error, "42883"))) {
            // 查询失败且原因是"云端没有这个函数"（旧版部署）：退回到本地规则自己拼邮箱
            if (!student_email(identity, email, sizeof(email))) {
                snprintf(c->error, sizeof(c->error), "请检查学号格式。");
                return 0;
            }
        } else                        // 其他失败（断网等）：错误信息已写好，直接返回
            return 0;
    }
    // 步骤 3：用邮箱 + 密码向云端换取令牌
    p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "email", email);
    cJSON_AddStringToObject(p, "password", password);
    j = raw_request(c, "POST", "/auth/v1/token?grant_type=password", p, 0);   // retry=0：还没登录，没有令牌可续期
    cJSON_Delete(p);
    if (!j)
        return 0;
    ok = accept_tokens(c, j);         // 步骤 4：保存令牌。姓名和角色之后由 lab_load_model 从用户表读取
    cJSON_Delete(j);
    return ok;
}

// 注册：invite 为空注册学生；填了管理员邀请码则注册管理员（邀请码是否有效由云端判断）
int lab_register(LabClient *c, const char *name, const char *id, const char *password, const char *invite) {
    char email[256];
    cJSON *p, *meta, *j;
    int ok;
    // 步骤 1：本地先做基本检查，不合格就不必发请求
    if (!*name || strlen(name) > 64 || strlen(password) < 6) {
        snprintf(c->error, sizeof(c->error), "请填写姓名，密码至少 6 个字符。");
        return 0;
    }
    // 步骤 2：确定注册邮箱：管理员必须填真实邮箱，学生由学号生成内部邮箱
    if (*invite) {
        if (!strchr(id, '@') || !copy_text(email, sizeof(email), id)) {
            snprintf(c->error, sizeof(c->error), "管理员注册需填写邮箱和邀请码。");
            return 0;
        }
    } else if (!student_email(id, email, sizeof(email))) {
        snprintf(c->error, sizeof(c->error), "请检查学号格式（2–20 位字母、数字、点、横线或下划线）。");
        return 0;
    }
    lab_session_clear(&c->session);   // 步骤 3：清掉旧身份
    // 步骤 4：组织注册请求。账号信息放在 p 里，学号、姓名、邀请码放在附加数据 meta 里，
    //         云端建账号时会把 meta 写进用户表
    p = cJSON_CreateObject();
    meta = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "email", email);
    cJSON_AddStringToObject(p, "password", password);
    cJSON_AddStringToObject(meta, "login_id", id);
    cJSON_AddStringToObject(meta, "display_name", name);
    if (*invite)
        cJSON_AddStringToObject(meta, "admin_invite_code", invite);
    cJSON_AddItemToObject(p, "data", meta);   // meta 挂到 p 下面，所有权交给 p，之后只删 p 即可
    j = raw_request(c, "POST", "/auth/v1/signup", p, 0);
    cJSON_Delete(p);                          // 连同 meta 一起释放
    if (!j)
        return 0;
    // 步骤 5：如果云端要求邮箱验证，回复里不会有令牌，此时账号已建好但还不能登录
    if (!*lab_str(j, "access_token")) {
        snprintf(c->error, sizeof(c->error),
                 "账号已创建，需完成邮箱验证后登录；学生内部邮箱请联系管理员检查认证配置。");
        cJSON_Delete(j);
        return 0;
    }
    ok = accept_tokens(c, j);                 // 步骤 6：注册成功直接视为已登录
    cJSON_Delete(j);
    return ok;
}

/* ===================== 模块 4：加载数据 =====================
 * 功能：登录后、每次操作后、每 30 秒自动刷新时，把 5 张表全部重新下载一遍，
 *       界面上所有列表、设备颜色、统计卡片都基于这份数据。
 */

// 分页读取一整张表。云端每次最多返回 1000 条，所以要一页一页地翻，直到最后一页
static cJSON *load_table(LabClient *c, const char *table, const char *columns, const char *order) {
    cJSON *all = cJSON_CreateArray();   // 汇总所有页的结果
    int offset = 0;                     // 从第几条开始取
    for (;;) {                          // 无限循环，靠里面的 return 退出
        char path[900];
        cJSON *rows, *item;
        int n;
        // 步骤 1：拼出这一页的地址，例如 /rest/v1/devices?select=number,status&order=number&limit=1000&offset=0
        snprintf(path, sizeof(path), "/rest/v1/%s?select=%s&order=%s&limit=1000&offset=%d", table, columns,
                 order, offset);
        rows = lab_request(c, "GET", path, NULL);
        if (!cJSON_IsArray(rows)) {     // 步骤 2：请求失败或回复不是数组，丢弃已取到的部分
            cJSON_Delete(rows);
            cJSON_Delete(all);
            return NULL;
        }
        n = cJSON_GetArraySize(rows);   // 这一页有几条
        // 步骤 3：把这一页的记录逐条"摘下来"挂到总数组上。摘下 = 转移所有权，不需要复制
        while ((item = cJSON_DetachItemFromArray(rows, 0)) != NULL)
            cJSON_AddItemToArray(all, item);
        cJSON_Delete(rows);             // 这一页已经空了，释放外壳
        if (n < 1000)                   // 步骤 4：不满 1000 条说明是最后一页
            return all;
        offset += n;                    // 步骤 5：翻到下一页
        if (offset >= 100000) {         // 步骤 6：上限 10 万条，防止数据异常时无限翻页、占满内存
            cJSON_Delete(all);
            snprintf(c->error, sizeof(c->error), "数据量过大，请使用网页管理端筛选后操作。");
            return NULL;
        }
    }
}
// 加载全部数据。先放进临时变量 next，5 张表全部成功才交给调用者；任何一张失败都不动原来的数据，
// 这样界面上不会出现"一半新一半旧"的情况
int lab_load_model(LabClient *c, LabModel *out) {
    LabModel next = {0};               // 5 个指针全部从 NULL 开始，失败时 lab_model_free 只会释放已加载的
    const cJSON *p = NULL, *item;
    // 步骤 1：先加载用户表，因为要靠它确认"我"的账号还能用
    next.profiles = load_table(c, "profiles", "id,login_id,display_name,role,active", "id");
    if (!next.profiles)
        goto fail;
    // 步骤 2：在用户表里找到 id 等于我的 uid 的那一行
    cJSON_ArrayForEach(item, next.profiles) if (!strcmp(lab_str(item, "id"), c->session.uid)) {
        p = item;
        break;
    }
    // 步骤 3：找不到我，或者管理员把我停用了（active 为 false）：清空身份，相当于强制退出登录
    if (!p || cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(p, "active"))) {
        snprintf(c->error, sizeof(c->error), "账号资料不可用或已停用，请联系管理员。");
        lab_session_clear(&c->session);
        goto fail;
    }
    // 步骤 4：把姓名、学号、角色写进登录身份。角色从这里读，而不是让用户自己声称是管理员
    copy_text(c->session.name, sizeof(c->session.name), lab_str(p, "display_name"));
    copy_text(c->session.login, sizeof(c->session.login), lab_str(p, "login_id"));
    copy_text(c->session.role, sizeof(c->session.role), lab_str(p, "role"));
    // 步骤 5：依次加载设备、预约、故障三张表，只取界面需要的列
    next.devices = load_table(c, "devices", "number,status", "number");
    if (!next.devices)
        goto fail;
    next.bookings =
        load_table(c, "reservations", "id,device_number,user_id,reservation_date,start_hour,status,created_at", "id");
    if (!next.bookings)
        goto fail;
    next.issues = load_table(
        c, "issue_reports",
        "id,device_number,reporter_id,description,status,created_at,resolved_at,resolved_by,resolution_note", "id");
    if (!next.issues)
        goto fail;
    // 步骤 6：加载签到记录。这张表加载失败不算致命错误：用空表代替，只是"我的"页面看不到实际使用时间
    next.usage = load_table(c, "reservation_usage_records", "reservation_id,checked_in_at,checked_out_at",
                            "reservation_id");
    if (!next.usage) {
        next.usage = cJSON_CreateArray();
        c->error[0] = 0;               // 清掉这个错误，不让它弹窗打扰用户
    }
    *out = next;                       // 步骤 7：全部成功，一次性交给调用者
    return 1;
fail:                                  // 统一失败出口：释放已经加载的表
    lab_model_free(&next);
    return 0;
}

/* ===================== 模块 5：时间换算 =====================
 * 功能：判断"现在能不能签到"、"设备是不是正在使用"，都要把预约的日期和小时变成一个可以比较大小的数字。
 *       统一用 Unix 毫秒（从 1970-01-01 00:00 UTC 起经过的毫秒数），全程不依赖电脑的时区设置，
 *       所以即使电脑时区设错了，签到判断也照样按北京时间来。
 */

// 公历日期 → 距离 1970-01-01 的天数。这是一个公开的经典算法（Howard Hinnant 日期算法）：
// 把 3 月当作一年的第一个月，闰日 2 月 29 日就落在年末，闰年计算变得很简单；再按 400 年一个周期计算
static int64_t civil_days(int y, unsigned m, unsigned d) {
    int era;
    unsigned yoe, doy, doe;
    y -= m <= 2;                                            // 1、2 月算作上一年的第 11、12 个月（m<=2 成立时值为 1）
    era = (y >= 0 ? y : y - 399) / 400;                     // 属于第几个 400 年周期（公历每 400 年完全重复一次）
    yoe = (unsigned)(y - era * 400);                        // 周期内的第几年：0~399
    doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;  // 从 3 月 1 日起算的第几天：0~365（153/5 这个公式正好算出各月累计天数）
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;            // 周期内的第几天：每年 365 天，加上 4 年一闰、减去百年不闰
    return (int64_t)era * 146097 + doe - 719468;            // 146097 是 400 年的总天数；减 719468 把起点挪到 1970-03-01 对应的 1970-01-01
}
// 预约开始时刻：例如 {"reservation_date":"2026-11-05","start_hour":14} → 北京时间 2026-11-05 14:00 的 Unix 毫秒。
// 同时承担数据校验：日期或小时不合法返回 -1
int64_t lab_booking_start(const cJSON *b) {
    const char *date = lab_str(b, "reservation_date");
    int y, m, d, h = lab_int(b, "start_hour"), days;
    // 步骤 1：除了第 5、8 个字符（下标 4、7，应该是横线），其余必须全是数字
    for (size_t i = 0; i < strlen(date); i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)date[i]))
            return -1;
    // 步骤 2：必须正好 10 个字符、格式为 YYYY-MM-DD，年份 1970~9999、月份 1~12，
    //         开始小时只能是 8~16（实验室开放 8:00~17:00，最后一个时段从 16 点开始）
    if (strlen(date) != 10 || sscanf(date, "%4d-%2d-%2d", &y, &m, &d) != 3 || date[4] != '-' ||
        date[7] != '-' || y < 1970 || y > 9999 || m < 1 || m > 12 || h < 8 || h > 16)
        return -1;
    // 步骤 3：查出这个月有几天。(int[]){...} 是 C99 的"复合字面量"：当场造一个匿名数组再取下标
    days = (int[]){31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}[m - 1];
    if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)))   // 闰年规则：4 年一闰，百年不闰，400 年再闰
        days = 29;
    if (d < 1 || d > days)             // 挡掉 2 月 30 日、4 月 31 日这类不存在的日期
        return -1;
    // 步骤 4：天数 × 86400 秒 + 小时偏移，再 × 1000 换成毫秒。
    //         为什么是 (h - 8)：北京时间比 UTC 早 8 小时，北京 h 点 = UTC (h-8) 点
    return (civil_days(y, (unsigned)m, (unsigned)d) * 86400 + (h - 8) * 3600) * 1000;
}

/* ===================== 模块 6：签到签退 =====================
 * 功能："我的"页面选中自己的一条预约，点"签到 / 签退"。
 * 规则：只能在预约的那一小时内签到；签到后（状态变成 in_use）可以签退。
 * 客户端先检查一遍，是为了及早给出友好提示；真正的判断由云端 v2_check_in / v2_check_out 再做一次，
 * 因为客户端的检查可以被绕过，不能当作安全措施。
 */

// 判断现在能对这条预约做什么：0 不能操作，1 可以签到，2 可以签退
int lab_attendance_action(const cJSON *b, int64_t now) {
    const char *s = lab_str(b, "status");
    int64_t start;
    if (!strcmp(s, "in_use"))          // 已经签到、正在使用：下一步只能签退
        return 2;
    if (strcmp(s, "reserved") && strcmp(s, "confirmed"))   // 既不是"已预约"也不是"使用中"（已完成、已取消等）：不能操作
        return 0;
    start = lab_booking_start(b);      // 算出这条预约的开始时刻
    // 现在处于 [开始, 开始 + 1 小时) 之间才能签到；3600000 毫秒 = 1 小时
    return start >= 0 && now >= start && now < start + 3600000 ? 1 : 0;
}

// 执行签到（action = 1）或签退（action = 2）
int lab_attend(LabClient *c, const cJSON *b, int action, int64_t now) {
    cJSON *p, *r;
    // 步骤 1：再核对一遍：动作合法、预约是我的、现在确实可以做这个动作。
    //         界面虽然检查过，但从点按钮到这里可能过了一段时间，状态可能已经变了
    if ((action != 1 && action != 2) || !b || strcmp(lab_str(b, "user_id"), c->session.uid) ||
        lab_attendance_action(b, now) != action) {
        snprintf(c->error, sizeof(c->error), "请检查当前账号、预约时段（北京时间）及签到状态，刷新后重试。");
        return 0;
    }
    // 步骤 2：把预约编号作为参数，调用云端对应的签到或签退函数
    p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "p_reservation_id", lab_str(b, "id"));
    r = lab_rpc(c, action == 1 ? "v2_check_in" : "v2_check_out", p);
    cJSON_Delete(p);
    if (!r)
        return 0;
    cJSON_Delete(r);                   // 步骤 3：只关心成功与否，回复内容不需要；新状态由随后的刷新拿到
    return 1;
}
/* ===================== 模块 7：提交预约 =====================
 * 功能：预约页点"确认预约"。用户可以一次选多台设备、多个小时（例如 14:00~17:00、3 台设备）。
 * 做法：按小时拆开，每小时发一次请求（一次请求里包含所有选中的设备）。
 * 防重复：每小时带一个不同的"幂等键"（一串随机 GUID）。同一个键云端只处理一次，
 *         所以网络超时后用户再点一次，也不会约出两份。
 * 注意：几个小时之间不是"全部成功或全部失败"。中途某小时失败就停下，前面成功的小时保留，
 *       committed 告诉界面成功了几个小时。
 */
int lab_reserve_range(LabClient *c, const char *date, int start, int end, const int *devices, size_t count,
                      const char *keys[9], int *committed) {
    int hour;
    size_t i;
    *committed = 0;                    // 先清零，任何提前返回时调用方看到的都是 0
    // 步骤 1：检查时间范围：8 点到 17 点之间、开始早于结束；设备数 1~40
    if (start < 8 || end > 17 || start >= end || !count || count > 40) {
        snprintf(c->error, sizeof(c->error), "请选择有效的设备和时间范围。");
        return 0;
    }
    // 步骤 2：每个设备号必须在 1~40 之间
    for (i = 0; i < count; i++)
        if (devices[i] < 1 || devices[i] > 40) {
            snprintf(c->error, sizeof(c->error), "设备编号无效。");
            return 0;
        }
    // 步骤 3：两两比较，不允许同一台设备出现两次（双重循环：每个设备和它前面的所有设备比）
    for (i = 0; i < count; i++)
        for (size_t k = 0; k < i; k++)
            if (devices[k] == devices[i]) {
                snprintf(c->error, sizeof(c->error), "设备不能重复选择。");
                return 0;
            }
    // 步骤 4：从开始小时到结束小时，逐小时提交
    for (hour = start; hour < end; hour++) {
        cJSON *p, *a, *r;
        // 4.1 取出这一小时的幂等键：8 点用 keys[0]，9 点用 keys[1]……
        if (!keys[hour - 8] || !*keys[hour - 8]) {
            snprintf(c->error, sizeof(c->error), "请求编号缺失。");
            return 0;
        }
        // 4.2 组织参数：{ p_date, p_start_hour, p_device_numbers:[...], p_idempotency_key }
        p = cJSON_CreateObject();
        a = cJSON_CreateArray();
        for (i = 0; i < count; i++)
            cJSON_AddItemToArray(a, cJSON_CreateNumber(devices[i]));
        cJSON_AddStringToObject(p, "p_date", date);
        cJSON_AddNumberToObject(p, "p_start_hour", hour);
        cJSON_AddItemToObject(p, "p_device_numbers", a);   // 设备数组交给 p 管理
        cJSON_AddStringToObject(p, "p_idempotency_key", keys[hour - 8]);
        // 4.3 调用云端创建预约。设备已被别人约走等冲突，由云端检查并返回错误
        r = lab_rpc(c, "v2_create_reservations", p);
        cJSON_Delete(p);
        if (!r)                        // 这一小时失败：立即停止，已成功的小时数在 committed 里
            return 0;
        cJSON_Delete(r);
        (*committed)++;                // 4.4 成功一小时，计数加 1（括号必须加：先取 committed 指向的值再自增）
    }
    return 1;
}

/* ===================== 模块 8：版本比较 =====================
 * 功能：启动时检查更新。官网上的版本号比程序自带的新，就弹窗提示下载。
 */
int lab_semver_newer(const char *a, const char *b) {
    unsigned x[3], y[3];
    char tail;
    // 步骤 1：两个版本号都只能含数字和点，带 "-beta" 之类的一律不认
    for (const char *p = a; *p; p++)
        if (!isdigit((unsigned char)*p) && *p != '.')
            return 0;
    for (const char *p = b; *p; p++)
        if (!isdigit((unsigned char)*p) && *p != '.')
            return 0;
    // 步骤 2：拆成三段数字。末尾多读一个 %c：如果三段之后还有多余字符（如 "3.1.0.5"），
    //         sscanf 会返回 4，从而被判为格式不对；正好 3 才合格
    if (sscanf(a, "%u.%u.%u%c", &x[0], &x[1], &x[2], &tail) != 3 ||
        sscanf(b, "%u.%u.%u%c", &y[0], &y[1], &y[2], &tail) != 3)
        return 0;
    // 步骤 3：从主版本号开始逐段比较，第一段不同的就决定了大小（例如 3.2.0 > 3.1.9）
    for (int i = 0; i < 3; i++) {
        if (x[i] > y[i])
            return 1;
        if (x[i] < y[i])
            return 0;
    }
    return 0;                          // 三段全相等：不算更新
}

/* ===================== 模块 9：AI 助手请求与管理员统计 =====================
 * 功能："智能助手"页面把用户的话、当前选择和对话历史打包发给云端 AI；
 *       管理员的"统计报表"按钮查询使用统计；设备按钮判断"使用中"。
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

// 设备按钮是否显示"使用中"：状态是 in_use，并且现在正处在这条预约的那一小时内。
// 两个条件都要：如果用户忘了签退，状态会一直是 in_use，只看状态就会让设备一直显示"使用中"
int lab_booking_is_live(const cJSON *booking,int64_t now) {
    int64_t start=lab_booking_start(booking);
    return !strcmp(lab_str(booking,"status"),"in_use") && start>0 && now>=start && now<start+3600000;
}

/* ===================== 模块 10：AI 助手对话状态 =====================
 * 功能：记住和 AI 聊了什么、AI 给了哪些方案、用户选了哪个方案，并把它们变成界面上的文字。
 * 一轮对话的顺序：bind 绑定用户 → payload 打包请求 → （main.c 发请求）→ accept 记录回复 →
 *               proposals / option 显示方案 → confirmation 确认方案 → 再发一次请求完成预约
 */

// 清空对话：退出登录时调用
void lab_agent_clear(LabAgentState *s) {
    cJSON_Delete(s->history);     // 释放对话历史
    cJSON_Delete(s->response);    // 释放最近一次回复
    memset(s,0,sizeof(*s));       // 所有字段清零，指针变 NULL
}

// 绑定用户：每次使用助手前调用。发现换了账号，就把上一个人的对话清掉，防止 A 看到 B 的聊天记录
void lab_agent_bind(LabAgentState *s,const char *owner) {
    if(strcmp(s->owner,owner)){                       // 当前对话的主人不是这个用户
        lab_agent_clear(s);                           // 清空旧对话
        copy_text(s->owner,sizeof(s->owner),owner);   // 记下新主人
    }
    if(!s->history)s->history=cJSON_CreateArray();    // 还没有历史数组就建一个空的
}

// 记录 AI 回复：owner 是发出请求时的用户，message 是用户当时说的话，response 是 AI 的完整回复
void lab_agent_accept(LabAgentState *s,const char *owner,const char *message,const cJSON *response) {
    // 步骤 1：安全检查。请求发出后用户可能已经切换了账号，这条迟到的回复属于上一个人，丢弃
    if(strcmp(s->owner,owner))return;
    if(!cJSON_IsObject(response))return;              // 回复格式不对也丢弃
    // 步骤 2：保存云端分配的对话编号，下一轮请求带上它，AI 才知道是同一段对话
    const char *session=lab_str(response,"sessionId");if(*session)copy_text(s->session,sizeof(s->session),session);
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

// 取出 AI 给的候选方案（借用指针，不能删除）。已经确认过的回复不再显示旧方案
const cJSON *lab_agent_proposals(const LabAgentState *s) {
    if(!strcmp(lab_str(s->response,"outcome"),"confirmed"))return NULL;
    return cJSON_GetObjectItemCaseSensitive(s->response,"proposals");
}

// 用户在下拉框选中第 option 个方案并点"选择此方案"，生成确认信息 {proposalId, idempotencyKey}
cJSON *lab_agent_confirmation(LabAgentState *s,int option,const char *new_key) {
    // 步骤 1：按下标取出方案和它的编号
    const cJSON *p=cJSON_GetArrayItem(lab_agent_proposals(s),option);const char *id=lab_str(p,"proposalId");
    // 步骤 2：编号和新的防重复编号都必须存在且放得下
    if(!*id || strlen(id)>=sizeof(s->proposal) || !new_key || !*new_key || strlen(new_key)>=sizeof(s->confirmation_key))return NULL;
    // 步骤 3：关键设计：只有换了一个方案，才启用新的防重复编号；
    //         同一个方案重复点确认（例如上次超时），沿用旧编号，云端就知道是同一次确认，不会重复预约
    if(strcmp(s->proposal,id)){copy_text(s->proposal,sizeof(s->proposal),id);copy_text(s->confirmation_key,sizeof(s->confirmation_key),new_key);}
    // 步骤 4：组装确认信息返回，调用者负责释放
    cJSON *result=cJSON_CreateObject();cJSON_AddStringToObject(result,"proposalId",s->proposal);
    cJSON_AddStringToObject(result,"idempotencyKey",s->confirmation_key);return result;
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

/* ===================== 模块 11：导出 CSV =====================
 * 功能：管理员点"导出完整预约 / 导出完整故障"，把已加载的全部记录保存成 Excel 能直接打开的 CSV 文件。
 */

// 写一个单元格：例如 abc"d 写成 "abc""d"
static void export_cell(FILE *file,const char *value) {
    // 步骤 1：写开头的双引号。内容以 = + - @ 制表符 回车 开头时，再加一个单引号（39 是 ' 的 ASCII 码）：
    //         Excel 会把 "=..." 当成公式执行，恶意内容可能借此运行命令（叫"CSV 注入"），加单引号就只当文字显示
    fputc('"',file);if(strchr("=+-@\t\r",*value)&&*value)fputc(39,file);
    // 步骤 2：逐字符写出内容；遇到双引号就多写一个（CSV 标准规定的转义方式）；最后写结尾双引号
    for(;*value;value++){if(*value=='"')fputc('"',file);fputc((unsigned char)*value,file);}fputc('"',file);
}

// 导出整张表：issues 为 1 导出故障，为 0 导出预约。文件由调用者打开和关闭，这里只负责写
int lab_export_csv(const LabModel *model,int issues,FILE *file) {
    if(!file || !model)return 0;
    // 步骤 1：两种表各自要导出的列。static：这两个数组只在程序启动时建立一次，不会每次调用都重建
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

/* ===================== 模块 12：统计报表排版 =====================
 * 功能：管理员点"统计报表"后弹出的文字报表。云端已经算好了所有数字，这里只负责排版成可读的文字，
 *       不在客户端重新计算，保证和网页版的统计口径一致。
 */

// 往报表末尾追加一段格式化文字，用法和 printf 一样。
// "..." 表示参数个数不固定；va_list / va_start / va_end 是 C 处理可变参数的标准方式，printf 本身也是这样实现的
static void report_append(char *out,size_t cap,const char *format,...) {
    size_t used=strlen(out);          // 找到当前末尾
    if(used>=cap)return;              // 已经写满，不再追加
    va_list args;                     // 用来依次取出 format 后面的那些参数
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

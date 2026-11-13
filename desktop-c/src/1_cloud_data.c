/*
 * 1_cloud_data.c —— 【第 1 人】和云端通信、加载数据
 *
 * 一句话：程序怎么跟云端数据库"对话"——发请求、令牌过期自动续期、把 5 张表拉下来。
 *
 * ============================== 讲解提纲（按顺序讲） ==============================
 * 0. 先打开 client.h 的【第 1 人】部分：
 *    - LabTransport 函数指针：业务层只知道"有个函数能发请求"，Windows 上是 lab_win_http，
 *      测试时换成假函数，不联网也能测。这是本模块最重要的设计点。
 *    - LabSession / LabModel / LabClient 三个结构体：我是谁、云端有什么数据、用什么发请求。
 * 1. raw_request（本文件第一个函数）：发一次请求的完整流程
 *    JSON 转文字 → 通过函数指针发出 → 回复解析成 JSON → 遇到 401 用刷新令牌续期，
 *    再递归调用自己重试一次（传 retry = 0，所以不会无限递归）→ 失败时把错误翻译成中文。
 * 2. lab_request / lab_rpc：对外的两个入口；lab_rpc 拼出 /rest/v1/rpc/函数名，
 *    第 2、3 人的预约、签到、AI 请求最后都走到这里。
 * 3. load_table：分页读表，每页 1000 条，不满 1000 条说明是最后一页。
 * 4. lab_load_model：5 张表先放进临时变量 next，全部成功才交给界面；
 *    任何一张失败就 goto fail 统一释放，界面不会出现"一半新一半旧"。
 * 5. lab_login + accept_tokens：学号先换成登录邮箱，再用邮箱和密码换令牌；
 *    新令牌也是先写进临时变量，检查完整后才整体替换。
 *
 * 和别人的衔接：第 2 人的 lab_reserve_range / lab_attend、第 3 人的 AI 请求都调用本文件的
 *    lab_rpc / lab_request；每次操作成功后，界面都会调用 lab_load_model 重新加载数据。
 * 界面里的对应位置：在 other/main.c 里搜索【答辩·第1人】。
 *
 * ============================== 可能被问到的问题 ==============================
 * 问：令牌续期为什么不会无限递归？
 * 答：续期请求和续期后的重试都传 retry = 0，第二次再遇到 401 就直接返回失败。
 * 问：为什么不直接改 out，要先装进 next？
 * 答：保证"要么全部换新，要么完全不动"。中途失败时界面继续显示旧数据，不会显示一半。
 * 问：为什么要用函数指针，而不是直接调用 WinHTTP？
 * 答：业务层因此不依赖 Windows，tests/client_test.c 用一个假的 transport 就能测试全部业务逻辑。
 * 问：一张表数据超过 1000 条怎么办？
 * 答：load_table 会一页一页地翻，直到某页不满 1000 条；上限 10 万条，防止数据异常时一直翻页。
 */
#include "client.h"   // 共用的数据结构和函数声明
#include <ctype.h>    // isalnum / tolower：检查学号、函数名里的字符
#include <stdio.h>    // snprintf：安全地拼字符串
#include <stdlib.h>   // free
#include <string.h>   // strlen / strcmp / strchr / strstr / memset

// 下面三个内部函数写在文件后半部分（补充代码），先在这里声明，前面的核心代码才能调用它们
static void explain(LabClient *c, cJSON *j);
static int accept_tokens(LabClient *c, cJSON *j);
static int student_email(const char *id, char *email, size_t cap);

/* ################################ 第一部分：核心代码（答辩讲这些） ################################ */

/* ---------- 1. 发请求：与云端通信 ----------
 * 功能：所有业务请求的必经之路。负责：把 JSON 转成文字发出去 → 把回复转回 JSON →
 *       令牌过期时自动换新令牌并重试 → 出错时把英文错误翻译成用户能看懂的中文。
 */

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

/* ---------- 2. 加载数据 ----------
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
    lab_copy_text(c->session.name, sizeof(c->session.name), lab_str(p, "display_name"));
    lab_copy_text(c->session.login, sizeof(c->session.login), lab_str(p, "login_id"));
    lab_copy_text(c->session.role, sizeof(c->session.role), lab_str(p, "role"));
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

/* ---------- 3. 登录 ----------
 * 功能：登录界面的"登录并继续"和"创建账号"。
 * 背景：云端的账号系统只认邮箱，学生却习惯用学号登录，所以学号要先换成一个内部邮箱
 *       （例如 b23010101@lab.njupt.local），再用它去登录。
 */

// 登录：identity 是用户输入的学号或邮箱
int lab_login(LabClient *c, const char *identity, const char *password) {
    char email[256];
    cJSON *p, *j;
    int ok;
    lab_session_clear(&c->session);   // 步骤 1：先清掉旧身份，防止登录失败后还残留上一个账号的令牌
    // 步骤 2：确定用哪个邮箱登录
    if (strchr(identity, '@')) {      // 2a. 输入里有 @，说明是邮箱（管理员），直接用
        if (!lab_copy_text(email, sizeof(email), identity))
            return 0;
    } else {                          // 2b. 是学号：请云端函数查出这个学号对应的登录邮箱
        p = cJSON_CreateObject();
        cJSON_AddStringToObject(p, "entered_login_id", identity);
        j = lab_rpc(c, "resolve_student_auth_email", p);
        cJSON_Delete(p);
        if (j) {                      // 查询成功：回复应该就是一个字符串（邮箱）
            ok = cJSON_IsString(j) && lab_copy_text(email, sizeof(email), j->valuestring);
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

// 接收新令牌：登录、注册、令牌续期成功后，从回复里取出新令牌存进 c->session
static int accept_tokens(LabClient *c, cJSON *j) {
    // 步骤 1：先复制一份当前身份到临时变量 next，在 next 上修改。
    //         这样如果回复不完整中途失败，c->session 还是原样，不会留下"写了一半"的身份
    LabSession next = c->session;
    const cJSON *u = cJSON_GetObjectItemCaseSensitive(j, "user");   // 回复里的用户信息
    // 步骤 2：访问令牌、刷新令牌、用户 ID 三样必须都有且放得下，任何一样不满足都拒绝
    if (!*lab_str(j, "access_token") ||
        !lab_copy_text(next.access, sizeof(next.access), lab_str(j, "access_token")) ||
        !lab_copy_text(next.refresh, sizeof(next.refresh), lab_str(j, "refresh_token")) ||
        !lab_copy_text(next.uid, sizeof(next.uid), lab_str(u, "id")) || !*next.uid) {
        snprintf(c->error, sizeof(c->error), "登录响应不完整，请重新登录。");
        return 0;
    }
    // 步骤 3：全部检查通过，整体替换（结构体可以用 = 一次性复制所有数组）
    c->session = next;
    lab_session_clear(&next);   // 临时变量里也有令牌，用完擦掉
    return 1;
}


/* ######################## 第二部分：补充代码（答辩不讲，被问到时再翻到这里） ######################## */

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
        if (!strchr(id, '@') || !lab_copy_text(email, sizeof(email), id)) {
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

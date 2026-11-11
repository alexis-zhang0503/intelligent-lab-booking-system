//【答辩注释】共享业务实现：登录与刷新令牌、分页加载、北京时间预约、签到签退、助手协议、统计和 CSV。此文件不依赖 Win32 或 GTK，便于两个平台复用和模拟网络测试。
//【讲解 L1】引入 client.h 的类型和函数声明。
#include "client.h"
//【讲解 L2】引入 ctype.h 的类型和函数声明。
#include <ctype.h>
//【讲解 L3】引入 limits.h 的类型和函数声明。
#include <limits.h>
//【讲解 L4】引入 stdio.h 的类型和函数声明。
#include <stdio.h>
//【讲解 L5】引入 stdarg.h 的类型和函数声明。
#include <stdarg.h>
//【讲解 L6】引入 stdlib.h 的类型和函数声明。
#include <stdlib.h>
//【讲解 L7】引入 string.h 的类型和函数声明。
#include <string.h>

//【讲解 L9】函数 lab_str：安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
const char *lab_str(const cJSON *j, const char *key) {
    //【讲解 L10】按精确字段名借用 JSON 子节点。
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);
    //【讲解 L11】结束当前函数并返回 cJSON_IsString(v) && v->valuestring ? v->valuestring : ""；具体成功或失败含义见函数说明。
    return cJSON_IsString(v) && v->valuestring ? v->valuestring : "";
//【讲解 L12】结束当前代码块。
}
//【讲解 L13】函数 lab_int：读取 JSON 整数，先检查数值类型和 int 范围；缺失或越界返回 0。
int lab_int(const cJSON *j, const char *key) {
    //【讲解 L14】按精确字段名借用 JSON 子节点。
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);
    //【讲解 L15】结束当前函数并返回 cJSON_IsNumber(v) && v->valuedouble >= INT_MIN && v->valuedouble <= INT_MAX ? v->valueint : 0；具体成功或失败含义见函数说明。
    return cJSON_IsNumber(v) && v->valuedouble >= INT_MIN && v->valuedouble <= INT_MAX ? v->valueint : 0;
//【讲解 L16】结束当前代码块。
}
//【讲解 L17】函数 copy_text：先检查目标缓冲区容量再复制字符串，保留末尾的空字符；成功返回 1。
static int copy_text(char *dst, size_t cap, const char *src) {
    //【讲解 L18】检查本行条件，满足后执行紧随其后的语句或代码块；计算字符串字节长度。
    if (!src || strlen(src) >= cap)
        //【讲解 L19】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L20】复制指定字节；计算字符串字节长度。
    memcpy(dst, src, strlen(src) + 1);
    //【讲解 L21】结束当前函数并返回 1；具体成功或失败含义见函数说明。
    return 1;
//【讲解 L22】结束当前代码块。
}
//【讲解 L23】函数 lab_session_clear：用 volatile 字节写入清空会话，降低清理被优化器省略的风险；并不等同于磁盘加密。
void lab_session_clear(LabSession *s) {
    //【讲解 L24】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    volatile unsigned char *p = (volatile unsigned char *)s;
    //【讲解 L25】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    size_t n = sizeof(*s);
    //【讲解 L26】条件成立时继续循环。
    while (n--)
        *p++ = 0;
//【讲解 L28】结束当前代码块。
}
//【讲解 L29】函数 lab_model_free：释放模型持有的五组 JSON，然后清零结构体；调用后旧记录指针不再有效。
void lab_model_free(LabModel *m) {
    //【讲解 L30】释放 JSON 对象及其子节点。
    cJSON_Delete(m->devices);
    //【讲解 L31】释放 JSON 对象及其子节点。
    cJSON_Delete(m->bookings);
    //【讲解 L32】释放 JSON 对象及其子节点。
    cJSON_Delete(m->profiles);
    //【讲解 L33】释放 JSON 对象及其子节点。
    cJSON_Delete(m->issues);
    //【讲解 L34】释放 JSON 对象及其子节点。
    cJSON_Delete(m->usage);
    //【讲解 L35】初始化或清空指定内存。
    memset(m, 0, sizeof(*m));
//【讲解 L36】结束当前代码块。
}
//【讲解 L37】函数 explain：把云端不同格式的错误整理成用户提示，并识别接口缺失及签到、登录错误。
static void explain(LabClient *c, cJSON *j) {
    //【讲解 L38】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    const char *msg = lab_str(j, "message"), *code = lab_str(j, "code");
    //【讲解 L39】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!*msg)
        //【讲解 L40】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        msg = lab_str(j, "error_description");
    //【讲解 L41】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!*msg)
        //【讲解 L42】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        msg = lab_str(j, "msg");
    //【讲解 L43】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!*msg)
        //【讲解 L44】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        msg = lab_str(j, "error");
    //【讲解 L45】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(code, "PGRST202") || !strcmp(code, "42883"))
        //【讲解 L46】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        msg = "云端接口尚未安装，请管理员执行对应数据库迁移。";
    //【讲解 L47】上一条件不成立时检查下一条件；查找子字符串。
    else if (strstr(msg, "during the booked hour"))
        //【讲解 L48】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        msg = "仅可在预约时段内签到（北京时间）。";
    //【讲解 L49】上一条件不成立时检查下一条件；查找子字符串。
    else if (strstr(msg, "Check-in is required"))
        //【讲解 L50】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        msg = "请先签到，再签退。";
    //【讲解 L51】上一条件不成立时检查下一条件；查找子字符串。
    else if (strstr(msg, "Invalid login credentials"))
        //【讲解 L52】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        msg = "账号或密码不正确。";
    //【讲解 L53】按容量格式化 UTF-8 文本。
    snprintf(c->error, sizeof(c->error), "%s%s%s（HTTP %d）", *msg ? msg : "云端请求失败", *code ? " · " : "",
             //【讲解 L54】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
             code, c->status);
//【讲解 L55】结束当前代码块。
}
//【讲解 L56】函数 accept_tokens：验证认证响应，再替换访问令牌、刷新令牌和用户 ID；失败时不接收不完整会话。
static int accept_tokens(LabClient *c, cJSON *j) {
    //【讲解 L57】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    LabSession next = c->session;
    //【讲解 L58】按精确字段名借用 JSON 子节点（字段 user）。
    const cJSON *u = cJSON_GetObjectItemCaseSensitive(j, "user");
    //【讲解 L59】检查本行条件，满足后执行紧随其后的语句或代码块；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    if (!*lab_str(j, "access_token") ||
        //【讲解 L60】调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        !copy_text(next.access, sizeof(next.access), lab_str(j, "access_token")) ||
        //【讲解 L61】调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        !copy_text(next.refresh, sizeof(next.refresh), lab_str(j, "refresh_token")) ||
        //【讲解 L62】调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
        !copy_text(next.uid, sizeof(next.uid), lab_str(u, "id")) || !*next.uid) {
        //【讲解 L63】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error), "登录响应不完整，请重新登录。");
        //【讲解 L64】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L65】结束当前代码块。
    }
    //【讲解 L66】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    c->session = next;
    //【讲解 L67】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
    lab_session_clear(&next);
    //【讲解 L68】结束当前函数并返回 1；具体成功或失败含义见函数说明。
    return 1;
//【讲解 L69】结束当前代码块。
}
//【讲解 L70】函数 raw_request：序列化请求并调用传输回调；收到 401 最多刷新令牌一次，然后重试原请求。返回 JSON 的所有权交给调用者。
static cJSON *raw_request(LabClient *c, const char *method, const char *path, const cJSON *body, int retry) {
    //【讲解 L71】把 JSON 序列化为紧凑文本。
    char *serialized = body ? cJSON_PrintUnformatted(body) : NULL;
    //【讲解 L72】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char *response;
    //【讲解 L73】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    cJSON *j;
    //【讲解 L74】声明或更新 error（错误文本）；按本行表达式保存对应状态。
    c->error[0] = 0;
    //【讲解 L75】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
    c->status = 0;
    //【讲解 L76】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (body && !serialized) {
        //【讲解 L77】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error), "内存不足。");
        //【讲解 L78】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
        return NULL;
    //【讲解 L79】结束当前代码块。
    }
    //【讲解 L80】声明或更新 access（访问令牌）、transport（HTTP 传输回调）、context（回调上下文）；按本行表达式保存对应状态。
    response = c->transport(c->context, LAB_API_HOST, method, path, LAB_PUBLIC_KEY, c->session.access,
                            //【讲解 L81】声明或更新 error（错误文本）、status（状态码或业务状态）；按本行表达式保存对应状态。
                            serialized ? serialized : "", &c->status, c->error, sizeof(c->error));
    //【讲解 L82】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (serialized) {
        //【讲解 L83】初始化或清空指定内存；计算字符串字节长度。
        memset(serialized, 0, strlen(serialized));
        //【讲解 L84】释放 cJSON 分配的文本。
        cJSON_free(serialized);
    //【讲解 L85】结束当前代码块。
    }
    //【讲解 L86】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!response)
        //【讲解 L87】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
        return NULL;
    //【讲解 L88】解析 JSON 文本。
    j = *response ? cJSON_Parse(response) : cJSON_CreateNull();
    //【讲解 L89】初始化或清空指定内存；计算字符串字节长度。
    memset(response, 0, strlen(response));
    //【讲解 L90】释放堆内存。
    free(response);
    //【讲解 L91】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (c->status == 401 && retry && *c->session.refresh) {
        //【讲解 L92】创建 JSON 对象。
        cJSON *payload = cJSON_CreateObject(), *fresh;
        //【讲解 L93】释放 JSON 对象及其子节点。
        cJSON_Delete(j);
        //【讲解 L94】写入字符串字段（字段 refresh_token）。
        cJSON_AddStringToObject(payload, "refresh_token", c->session.refresh);
        //【讲解 L95】调用 raw_request，序列化请求并调用传输回调。
        fresh = raw_request(c, "POST", "/auth/v1/token?grant_type=refresh_token", payload, 0);
        //【讲解 L96】释放 JSON 对象及其子节点。
        cJSON_Delete(payload);
        //【讲解 L97】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!fresh)
            //【讲解 L98】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
            return NULL;
        //【讲解 L99】检查本行条件，满足后执行紧随其后的语句或代码块；调用 accept_tokens，验证认证响应，再替换访问令牌、刷新令牌和用户 ID。
        if (!accept_tokens(c, fresh)) {
            //【讲解 L100】释放 JSON 对象及其子节点。
            cJSON_Delete(fresh);
            //【讲解 L101】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
            return NULL;
        //【讲解 L102】结束当前代码块。
        }
        //【讲解 L103】释放 JSON 对象及其子节点。
        cJSON_Delete(fresh);
        //【讲解 L104】调用 raw_request，序列化请求并调用传输回调。
        return raw_request(c, method, path, body, 0);
    //【讲解 L105】结束当前代码块。
    }
    //【讲解 L106】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (c->status < 200 || c->status >= 300) {
        //【讲解 L107】调用 explain，把云端不同格式的错误整理成用户提示，并识别接口缺失及签到、登录错误。
        explain(c, j);
        //【讲解 L108】释放 JSON 对象及其子节点。
        cJSON_Delete(j);
        //【讲解 L109】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
        return NULL;
    //【讲解 L110】结束当前代码块。
    }
    //【讲解 L111】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!j)
        //【讲解 L112】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error), "云端返回了无法识别的数据，请稍后重试。");
    //【讲解 L113】结束当前函数并返回 j；具体成功或失败含义见函数说明。
    return j;
//【讲解 L114】结束当前代码块。
}
//【讲解 L115】函数 lab_request：对外提供统一请求入口，开启一次令牌刷新重试。
cJSON *lab_request(LabClient *c, const char *method, const char *path, const cJSON *body) {
    //【讲解 L116】调用 raw_request，序列化请求并调用传输回调。
    return raw_request(c, method, path, body, 1);
//【讲解 L117】结束当前代码块。
}
//【讲解 L118】函数 lab_rpc：检查 RPC 名称只包含允许的字符，再构造 /rest/v1/rpc/ 路径并发送 POST。
cJSON *lab_rpc(LabClient *c, const char *name, const cJSON *body) {
    //【讲解 L119】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char path[180];
    //【讲解 L120】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    const char *p;
    //【讲解 L121】按本行初值、条件和步长循环。
    for (p = name; *p; p++)
        //【讲解 L122】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!(isalnum((unsigned char)*p) || *p == '_'))
            //【讲解 L123】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
            return NULL;
    //【讲解 L124】检查本行条件，满足后执行紧随其后的语句或代码块；计算字符串字节长度。
    if (strlen(name) > 100)
        //【讲解 L125】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
        return NULL;
    //【讲解 L126】按容量格式化 UTF-8 文本。
    snprintf(path, sizeof(path), "/rest/v1/rpc/%s", name);
    //【讲解 L127】调用 lab_request，对外提供统一请求入口，开启一次令牌刷新重试。
    return lab_request(c, "POST", path, body);
//【讲解 L128】结束当前代码块。
}
//【讲解 L129】函数 student_email：兼容旧认证部署：检查学生标识格式后拼接内部登录邮箱；仅在指定接口缺失时使用。
static int student_email(const char *id, char *email, size_t cap) {
    //【讲解 L130】计算字符串字节长度。
    size_t n = strlen(id), i;
    //【讲解 L131】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (n < 2 || n > 20)
        //【讲解 L132】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L133】按本行初值、条件和步长循环。
    for (i = 0; i < n; i++)
        //【讲解 L134】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!(isalnum((unsigned char)id[i]) || id[i] == '.' || id[i] == '_' || id[i] == '-'))
            //【讲解 L135】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
    //【讲解 L136】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (n + 17 >= cap)
        //【讲解 L137】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L138】按本行初值、条件和步长循环。
    for (i = 0; i < n; i++)
        //【讲解 L139】执行 tolower，参数沿用本行给出的对象、标志和容量。
        email[i] = (char)tolower((unsigned char)id[i]);
    //【讲解 L140】执行 strcpy，参数沿用本行给出的对象、标志和容量。
    strcpy(email + n, "@lab.njupt.local");
    //【讲解 L141】结束当前函数并返回 1；具体成功或失败含义见函数说明。
    return 1;
//【讲解 L142】结束当前代码块。
}
//【讲解 L143】函数 lab_login：支持邮箱或学号登录；学号先解析为认证邮箱，再向 Auth 获取会话。角色稍后从 profiles 读取。
int lab_login(LabClient *c, const char *identity, const char *password) {
    //【讲解 L144】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char email[256];
    //【讲解 L145】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    cJSON *p, *j;
    //【讲解 L146】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int ok;
    //【讲解 L147】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
    lab_session_clear(&c->session);
    //【讲解 L148】检查本行条件，满足后执行紧随其后的语句或代码块；查找指定分隔字符。
    if (strchr(identity, '@')) {
        //【讲解 L149】检查本行条件，满足后执行紧随其后的语句或代码块；调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符。
        if (!copy_text(email, sizeof(email), identity))
            //【讲解 L150】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
    //【讲解 L151】处理另一分支。
    } else {
        //【讲解 L152】创建 JSON 对象。
        p = cJSON_CreateObject();
        //【讲解 L153】写入字符串字段（字段 entered_login_id）。
        cJSON_AddStringToObject(p, "entered_login_id", identity);
        //【讲解 L154】调用 lab_rpc，检查 RPC 名称只包含允许的字符，再构造 /rest/v1/rpc/ 路径并发送 POST。
        j = lab_rpc(c, "resolve_student_auth_email", p);
        //【讲解 L155】释放 JSON 对象及其子节点。
        cJSON_Delete(p);
        //【讲解 L156】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (j) {
            //【讲解 L157】调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符。
            ok = cJSON_IsString(j) && copy_text(email, sizeof(email), j->valuestring);
            //【讲解 L158】释放 JSON 对象及其子节点。
            cJSON_Delete(j);
            //【讲解 L159】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (!ok) {
                //【讲解 L160】按容量格式化 UTF-8 文本。
                snprintf(c->error, sizeof(c->error), "账号或密码不正确。");
                //【讲解 L161】结束当前函数并返回 0；具体成功或失败含义见函数说明。
                return 0;
            //【讲解 L162】结束当前代码块。
            }
        //【讲解 L163】上一条件不成立时检查下一条件；查找子字符串。
        } else if (c->status == 404 && (strstr(c->error, "PGRST202") || strstr(c->error, "42883"))) {
            //【讲解 L164】检查本行条件，满足后执行紧随其后的语句或代码块；调用 student_email，兼容旧认证部署：检查学生标识格式后拼接内部登录邮箱。
            if (!student_email(identity, email, sizeof(email))) {
                //【讲解 L165】按容量格式化 UTF-8 文本。
                snprintf(c->error, sizeof(c->error), "请检查学号格式。");
                //【讲解 L166】结束当前函数并返回 0；具体成功或失败含义见函数说明。
                return 0;
            //【讲解 L167】结束当前代码块。
            }
        //【讲解 L168】处理另一分支。
        } else
            //【讲解 L169】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
    //【讲解 L170】结束当前代码块。
    }
    //【讲解 L171】创建 JSON 对象。
    p = cJSON_CreateObject();
    //【讲解 L172】写入字符串字段（字段 email）。
    cJSON_AddStringToObject(p, "email", email);
    //【讲解 L173】写入字符串字段（字段 password）。
    cJSON_AddStringToObject(p, "password", password);
    //【讲解 L174】调用 raw_request，序列化请求并调用传输回调。
    j = raw_request(c, "POST", "/auth/v1/token?grant_type=password", p, 0);
    //【讲解 L175】释放 JSON 对象及其子节点。
    cJSON_Delete(p);
    //【讲解 L176】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!j)
        //【讲解 L177】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L178】调用 accept_tokens，验证认证响应，再替换访问令牌、刷新令牌和用户 ID。
    ok = accept_tokens(c, j);
    //【讲解 L179】释放 JSON 对象及其子节点。
    cJSON_Delete(j);
    //【讲解 L180】结束当前函数并返回 ok；具体成功或失败含义见函数说明。
    return ok;
//【讲解 L181】结束当前代码块。
}
//【讲解 L182】函数 lab_register：组织学生或带邀请码的注册参数，调用云端注册流程；管理员资格由服务端验证。
int lab_register(LabClient *c, const char *name, const char *id, const char *password, const char *invite) {
    //【讲解 L183】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char email[256];
    //【讲解 L184】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    cJSON *p, *meta, *j;
    //【讲解 L185】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int ok;
    //【讲解 L186】检查本行条件，满足后执行紧随其后的语句或代码块；计算字符串字节长度。
    if (!*name || strlen(name) > 64 || strlen(password) < 6) {
        //【讲解 L187】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error), "请填写姓名，密码至少 6 个字符。");
        //【讲解 L188】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L189】结束当前代码块。
    }
    //【讲解 L190】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (*invite) {
        //【讲解 L191】检查本行条件，满足后执行紧随其后的语句或代码块；查找指定分隔字符；调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符。
        if (!strchr(id, '@') || !copy_text(email, sizeof(email), id)) {
            //【讲解 L192】按容量格式化 UTF-8 文本。
            snprintf(c->error, sizeof(c->error), "管理员注册需填写邮箱和邀请码。");
            //【讲解 L193】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
        //【讲解 L194】结束当前代码块。
        }
    //【讲解 L195】上一条件不成立时检查下一条件；调用 student_email，兼容旧认证部署：检查学生标识格式后拼接内部登录邮箱。
    } else if (!student_email(id, email, sizeof(email))) {
        //【讲解 L196】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error), "请检查学号格式（2–20 位字母、数字、点、横线或下划线）。");
        //【讲解 L197】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L198】结束当前代码块。
    }
    //【讲解 L199】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
    lab_session_clear(&c->session);
    //【讲解 L200】创建 JSON 对象。
    p = cJSON_CreateObject();
    //【讲解 L201】创建 JSON 对象。
    meta = cJSON_CreateObject();
    //【讲解 L202】写入字符串字段（字段 email）。
    cJSON_AddStringToObject(p, "email", email);
    //【讲解 L203】写入字符串字段（字段 password）。
    cJSON_AddStringToObject(p, "password", password);
    //【讲解 L204】写入字符串字段（字段 login_id）。
    cJSON_AddStringToObject(meta, "login_id", id);
    //【讲解 L205】写入字符串字段（字段 display_name）。
    cJSON_AddStringToObject(meta, "display_name", name);
    //【讲解 L206】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (*invite)
        //【讲解 L207】写入字符串字段（字段 admin_invite_code）。
        cJSON_AddStringToObject(meta, "admin_invite_code", invite);
    //【讲解 L208】把节点所有权交给对象（字段 data）。
    cJSON_AddItemToObject(p, "data", meta);
    //【讲解 L209】调用 raw_request，序列化请求并调用传输回调。
    j = raw_request(c, "POST", "/auth/v1/signup", p, 0);
    //【讲解 L210】释放 JSON 对象及其子节点。
    cJSON_Delete(p);
    //【讲解 L211】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!j)
        //【讲解 L212】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L213】检查本行条件，满足后执行紧随其后的语句或代码块；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    if (!*lab_str(j, "access_token")) {
        //【讲解 L214】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error),
                 //【讲解 L215】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                 "账号已创建，需完成邮箱验证后登录；学生内部邮箱请联系管理员检查认证配置。");
        //【讲解 L216】释放 JSON 对象及其子节点。
        cJSON_Delete(j);
        //【讲解 L217】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L218】结束当前代码块。
    }
    //【讲解 L219】调用 accept_tokens，验证认证响应，再替换访问令牌、刷新令牌和用户 ID。
    ok = accept_tokens(c, j);
    //【讲解 L220】释放 JSON 对象及其子节点。
    cJSON_Delete(j);
    //【讲解 L221】结束当前函数并返回 ok；具体成功或失败含义见函数说明。
    return ok;
//【讲解 L222】结束当前代码块。
}
//【讲解 L223】函数 load_table：每次读取 1000 条并合并，防止只拿到第一页；超过 100000 条时明确报错。
static cJSON *load_table(LabClient *c, const char *table, const char *columns, const char *order) {
    //【讲解 L224】创建 JSON 数组。
    cJSON *all = cJSON_CreateArray();
    //【讲解 L225】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int offset = 0;
    //【讲解 L226】按本行初值、条件和步长循环。
    for (;;) {
        //【讲解 L227】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        char path[900];
        //【讲解 L228】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        cJSON *rows, *item;
        //【讲解 L229】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        int n;
        //【讲解 L230】按容量格式化 UTF-8 文本。
        snprintf(path, sizeof(path), "/rest/v1/%s?select=%s&order=%s&limit=1000&offset=%d", table, columns,
                 //【讲解 L231】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                 order, offset);
        //【讲解 L232】调用 lab_request，对外提供统一请求入口，开启一次令牌刷新重试。
        rows = lab_request(c, "GET", path, NULL);
        //【讲解 L233】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!cJSON_IsArray(rows)) {
            //【讲解 L234】释放 JSON 对象及其子节点。
            cJSON_Delete(rows);
            //【讲解 L235】释放 JSON 对象及其子节点。
            cJSON_Delete(all);
            //【讲解 L236】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
            return NULL;
        //【讲解 L237】结束当前代码块。
        }
        //【讲解 L238】读取数组元素数。
        n = cJSON_GetArraySize(rows);
        //【讲解 L239】条件成立时继续循环；从原数组移出节点，准备转移所有权。
        while ((item = cJSON_DetachItemFromArray(rows, 0)) != NULL)
            //【讲解 L240】把节点所有权交给数组。
            cJSON_AddItemToArray(all, item);
        //【讲解 L241】释放 JSON 对象及其子节点。
        cJSON_Delete(rows);
        //【讲解 L242】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (n < 1000)
            //【讲解 L243】结束当前函数并返回 all；具体成功或失败含义见函数说明。
            return all;
        //【讲解 L244】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        offset += n;
        //【讲解 L245】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (offset >= 100000) {
            //【讲解 L246】释放 JSON 对象及其子节点。
            cJSON_Delete(all);
            //【讲解 L247】按容量格式化 UTF-8 文本。
            snprintf(c->error, sizeof(c->error), "数据量过大，请使用网页管理端筛选后操作。");
            //【讲解 L248】结束当前函数并返回 NULL；具体成功或失败含义见函数说明。
            return NULL;
        //【讲解 L249】结束当前代码块。
        }
    //【讲解 L250】结束当前代码块。
    }
//【讲解 L251】结束当前代码块。
}
//【讲解 L252】函数 lab_load_model：先加载账号资料并确认 active，再加载设备、预约、故障及使用记录；失败释放临时模型。usage 加载失败降级为空。
int lab_load_model(LabClient *c, LabModel *out) {
    //【讲解 L253】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    LabModel next = {0};
    //【讲解 L254】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    const cJSON *p = NULL, *item;
    //【讲解 L255】调用 load_table，每次读取 1000 条并合并，防止只拿到第一页。
    next.profiles = load_table(c, "profiles", "id,login_id,display_name,role,active", "id");
    //【讲解 L256】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!next.profiles)
        //【讲解 L257】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L258】遍历 JSON 数组；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    cJSON_ArrayForEach(item, next.profiles) if (!strcmp(lab_str(item, "id"), c->session.uid)) {
        //【讲解 L259】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        p = item;
        //【讲解 L260】结束当前最近的循环或 switch 分支。
        break;
    //【讲解 L261】结束当前代码块。
    }
    //【讲解 L262】检查本行条件，满足后执行紧随其后的语句或代码块；按精确字段名借用 JSON 子节点（字段 active）。
    if (!p || cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(p, "active"))) {
        //【讲解 L263】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error), "账号资料不可用或已停用，请联系管理员。");
        //【讲解 L264】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
        lab_session_clear(&c->session);
        //【讲解 L265】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L266】结束当前代码块。
    }
    //【讲解 L267】调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    copy_text(c->session.name, sizeof(c->session.name), lab_str(p, "display_name"));
    //【讲解 L268】调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    copy_text(c->session.login, sizeof(c->session.login), lab_str(p, "login_id"));
    //【讲解 L269】调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    copy_text(c->session.role, sizeof(c->session.role), lab_str(p, "role"));
    //【讲解 L270】调用 load_table，每次读取 1000 条并合并，防止只拿到第一页。
    next.devices = load_table(c, "devices", "number,status", "number");
    //【讲解 L271】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!next.devices)
        //【讲解 L272】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L273】声明或更新 bookings（预约列表）；按本行表达式保存对应状态。
    next.bookings =
        //【讲解 L274】调用 load_table，每次读取 1000 条并合并，防止只拿到第一页。
        load_table(c, "reservations", "id,device_number,user_id,reservation_date,start_hour,status,created_at", "id");
    //【讲解 L275】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!next.bookings)
        //【讲解 L276】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L277】调用 load_table，每次读取 1000 条并合并，防止只拿到第一页。
    next.issues = load_table(
        //【讲解 L278】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
        c, "issue_reports",
        //【讲解 L279】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
        "id,device_number,reporter_id,description,status,created_at,resolved_at,resolved_by,resolution_note", "id");
    //【讲解 L280】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!next.issues)
        //【讲解 L281】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L282】调用 load_table，每次读取 1000 条并合并，防止只拿到第一页。
    next.usage = load_table(c, "reservation_usage_records", "reservation_id,checked_in_at,checked_out_at",
                            //【讲解 L283】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                            "reservation_id");
    //【讲解 L284】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!next.usage) {
        //【讲解 L285】创建 JSON 数组。
        next.usage = cJSON_CreateArray();
        //【讲解 L286】声明或更新 error（错误文本）；按本行表达式保存对应状态。
        c->error[0] = 0;
    //【讲解 L287】结束当前代码块。
    }
    *out = next;
    //【讲解 L289】结束当前函数并返回 1；具体成功或失败含义见函数说明。
    return 1;
//【讲解 L290】失败清理入口，生成错误并释放资源。
fail:
    //【讲解 L291】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体。
    lab_model_free(&next);
    //【讲解 L292】结束当前函数并返回 0；具体成功或失败含义见函数说明。
    return 0;
//【讲解 L293】结束当前代码块。
}
/* Gregorian civil day conversion, independent of the computer timezone. */
//【讲解 L295】函数 civil_days：将公历年月日转换为距 1970-01-01 的天数，按 400 年周期计算，不依赖系统本地时区。
static int64_t civil_days(int y, unsigned m, unsigned d) {
    //【讲解 L296】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int era;
    //【讲解 L297】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    unsigned yoe, doy, doe;
    //【讲解 L298】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    y -= m <= 2;
    //【讲解 L299】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    era = (y >= 0 ? y : y - 399) / 400;
    //【讲解 L300】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    yoe = (unsigned)(y - era * 400);
    //【讲解 L301】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    //【讲解 L302】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    //【讲解 L303】结束当前函数并返回 (int64_t)era * 146097 + doe - 719468；具体成功或失败含义见函数说明。
    return (int64_t)era * 146097 + doe - 719468;
//【讲解 L304】结束当前代码块。
}
//【讲解 L305】函数 lab_booking_start：校验日期、闰年和 08–16 点起始小时；把北京时间预约开始转换为 Unix 毫秒，无效返回 -1。
int64_t lab_booking_start(const cJSON *b) {
    //【讲解 L306】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    const char *date = lab_str(b, "reservation_date");
    //【讲解 L307】调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
    int y, m, d, h = lab_int(b, "start_hour"), days;
    //【讲解 L308】按本行初值、条件和步长循环；计算字符串字节长度。
    for (size_t i = 0; i < strlen(date); i++)
        //【讲解 L309】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (i != 4 && i != 7 && !isdigit((unsigned char)date[i]))
            //【讲解 L310】结束当前函数并返回 -1；具体成功或失败含义见函数说明。
            return -1;
    //【讲解 L311】检查本行条件，满足后执行紧随其后的语句或代码块；计算字符串字节长度；从文本解析字段。
    if (strlen(date) != 10 || sscanf(date, "%4d-%2d-%2d", &y, &m, &d) != 3 || date[4] != '-' ||
        //【讲解 L312】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        date[7] != '-' || y < 1970 || y > 9999 || m < 1 || m > 12 || h < 8 || h > 16)
        //【讲解 L313】结束当前函数并返回 -1；具体成功或失败含义见函数说明。
        return -1;
    //【讲解 L314】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    days = (int[]){31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}[m - 1];
    //【讲解 L315】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)))
        //【讲解 L316】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        days = 29;
    //【讲解 L317】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (d < 1 || d > days)
        //【讲解 L318】结束当前函数并返回 -1；具体成功或失败含义见函数说明。
        return -1;
    //【讲解 L319】调用 civil_days，将公历年月日转换为距 1970-01-01 的天数，按 400 年周期计算，不依赖系统本地时区。
    return (civil_days(y, (unsigned)m, (unsigned)d) * 86400 + (h - 8) * 3600) * 1000;
//【讲解 L320】结束当前代码块。
}
//【讲解 L321】函数 lab_attendance_action：根据状态和时间给出可执行动作：0 不可操作，1 签到，2 签退。in_use 可进入签退分支，最终规则仍由云端校验。
int lab_attendance_action(const cJSON *b, int64_t now) {
    //【讲解 L322】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    const char *s = lab_str(b, "status");
    //【讲解 L323】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int64_t start;
    //【讲解 L324】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (!strcmp(s, "in_use"))
        //【讲解 L325】结束当前函数并返回 2；具体成功或失败含义见函数说明。
        return 2;
    //【讲解 L326】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if (strcmp(s, "reserved") && strcmp(s, "confirmed"))
        //【讲解 L327】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L328】调用 lab_booking_start，校验日期、闰年和 08–16 点起始小时。
    start = lab_booking_start(b);
    //【讲解 L329】结束当前函数并返回 start >= 0 && now >= start && now < start + 3600000 ? 1 : 0；具体成功或失败含义见函数说明。
    return start >= 0 && now >= start && now < start + 3600000 ? 1 : 0;
//【讲解 L330】结束当前代码块。
}
//【讲解 L331】函数 lab_attend：校验预约归属及动作，再调用 v2_check_in 或 v2_check_out；本地检查不是最终授权。
int lab_attend(LabClient *c, const cJSON *b, int action, int64_t now) {
    //【讲解 L332】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    cJSON *p, *r;
    //【讲解 L333】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    if ((action != 1 && action != 2) || !b || strcmp(lab_str(b, "user_id"), c->session.uid) ||
        //【讲解 L334】调用 lab_attendance_action，根据状态和时间给出可执行动作：0 不可操作，1 签到，2 签退。
        lab_attendance_action(b, now) != action) {
        //【讲解 L335】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error), "请检查当前账号、预约时段（北京时间）及签到状态，刷新后重试。");
        //【讲解 L336】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L337】结束当前代码块。
    }
    //【讲解 L338】创建 JSON 对象。
    p = cJSON_CreateObject();
    //【讲解 L339】写入字符串字段；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针（字段 p_reservation_id）。
    cJSON_AddStringToObject(p, "p_reservation_id", lab_str(b, "id"));
    //【讲解 L340】调用 lab_rpc，检查 RPC 名称只包含允许的字符，再构造 /rest/v1/rpc/ 路径并发送 POST。
    r = lab_rpc(c, action == 1 ? "v2_check_in" : "v2_check_out", p);
    //【讲解 L341】释放 JSON 对象及其子节点。
    cJSON_Delete(p);
    //【讲解 L342】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!r)
        //【讲解 L343】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L344】释放 JSON 对象及其子节点。
    cJSON_Delete(r);
    //【讲解 L345】结束当前函数并返回 1；具体成功或失败含义见函数说明。
    return 1;
//【讲解 L346】结束当前代码块。
}
//【讲解 L347】函数 lab_reserve_range：按小时提交多个设备，每小时使用一个幂等键。失败立即停止，committed 记录已成功的小时数；整个时间范围不是一个事务。
int lab_reserve_range(LabClient *c, const char *date, int start, int end, const int *devices, size_t count,
                      //【讲解 L348】声明或更新 keys（每小时幂等键）、committed（已成功提交的小时数）；按本行表达式保存对应状态。
                      const char *keys[9], int *committed) {
    //【讲解 L349】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int hour;
    //【讲解 L350】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    size_t i;
    *committed = 0;
    //【讲解 L352】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (start < 8 || end > 17 || start >= end || !count || count > 40) {
        //【讲解 L353】按容量格式化 UTF-8 文本。
        snprintf(c->error, sizeof(c->error), "请选择有效的设备和时间范围。");
        //【讲解 L354】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L355】结束当前代码块。
    }
    //【讲解 L356】按本行初值、条件和步长循环。
    for (i = 0; i < count; i++)
        //【讲解 L357】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (devices[i] < 1 || devices[i] > 40) {
            //【讲解 L358】按容量格式化 UTF-8 文本。
            snprintf(c->error, sizeof(c->error), "设备编号无效。");
            //【讲解 L359】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
        //【讲解 L360】结束当前代码块。
        }
    //【讲解 L361】按本行初值、条件和步长循环。
    for (i = 0; i < count; i++)
        //【讲解 L362】按本行初值、条件和步长循环。
        for (size_t k = 0; k < i; k++)
            //【讲解 L363】检查本行条件，满足后执行紧随其后的语句或代码块。
            if (devices[k] == devices[i]) {
                //【讲解 L364】按容量格式化 UTF-8 文本。
                snprintf(c->error, sizeof(c->error), "设备不能重复选择。");
                //【讲解 L365】结束当前函数并返回 0；具体成功或失败含义见函数说明。
                return 0;
            //【讲解 L366】结束当前代码块。
            }
    //【讲解 L367】按本行初值、条件和步长循环。
    for (hour = start; hour < end; hour++) {
        //【讲解 L368】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        cJSON *p, *a, *r;
        //【讲解 L369】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!keys[hour - 8] || !*keys[hour - 8]) {
            //【讲解 L370】按容量格式化 UTF-8 文本。
            snprintf(c->error, sizeof(c->error), "请求编号缺失。");
            //【讲解 L371】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
        //【讲解 L372】结束当前代码块。
        }
        //【讲解 L373】创建 JSON 对象。
        p = cJSON_CreateObject();
        //【讲解 L374】创建 JSON 数组。
        a = cJSON_CreateArray();
        //【讲解 L375】按本行初值、条件和步长循环。
        for (i = 0; i < count; i++)
            //【讲解 L376】把节点所有权交给数组。
            cJSON_AddItemToArray(a, cJSON_CreateNumber(devices[i]));
        //【讲解 L377】写入字符串字段（字段 p_date）。
        cJSON_AddStringToObject(p, "p_date", date);
        //【讲解 L378】写入数值字段（字段 p_start_hour）。
        cJSON_AddNumberToObject(p, "p_start_hour", hour);
        //【讲解 L379】把节点所有权交给对象（字段 p_device_numbers）。
        cJSON_AddItemToObject(p, "p_device_numbers", a);
        //【讲解 L380】写入字符串字段（字段 p_idempotency_key）。
        cJSON_AddStringToObject(p, "p_idempotency_key", keys[hour - 8]);
        //【讲解 L381】调用 lab_rpc，检查 RPC 名称只包含允许的字符，再构造 /rest/v1/rpc/ 路径并发送 POST。
        r = lab_rpc(c, "v2_create_reservations", p);
        //【讲解 L382】释放 JSON 对象及其子节点。
        cJSON_Delete(p);
        //【讲解 L383】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!r)
            //【讲解 L384】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
        //【讲解 L385】释放 JSON 对象及其子节点。
        cJSON_Delete(r);
        //【讲解 L386】声明或更新 committed（已成功提交的小时数）；按本行表达式保存对应状态。
        (*committed)++;
    //【讲解 L387】结束当前代码块。
    }
    //【讲解 L388】结束当前函数并返回 1；具体成功或失败含义见函数说明。
    return 1;
//【讲解 L389】结束当前代码块。
}
//【讲解 L390】函数 lab_semver_newer：只比较三段纯数字版本号；不支持 beta 等预发布标签。
int lab_semver_newer(const char *a, const char *b) {
    //【讲解 L391】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    unsigned x[3], y[3];
    //【讲解 L392】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char tail;
    //【讲解 L393】按本行初值、条件和步长循环。
    for (const char *p = a; *p; p++)
        //【讲解 L394】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!isdigit((unsigned char)*p) && *p != '.')
            //【讲解 L395】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
    //【讲解 L396】按本行初值、条件和步长循环。
    for (const char *p = b; *p; p++)
        //【讲解 L397】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!isdigit((unsigned char)*p) && *p != '.')
            //【讲解 L398】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
    //【讲解 L399】检查本行条件，满足后执行紧随其后的语句或代码块；从文本解析字段。
    if (sscanf(a, "%u.%u.%u%c", &x[0], &x[1], &x[2], &tail) != 3 ||
        //【讲解 L400】从文本解析字段。
        sscanf(b, "%u.%u.%u%c", &y[0], &y[1], &y[2], &tail) != 3)
        //【讲解 L401】结束当前函数并返回 0；具体成功或失败含义见函数说明。
        return 0;
    //【讲解 L402】按本行初值、条件和步长循环。
    for (int i = 0; i < 3; i++) {
        //【讲解 L403】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (x[i] > y[i])
            //【讲解 L404】结束当前函数并返回 1；具体成功或失败含义见函数说明。
            return 1;
        //【讲解 L405】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (x[i] < y[i])
            //【讲解 L406】结束当前函数并返回 0；具体成功或失败含义见函数说明。
            return 0;
    //【讲解 L407】结束当前代码块。
    }
    //【讲解 L408】结束当前函数并返回 0；具体成功或失败含义见函数说明。
    return 0;
//【讲解 L409】结束当前代码块。
}

/* Keep browser and native clients on the same V3 edge-function contract. */
//【讲解 L412】函数 lab_agent_payload：构造与网页版一致的 V3 助手请求字段；依据当前角色选择助手类型，对历史和确认对象做深复制。
cJSON *lab_agent_payload(const LabSession *session, const char *message, const char *date,
                        //【讲解 L413】声明或更新 devices（设备列表）；按本行表达式保存对应状态。
                        int start, int end, const int *devices, size_t count,
                        //【讲解 L414】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
                        const char *request_id, const char *session_id,
                        //【讲解 L415】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
                        const cJSON *history, const cJSON *confirmation) {
    //【讲解 L416】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!session || !date || !message || !request_id || !*request_id || count > 40 ||
        //【讲解 L417】声明或更新 devices（设备列表）；按本行表达式保存对应状态。
        (count && !devices) || start < 8 || end > 17 || start >= end) return NULL;
    //【讲解 L418】按本行初值、条件和步长循环。
    for (size_t i=0;i<count;i++) if (devices[i]<1 || devices[i]>40) return NULL;
    //【讲解 L419】创建 JSON 对象。
    cJSON *p=cJSON_CreateObject(), *input=cJSON_CreateObject();
    //【讲解 L420】写入字符串字段（字段 requestId）。
    cJSON_AddStringToObject(p,"requestId",request_id);
    //【讲解 L421】写入字符串字段；比较字符串内容（字段 agentType）。
    cJSON_AddStringToObject(p,"agentType",!strcmp(session->role,"admin")?"admin_analyst":"booking_planner");
    //【讲解 L422】写入字符串字段（字段 language）。
    cJSON_AddStringToObject(p,"language","zh");
    //【讲解 L423】写入字符串字段（字段 message）。
    cJSON_AddStringToObject(p,"message",message);
    //【讲解 L424】检查本行条件，满足后执行紧随其后的语句或代码块；写入字符串字段（字段 sessionId）。
    if(session_id && *session_id)cJSON_AddStringToObject(p,"sessionId",session_id);
    //【讲解 L425】把节点所有权交给对象；深复制 JSON 节点，隔离所有权；创建 JSON 数组（字段 history）。
    cJSON_AddItemToObject(p,"history",cJSON_IsArray(history)?cJSON_Duplicate(history,1):cJSON_CreateArray());
    //【讲解 L426】检查本行条件，满足后执行紧随其后的语句或代码块；把节点所有权交给对象；深复制 JSON 节点，隔离所有权（字段 confirmation）。
    if(confirmation)cJSON_AddItemToObject(p,"confirmation",cJSON_Duplicate(confirmation,1));
    //【讲解 L427】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if(!strcmp(session->role,"admin")) {
        //【讲解 L428】写入字符串字段（字段 startDate）。
        cJSON_AddStringToObject(input,"startDate",date);
        //【讲解 L429】写入字符串字段（字段 endDate）。
        cJSON_AddStringToObject(input,"endDate",date);
    //【讲解 L430】上一条件不成立时检查下一条件。
    } else if(count) {
        //【讲解 L431】创建 JSON 对象；创建 JSON 数组。
        cJSON *selection=cJSON_CreateObject(),*numbers=cJSON_CreateArray(),*hours=cJSON_CreateArray();
        //【讲解 L432】写入字符串字段（字段 date）。
        cJSON_AddStringToObject(selection,"date",date);
        //【讲解 L433】把节点所有权交给数组。
        cJSON_AddItemToArray(hours,cJSON_CreateNumber(start));
        //【讲解 L434】把节点所有权交给对象（字段 startHours）。
        cJSON_AddItemToObject(selection,"startHours",hours);
        //【讲解 L435】写入数值字段（字段 durationHours）。
        cJSON_AddNumberToObject(selection,"durationHours",end-start);
        //【讲解 L436】写入数值字段（字段 requiredDeviceCount）。
        cJSON_AddNumberToObject(selection,"requiredDeviceCount",(double)count);
        //【讲解 L437】按本行初值、条件和步长循环；把节点所有权交给数组。
        for(size_t i=0;i<count;i++)cJSON_AddItemToArray(numbers,cJSON_CreateNumber(devices[i]));
        //【讲解 L438】把节点所有权交给对象（字段 preferredDevices）。
        cJSON_AddItemToObject(selection,"preferredDevices",numbers);
        //【讲解 L439】把节点所有权交给对象（字段 bookingSelection）。
        cJSON_AddItemToObject(input,"bookingSelection",selection);
    //【讲解 L440】处理另一分支；写入 null，表示未指定或未知（字段 bookingSelection）。
    } else cJSON_AddNullToObject(input,"bookingSelection");
    //【讲解 L441】把节点所有权交给对象（字段 input）。
    cJSON_AddItemToObject(p,"input",input);
    //【讲解 L442】结束当前函数并返回 p；具体成功或失败含义见函数说明。
    return p;
//【讲解 L443】结束当前代码块。
}
//【讲解 L444】函数 lab_usage_metrics：调用 v3_admin_usage_metrics，device=0 表示全部设备；校验关键时长字段存在，返回值由调用者释放。
cJSON *lab_usage_metrics(LabClient *c,const char *start_date,const char *end_date,int device) {
    //【讲解 L445】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(!start_date || !end_date || device<0 || device>40) {
        //【讲解 L446】按容量格式化 UTF-8 文本。
        snprintf(c->error,sizeof(c->error),"Invalid metrics filter");return NULL;
    //【讲解 L447】结束当前代码块。
    }
    //【讲解 L448】创建 JSON 对象。
    cJSON *p=cJSON_CreateObject();
    //【讲解 L449】写入字符串字段（字段 p_start_date）。
    cJSON_AddStringToObject(p,"p_start_date",start_date);
    //【讲解 L450】写入字符串字段（字段 p_end_date）。
    cJSON_AddStringToObject(p,"p_end_date",end_date);
    //【讲解 L451】检查本行条件，满足后执行紧随其后的语句或代码块；写入数值字段（字段 p_device_number）。
    if(device)cJSON_AddNumberToObject(p,"p_device_number",device);
    //【讲解 L452】处理另一分支；写入 null，表示未指定或未知（字段 p_device_number）。
    else cJSON_AddNullToObject(p,"p_device_number");
    //【讲解 L453】调用 lab_rpc，检查 RPC 名称只包含允许的字符，再构造 /rest/v1/rpc/ 路径并发送 POST；释放 JSON 对象及其子节点。
    cJSON *r=lab_rpc(c,"v3_admin_usage_metrics",p);cJSON_Delete(p);
    //【讲解 L454】检查本行条件，满足后执行紧随其后的语句或代码块；按精确字段名借用 JSON 子节点（字段 actual_used_minutes）。
    if(r && (!cJSON_IsObject(r) || !cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(r,"actual_used_minutes")) ||
             //【讲解 L455】按精确字段名借用 JSON 子节点（字段 booked_device_minutes）。
             !cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(r,"booked_device_minutes")))) {
        //【讲解 L456】释放 JSON 对象及其子节点；按容量格式化 UTF-8 文本。
        cJSON_Delete(r);snprintf(c->error,sizeof(c->error),"统计响应不完整，请刷新重试。");return NULL;
    //【讲解 L457】结束当前代码块。
    }
    //【讲解 L458】结束当前函数并返回 r；具体成功或失败含义见函数说明。
    return r;
//【讲解 L459】结束当前代码块。
}
//【讲解 L460】函数 lab_booking_is_live：只有状态为 in_use 且当前时间在该小时内才显示正在使用，避免历史状态误亮。
int lab_booking_is_live(const cJSON *booking,int64_t now) {
    //【讲解 L461】调用 lab_booking_start，校验日期、闰年和 08–16 点起始小时。
    int64_t start=lab_booking_start(booking);
    //【讲解 L462】比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    return !strcmp(lab_str(booking,"status"),"in_use") && start>0 && now>=start && now<start+3600000;
//【讲解 L463】结束当前代码块。
}

//【讲解 L465】函数 lab_agent_clear：释放助手历史及最近响应并清空状态；当前实现只在内存中保存会话。
void lab_agent_clear(LabAgentState *s) {
    //【讲解 L466】释放 JSON 对象及其子节点；初始化或清空指定内存。
    cJSON_Delete(s->history);cJSON_Delete(s->response);memset(s,0,sizeof(*s));
//【讲解 L467】结束当前代码块。
}
//【讲解 L468】函数 lab_agent_bind：把助手状态绑定到账号；账号变化时清空旧历史，避免跨账号显示。
void lab_agent_bind(LabAgentState *s,const char *owner) {
    //【讲解 L469】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_agent_clear，释放助手历史及最近响应并清空状态；调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符。
    if(strcmp(s->owner,owner)){lab_agent_clear(s);copy_text(s->owner,sizeof(s->owner),owner);}
    //【讲解 L470】检查本行条件，满足后执行紧随其后的语句或代码块；创建 JSON 数组。
    if(!s->history)s->history=cJSON_CreateArray();
//【讲解 L471】结束当前代码块。
}
//【讲解 L472】函数 lab_agent_accept：拒绝其他账号的迟到响应，记录对话并最多保留 20 条消息；确认成功后清除待确认标记。
void lab_agent_accept(LabAgentState *s,const char *owner,const char *message,const cJSON *response) {
    //【讲解 L473】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容。
    if(strcmp(s->owner,owner))return; /* A response must never cross account boundaries. */
    //【讲解 L474】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(!cJSON_IsObject(response))return;
    //【讲解 L475】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针；调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符。
    const char *session=lab_str(response,"sessionId");if(*session)copy_text(s->session,sizeof(s->session),session);
    //【讲解 L476】释放 JSON 对象及其子节点；深复制 JSON 节点，隔离所有权。
    cJSON_Delete(s->response);s->response=cJSON_Duplicate(response,1);
    //【讲解 L477】按精确字段名借用 JSON 子节点（字段 errors）。
    const cJSON *errors=cJSON_GetObjectItemCaseSensitive(response,"errors");
    //【讲解 L478】调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    const char *reply=lab_str(response,"message");
    //【讲解 L479】检查本行条件，满足后执行紧随其后的语句或代码块；读取数组元素数；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针；按下标借用数组元素。
    if(cJSON_GetArraySize(errors))reply=lab_str(cJSON_GetArrayItem(errors,0),"message");
    //【讲解 L480】检查本行条件，满足后执行紧随其后的语句或代码块；创建 JSON 对象；写入字符串字段；把节点所有权交给数组（字段 role）。
    if(message && *message){cJSON *entry=cJSON_CreateObject();cJSON_AddStringToObject(entry,"role","user");cJSON_AddStringToObject(entry,"message",message);cJSON_AddItemToArray(s->history,entry);}
    //【讲解 L481】检查本行条件，满足后执行紧随其后的语句或代码块；创建 JSON 对象；写入字符串字段；把节点所有权交给数组（字段 role）。
    if(*reply){cJSON *entry=cJSON_CreateObject();cJSON_AddStringToObject(entry,"role","assistant");cJSON_AddStringToObject(entry,"message",reply);cJSON_AddItemToArray(s->history,entry);}
    //【讲解 L482】条件成立时继续循环；读取数组元素数；删除数组中的旧消息或元素。
    while(cJSON_GetArraySize(s->history)>20)cJSON_DeleteItemFromArray(s->history,0);
    //【讲解 L483】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    if(!strcmp(lab_str(response,"outcome"),"confirmed")){s->proposal[0]=0;s->confirmation_key[0]=0;}
//【讲解 L484】结束当前代码块。
}
//【讲解 L485】函数 lab_agent_proposals：借用最近响应中的候选方案数组；若 outcome 已 confirmed 则不再返回旧方案。
const cJSON *lab_agent_proposals(const LabAgentState *s) {
    //【讲解 L486】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    if(!strcmp(lab_str(s->response,"outcome"),"confirmed"))return NULL;
    //【讲解 L487】按精确字段名借用 JSON 子节点（字段 proposals）。
    return cJSON_GetObjectItemCaseSensitive(s->response,"proposals");
//【讲解 L488】结束当前代码块。
}
//【讲解 L489】函数 lab_agent_confirmation：根据所选方案构造确认体；同一方案重试复用原幂等键，换方案才更新键。
cJSON *lab_agent_confirmation(LabAgentState *s,int option,const char *new_key) {
    //【讲解 L490】按下标借用数组元素；调用 lab_agent_proposals，借用最近响应中的候选方案数组；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    const cJSON *p=cJSON_GetArrayItem(lab_agent_proposals(s),option);const char *id=lab_str(p,"proposalId");
    //【讲解 L491】检查本行条件，满足后执行紧随其后的语句或代码块；计算字符串字节长度。
    if(!*id || strlen(id)>=sizeof(s->proposal) || !new_key || !*new_key || strlen(new_key)>=sizeof(s->confirmation_key))return NULL;
    //【讲解 L492】检查本行条件，满足后执行紧随其后的语句或代码块；比较字符串内容；调用 copy_text，先检查目标缓冲区容量再复制字符串，保留末尾的空字符。
    if(strcmp(s->proposal,id)){copy_text(s->proposal,sizeof(s->proposal),id);copy_text(s->confirmation_key,sizeof(s->confirmation_key),new_key);}
    //【讲解 L493】创建 JSON 对象；写入字符串字段（字段 proposalId）。
    cJSON *result=cJSON_CreateObject();cJSON_AddStringToObject(result,"proposalId",s->proposal);
    //【讲解 L494】写入字符串字段（字段 idempotencyKey）。
    cJSON_AddStringToObject(result,"idempotencyKey",s->confirmation_key);return result;
//【讲解 L495】结束当前代码块。
}
//【讲解 L496】函数 lab_agent_transcript：把内存中的对话拼成有容量限制的中文显示文本；没有历史时显示引导语。
void lab_agent_transcript(const LabAgentState *s,char *out,size_t cap) {
    //【讲解 L497】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(!cap)return;out[0]=0;size_t used=0;const cJSON *entry;
    //【讲解 L498】遍历 JSON 数组；按容量格式化 UTF-8 文本；比较字符串内容。
    cJSON_ArrayForEach(entry,s->history){int n=snprintf(out+used,cap-used,"%s：%s\n\n",!strcmp(lab_str(entry,"role"),"user")?"我":"助手",lab_str(entry,"message"));if(n<0 || (size_t)n>=cap-used)break;used+=(size_t)n;}
    //【讲解 L499】检查本行条件，满足后执行紧随其后的语句或代码块；按容量格式化 UTF-8 文本。
    if(!*out)snprintf(out,cap,"告诉我你的预约需求。");
//【讲解 L500】结束当前代码块。
}
//【讲解 L501】函数 lab_agent_option：把一个候选方案的日期、小时、设备拼成下拉框摘要，并在写入时检查剩余容量。
void lab_agent_option(const cJSON *proposal,int number,char *out,size_t cap) {
    //【讲解 L502】检查本行条件，满足后执行紧随其后的语句或代码块；按容量格式化 UTF-8 文本。
    if(!cap)return;size_t used=0;int n=snprintf(out,cap,"方案 %d",number);if(n<0 || (size_t)n>=cap)return;used=(size_t)n;
    //【讲解 L503】按精确字段名借用 JSON 子节点（字段 candidate）。
    const cJSON *items=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(proposal,"candidate"),"items"),*item;
    //【讲解 L504】遍历 JSON 数组；按容量格式化 UTF-8 文本；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    cJSON_ArrayForEach(item,items){n=snprintf(out+used,cap-used," · %s %02d:00 · 设备",lab_str(item,"reservation_date"),lab_int(item,"start_hour"));if(n<0 || (size_t)n>=cap-used)return;used+=(size_t)n;
      //【讲解 L505】遍历 JSON 数组；按精确字段名借用 JSON 子节点；按容量格式化 UTF-8 文本（字段 device_numbers）。
      const cJSON *device;cJSON_ArrayForEach(device,cJSON_GetObjectItemCaseSensitive(item,"device_numbers")){n=snprintf(out+used,cap-used," #%02d",device->valueint);if(n<0 || (size_t)n>=cap-used)return;used+=(size_t)n;}}
//【讲解 L506】结束当前代码块。
}

//【讲解 L508】函数 export_cell：CSV 单元格始终加双引号；内部双引号写两次。公式前缀文本额外加单引号，降低表格软件公式执行风险。
static void export_cell(FILE *file,const char *value) {
    //【讲解 L509】输出一个字符；查找指定分隔字符。
    fputc('"',file);if(strchr("=+-@\t\r",*value)&&*value)fputc(39,file);
    //【讲解 L510】按本行初值、条件和步长循环；输出一个字符。
    for(;*value;value++){if(*value=='"')fputc('"',file);fputc((unsigned char)*value,file);}fputc('"',file);
//【讲解 L511】结束当前代码块。
}
//【讲解 L512】函数 lab_export_csv：以 UTF-8 BOM 输出当前已加载模型中的全部预约或故障；包含历史及取消记录，实际可见范围取决于云端权限。
int lab_export_csv(const LabModel *model,int issues,FILE *file) {
    //【讲解 L513】检查本行条件，满足后执行紧随其后的语句或代码块。
    if(!file || !model)return 0;
    //【讲解 L514】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
    static const char *book[]={"id","reservation_date","start_hour","device_number","user_id","status","created_at"};
    //【讲解 L515】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
    static const char *fault[]={"id","device_number","reporter_id","description","status","created_at","resolved_at","resolved_by","resolution_note"};
    //【讲解 L516】按字节写入文件。
    const char **fields=issues?fault:book;int count=issues?9:7;fwrite("\xef\xbb\xbf",1,3,file);
    //【讲解 L517】按本行初值、条件和步长循环；输出一个字符；调用 export_cell，CSV 单元格始终加双引号；输出文本到文件。
    for(int i=0;i<count;i++){if(i)fputc(',',file);export_cell(file,fields[i]);}fputs("\r\n",file);
    //【讲解 L518】遍历 JSON 数组。
    const cJSON *row;cJSON_ArrayForEach(row,(issues?model->issues:model->bookings)){
        //【讲解 L519】按本行初值、条件和步长循环；输出一个字符；按精确字段名借用 JSON 子节点。
        for(int i=0;i<count;i++){if(i)fputc(',',file);const cJSON *value=cJSON_GetObjectItemCaseSensitive(row,fields[i]);
            //【讲解 L520】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
            char number[64];const char *text="";if(cJSON_IsString(value))text=value->valuestring;
            //【讲解 L521】上一条件不成立时检查下一条件；按容量格式化 UTF-8 文本。
            else if(cJSON_IsNumber(value)){snprintf(number,sizeof(number),"%.17g",value->valuedouble);text=number;}
            //【讲解 L522】调用 export_cell，CSV 单元格始终加双引号。
            export_cell(file,text);
        //【讲解 L523】输出文本到文件。
        }fputs("\r\n",file);
    //【讲解 L524】检查文件写入错误。
    }return !ferror(file);
//【讲解 L525】结束当前代码块。
}

//【讲解 L527】函数 report_append：使用变参向报表缓冲区追加文本，容量参数包含终止空字符。
static void report_append(char *out,size_t cap,const char *format,...) {
    //【讲解 L528】计算字符串字节长度。
    size_t used=strlen(out);if(used>=cap)return;va_list args;va_start(args,format);vsnprintf(out+used,cap-used,format,args);va_end(args);
//【讲解 L529】结束当前代码块。
}
//【讲解 L530】函数 report_value：显示一个统计指标；比例乘 100 显示为百分比，非数字（含 null）显示暂无完整数据。
static void report_value(char *out,size_t cap,const cJSON *data,const char *key,const char *label,int percent){
 //【讲解 L531】按精确字段名借用 JSON 子节点；调用 report_append，使用变参向报表缓冲区追加文本，容量参数包含终止空字符。
 const cJSON *v=cJSON_GetObjectItemCaseSensitive(data,key);if(cJSON_IsNumber(v))report_append(out,cap,"%s：%.2f%s\r\n",label,v->valuedouble*(percent?100:1),percent?"%":"");else report_append(out,cap,"%s：暂无完整数据\r\n",label);
//【讲解 L532】结束当前代码块。
}
//【讲解 L533】函数 lab_metrics_report：将云端统计结果转换为总览、逐设备和逐时段报表；不在客户端重算完整统计口径。
void lab_metrics_report(const cJSON *data,char *out,size_t cap) {
 //【讲解 L534】检查本行条件，满足后执行紧随其后的语句或代码块；按精确字段名借用 JSON 子节点（字段 range）。
 if(!cap)return;out[0]=0;const cJSON *range=cJSON_GetObjectItemCaseSensitive(data,"range");
 //【讲解 L535】调用 report_append，使用变参向报表缓冲区追加文本，容量参数包含终止空字符；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
 report_append(out,cap,"实验室使用情况 %s — %s\r\n（北京时间；时长单位为设备分钟）\r\n\r\n",lab_str(range,"start_date"),lab_str(range,"end_date"));
 //【讲解 L536】声明或更新 keys（每小时幂等键）；按本行表达式保存对应状态。
 const char *keys[]={"booked_device_minutes","actual_used_minutes","provisional_used_minutes","capacity_device_minutes","reservation_rate","actual_usage_rate","usage_gap_rate","no_show_rate","cancellation_rate"};
 //【讲解 L537】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
 const char *labels[]={"预约时长","实际使用时长","进行中临时时长","可用时长","预约率","实际使用率","预约与使用率差异","爽约率","取消率"};
 //【讲解 L538】按本行初值、条件和步长循环；调用 report_value，显示一个统计指标。
 for(int i=0;i<9;i++)report_value(out,cap,data,keys[i],labels[i],i>=4);
 //【讲解 L539】调用 report_value，显示一个统计指标；按精确字段名借用 JSON 子节点（字段 data_completeness）。
 report_value(out,cap,cJSON_GetObjectItemCaseSensitive(data,"data_completeness"),"complete_usage_record_coverage","记录完整率",1);
 //【讲解 L540】调用 report_append，使用变参向报表缓冲区追加文本，容量参数包含终止空字符。
 const cJSON *row;report_append(out,cap,"\r\n每台设备：使用分钟 / 已知可用分钟 / 利用率\r\n");
 //【讲解 L541】遍历 JSON 数组；按精确字段名借用 JSON 子节点（字段 device_utilization）。
 cJSON_ArrayForEach(row,cJSON_GetObjectItemCaseSensitive(data,"device_utilization")){
 //【讲解 L542】调用 report_append，使用变参向报表缓冲区追加文本，容量参数包含终止空字符；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围；按精确字段名借用 JSON 子节点（字段 actual_used_minutes）。
 report_append(out,cap,"设备 #%02d  %.2f / %.2f  ",lab_int(row,"device_number"),cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"actual_used_minutes")),cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"known_capacity_minutes")));report_value(out,cap,row,"actual_usage_rate","利用率",1);}
 //【讲解 L543】调用 report_append，使用变参向报表缓冲区追加文本，容量参数包含终止空字符。
 report_append(out,cap,"\r\n每个时段：使用分钟 / 已知可用分钟 / 利用率\r\n");
 //【讲解 L544】遍历 JSON 数组；按精确字段名借用 JSON 子节点（字段 slot_usage_trends）。
 cJSON_ArrayForEach(row,cJSON_GetObjectItemCaseSensitive(data,"slot_usage_trends")){
 //【讲解 L545】调用 report_append，使用变参向报表缓冲区追加文本，容量参数包含终止空字符；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围；按精确字段名借用 JSON 子节点（字段 actual_used_minutes）。
 report_append(out,cap,"%02d:00–%02d:00  %.2f / %.2f  ",lab_int(row,"start_hour"),lab_int(row,"start_hour")+1,cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"actual_used_minutes")),cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"known_capacity_minutes")));report_value(out,cap,row,"actual_usage_rate","利用率",1);}
//【讲解 L546】结束当前代码块。
}

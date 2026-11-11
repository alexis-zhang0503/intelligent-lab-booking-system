//【答辩注释】共享 C 层测试。通过 LabTransport 注入模拟响应，用 assert 检查协议和边界；不需要生产账号，也不能代替真实云端端到端测试。
//【讲解 L1】引入 ../src/client.h 的类型和函数声明。
#include "../src/client.h"
//【讲解 L2】引入 assert.h 的类型和函数声明。
#include <assert.h>
//【讲解 L3】引入 stdio.h 的类型和函数声明。
#include <stdio.h>
//【讲解 L4】引入 stdlib.h 的类型和函数声明。
#include <stdlib.h>
//【讲解 L5】引入 string.h 的类型和函数声明。
#include <string.h>

//【讲解 L7】引入 ../src/ui_theme.h 的类型和函数声明。
#include "../src/ui_theme.h"

//【讲解 L9】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef struct {
    //【讲解 L10】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    const char *path, *response;
    //【讲解 L11】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
    int status;
    //【讲解 L12】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    const char *bodyContains, *token;
//【讲解 L13】结束当前代码块或类型声明。
} Step;
//【讲解 L14】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef struct {
    //【讲解 L15】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step *steps;
    //【讲解 L16】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    int count, index;
//【讲解 L17】结束当前代码块或类型声明。
} Mock;
//【讲解 L18】函数 send_mock：按预设 Step 返回模拟 HTTP 内容，并断言请求路径、身份头和请求体关键字段。
static char *send_mock(void *ctx, const char *host, const char *method, const char *path, const char *key,
                       //【讲解 L19】声明或更新 error（错误文本）、status（状态码或业务状态）；按本行表达式保存对应状态。
                       const char *token, const char *body, int *status, char *error, size_t cap) {
    //【讲解 L20】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Mock *m = ctx;
    //【讲解 L21】断言条件必须成立，失败立即暴露测试错误。
    assert(m->index < m->count);
    //【讲解 L22】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step *s = &m->steps[m->index++];
    //【讲解 L23】断言条件必须成立，失败立即暴露测试错误；比较字符串内容。
    assert(!strcmp(host, LAB_API_HOST));
    //【讲解 L24】断言条件必须成立，失败立即暴露测试错误；比较字符串内容。
    assert(!strcmp(key, LAB_PUBLIC_KEY));
    //【讲解 L25】断言条件必须成立，失败立即暴露测试错误。
    assert(*method);
    //【讲解 L26】断言条件必须成立，失败立即暴露测试错误；比较字符串内容。
    assert(!strcmp(path, s->path));
    //【讲解 L27】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (s->bodyContains)
        //【讲解 L28】断言条件必须成立，失败立即暴露测试错误；查找子字符串。
        assert(strstr(body, s->bodyContains));
    //【讲解 L29】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (s->token)
        //【讲解 L30】断言条件必须成立，失败立即暴露测试错误；比较字符串内容。
        assert(!strcmp(token, s->token));
    *status = s->status;
    *error = 0;
    //【讲解 L33】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    (void)cap;
    //【讲解 L34】申请堆内存；计算字符串字节长度。
    char *out = malloc(strlen(s->response) + 1);
    //【讲解 L35】执行 strcpy，参数沿用本行给出的对象、标志和容量。
    strcpy(out, s->response);
    //【讲解 L36】结束当前函数并返回 out；具体成功或失败含义见函数说明。
    return out;
//【讲解 L37】结束当前代码块。
}
//【讲解 L38】函数 client：构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
static LabClient client(Mock *m) {
    //【讲解 L39】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    LabClient c = {0};
    //【讲解 L40】声明或更新 transport（HTTP 传输回调）；按本行表达式保存对应状态。
    c.transport = send_mock;
    //【讲解 L41】声明或更新 context（回调上下文）；按本行表达式保存对应状态。
    c.context = m;
    //【讲解 L42】结束当前函数并返回 c；具体成功或失败含义见函数说明。
    return c;
//【讲解 L43】结束当前代码块。
}
//【讲解 L44】函数 auth：验证学号解析兼容分支和登录认证请求。
static void auth(void) {
    //【讲解 L45】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step steps[] = {{"/rest/v1/rpc/resolve_student_auth_email",
                     //【讲解 L46】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                     "{\"code\":\"PGRST202\",\"message\":\"missing\"}", 404, "entered_login_id", NULL},
                    //【讲解 L47】声明或更新 password（密码输入或任务副本）；按本行表达式保存对应状态。
                    {"/auth/v1/token?grant_type=password",
                     //【讲解 L48】声明或更新 refresh（刷新令牌）；按本行表达式保存对应状态。
                     "{\"access_token\":\"jwt\",\"refresh_token\":\"refresh\",\"user\":{\"id\":\"u\"}}", 200,
                     //【讲解 L49】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                     "abc@lab.njupt.local", ""}};
    //【讲解 L50】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Mock m = {steps, 2, 0};
    //【讲解 L51】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    LabClient c = client(&m);
    //【讲解 L52】断言条件必须成立，失败立即暴露测试错误；调用 lab_login，支持邮箱或学号登录。
    assert(lab_login(&c, "ABC", "p\"ass"));
    //【讲解 L53】断言条件必须成立，失败立即暴露测试错误；比较字符串内容。
    assert(m.index == 2 && !strcmp(c.session.uid, "u"));
    //【讲解 L54】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
    lab_session_clear(&c.session);
    //【讲解 L55】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step denied[] = {
        //【讲解 L56】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
        {"/rest/v1/rpc/resolve_student_auth_email", "{\"message\":\"not found\"}", 404, NULL, NULL}};
    //【讲解 L57】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    m = (Mock){denied, 1, 0};
    //【讲解 L58】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    c = client(&m);
    //【讲解 L59】断言条件必须成立，失败立即暴露测试错误；调用 lab_login，支持邮箱或学号登录。
    assert(!lab_login(&c, "ABC", "password"));
    //【讲解 L60】断言条件必须成立，失败立即暴露测试错误。
    assert(m.index == 1);
    //【讲解 L61】声明或更新 password（密码输入或任务副本）；按本行表达式保存对应状态。
    Step malformed[] = {{"/auth/v1/token?grant_type=password", "{\"access_token\":\"x\"}", 200, NULL, NULL}};
    //【讲解 L62】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    m = (Mock){malformed, 1, 0};
    //【讲解 L63】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    c = client(&m);
    //【讲解 L64】断言条件必须成立，失败立即暴露测试错误；调用 lab_login，支持邮箱或学号登录。
    assert(!lab_login(&c, "a@b.c", "password"));
    //【讲解 L65】断言条件必须成立，失败立即暴露测试错误。
    assert(!*c.session.access);
//【讲解 L66】结束当前代码块。
}
//【讲解 L67】函数 refresh：验证 401 后刷新令牌和重试原请求，避免无限递归。
static void refresh(void) {
    //【讲解 L68】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step steps[] = {{"/test", "{\"message\":\"expired\"}", 401, NULL, "old"},
                    //【讲解 L69】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                    {"/auth/v1/token?grant_type=refresh_token",
                     //【讲解 L70】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                     "{\"access_token\":\"new\",\"refresh_token\":\"r2\",\"user\":{\"id\":\"u\"}}", 200, "r1",
                     //【讲解 L71】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                     NULL},
                    //【讲解 L72】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                    {"/test", "[]", 200, NULL, "new"}};
    //【讲解 L73】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Mock m = {steps, 3, 0};
    //【讲解 L74】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    LabClient c = client(&m);
    //【讲解 L75】声明或更新 access（访问令牌）；按本行表达式保存对应状态。
    strcpy(c.session.access, "old");
    //【讲解 L76】声明或更新 refresh（刷新令牌）；按本行表达式保存对应状态。
    strcpy(c.session.refresh, "r1");
    //【讲解 L77】调用 lab_request，对外提供统一请求入口，开启一次令牌刷新重试。
    cJSON *r = lab_request(&c, "GET", "/test", NULL);
    //【讲解 L78】断言条件必须成立，失败立即暴露测试错误。
    assert(r && m.index == 3);
    //【讲解 L79】释放 JSON 对象及其子节点。
    cJSON_Delete(r);
    //【讲解 L80】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
    lab_session_clear(&c.session);
    //【讲解 L81】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step failed[] = {{"/test", "{}", 401, NULL, NULL},
                     //【讲解 L82】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
                     {"/auth/v1/token?grant_type=refresh_token", "{}", 401, NULL, NULL}};
    //【讲解 L83】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    m = (Mock){failed, 2, 0};
    //【讲解 L84】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    c = client(&m);
    //【讲解 L85】声明或更新 refresh（刷新令牌）；按本行表达式保存对应状态。
    strcpy(c.session.refresh, "r1");
    //【讲解 L86】断言条件必须成立，失败立即暴露测试错误；调用 lab_request，对外提供统一请求入口，开启一次令牌刷新重试。
    assert(!lab_request(&c, "POST", "/test", NULL));
    //【讲解 L87】断言条件必须成立，失败立即暴露测试错误。
    assert(m.index == 2);
//【讲解 L88】结束当前代码块。
}
//【讲解 L89】函数 attendance：验证北京时间边界、签到签退动作及当前用户归属。
static void attendance(void) {
    //【讲解 L90】解析 JSON 文本。
    cJSON *b = cJSON_Parse("{\"id\":\"reservation\",\"user_id\":\"u\",\"reservation_date\":\"2026-11-04\","
                           //【讲解 L91】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
                           "\"start_hour\":8,\"status\":\"reserved\"}");
    //【讲解 L92】调用 lab_booking_start，校验日期、闰年和 08–16 点起始小时。
    int64_t t = lab_booking_start(b);
    //【讲解 L93】断言条件必须成立，失败立即暴露测试错误。
    assert(t == 1793750400000LL);
    //【讲解 L94】断言条件必须成立，失败立即暴露测试错误；调用 lab_attendance_action，根据状态和时间给出可执行动作：0 不可操作，1 签到，2 签退。
    assert(!lab_attendance_action(b, t - 1));
    //【讲解 L95】断言条件必须成立，失败立即暴露测试错误；调用 lab_attendance_action，根据状态和时间给出可执行动作：0 不可操作，1 签到，2 签退。
    assert(lab_attendance_action(b, t) == 1);
    //【讲解 L96】断言条件必须成立，失败立即暴露测试错误；调用 lab_attendance_action，根据状态和时间给出可执行动作：0 不可操作，1 签到，2 签退。
    assert(lab_attendance_action(b, t + 3599999) == 1);
    //【讲解 L97】断言条件必须成立，失败立即暴露测试错误；调用 lab_attendance_action，根据状态和时间给出可执行动作：0 不可操作，1 签到，2 签退。
    assert(!lab_attendance_action(b, t + 3600000));
    //【讲解 L98】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step steps[] = {{"/rest/v1/rpc/v2_check_in", "null", 200, "\"p_reservation_id\":\"reservation\"", NULL},
                    //【讲解 L99】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                    {"/rest/v1/rpc/v2_check_out", "null", 200, "p_reservation_id", NULL}};
    //【讲解 L100】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Mock m = {steps, 2, 0};
    //【讲解 L101】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    LabClient c = client(&m);
    //【讲解 L102】声明或更新 uid（用户唯一标识）；按本行表达式保存对应状态。
    strcpy(c.session.uid, "other");
    //【讲解 L103】断言条件必须成立，失败立即暴露测试错误；调用 lab_attend，校验预约归属及动作，再调用 v2_check_in 或 v2_check_out。
    assert(!lab_attend(&c, b, 1, t));
    //【讲解 L104】断言条件必须成立，失败立即暴露测试错误。
    assert(m.index == 0);
    //【讲解 L105】声明或更新 uid（用户唯一标识）；按本行表达式保存对应状态。
    strcpy(c.session.uid, "u");
    //【讲解 L106】断言条件必须成立，失败立即暴露测试错误；调用 lab_attend，校验预约归属及动作，再调用 v2_check_in 或 v2_check_out。
    assert(!lab_attend(&c, b, 0, t - 1));
    //【讲解 L107】断言条件必须成立，失败立即暴露测试错误；调用 lab_attend，校验预约归属及动作，再调用 v2_check_in 或 v2_check_out。
    assert(lab_attend(&c, b, 1, t));
    //【讲解 L108】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
    cJSON_ReplaceItemInObject(b, "status", cJSON_CreateString("in_use"));
    //【讲解 L109】断言条件必须成立，失败立即暴露测试错误；调用 lab_attend，校验预约归属及动作，再调用 v2_check_in 或 v2_check_out。
    assert(lab_attend(&c, b, 2, t + 7200000));
    //【讲解 L110】断言条件必须成立，失败立即暴露测试错误。
    assert(m.index == 2);
    //【讲解 L111】执行 cJSON_ReplaceItemInObject、cJSON_CreateString，参数沿用本行给出的对象、标志和容量。
    cJSON_ReplaceItemInObject(b, "reservation_date", cJSON_CreateString("2026-02-30"));
    //【讲解 L112】断言条件必须成立，失败立即暴露测试错误；调用 lab_booking_start，校验日期、闰年和 08–16 点起始小时。
    assert(lab_booking_start(b) == -1);
    //【讲解 L113】释放 JSON 对象及其子节点。
    cJSON_Delete(b);
//【讲解 L114】结束当前代码块。
}
//【讲解 L115】函数 partial_booking：模拟跨小时预约中途失败，确认已成功小时数不会被误报为全部成功。
static void partial_booking(void) {
    //【讲解 L116】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step steps[] = {{"/rest/v1/rpc/v2_create_reservations", "[]", 200, "\"p_idempotency_key\":\"k0\"", NULL},
                    //【讲解 L117】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                    {"/rest/v1/rpc/v2_create_reservations", "{\"message\":\"conflict\"}", 409,
                     //【讲解 L118】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                     "\"p_idempotency_key\":\"k1\"", NULL}};
    //【讲解 L119】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Mock m = {steps, 2, 0};
    //【讲解 L120】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    LabClient c = client(&m);
    //【讲解 L121】声明或更新 keys（每小时幂等键）；按本行表达式保存对应状态。
    const char *keys[9] = {"k0", "k1"};
    //【讲解 L122】声明或更新 devices（设备列表）；按本行表达式保存对应状态。
    int devices[] = {1, 40}, n;
    //【讲解 L123】断言条件必须成立，失败立即暴露测试错误；调用 lab_reserve_range，按小时提交多个设备，每小时使用一个幂等键。
    assert(!lab_reserve_range(&c, "2026-11-04", 8, 10, devices, 2, keys, &n));
    //【讲解 L124】断言条件必须成立，失败立即暴露测试错误。
    assert(n == 1 && m.index == 2);
    //【讲解 L125】声明或更新 devices（设备列表）；按本行表达式保存对应状态。
    devices[1] = 1;
    //【讲解 L126】断言条件必须成立，失败立即暴露测试错误；调用 lab_reserve_range，按小时提交多个设备，每小时使用一个幂等键。
    assert(!lab_reserve_range(&c, "2026-11-04", 8, 9, devices, 2, keys, &n));
    //【讲解 L127】断言条件必须成立，失败立即暴露测试错误。
    assert(n == 0 && m.index == 2);
//【讲解 L128】结束当前代码块。
}
//【讲解 L129】函数 model_and_signup：验证分页模型、账号资料和注册相关请求。
static void model_and_signup(void) {
    //【讲解 L130】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step steps[] = {
        //【讲解 L131】声明或更新 role（角色）、profiles（用户资料）；按本行表达式保存对应状态。
        {"/rest/v1/profiles?select=id,login_id,display_name,role,active&order=id&limit=1000&offset=0",
         //【讲解 L132】声明或更新 role（角色）；按本行表达式保存对应状态。
         "[{\"id\":\"u\",\"role\":\"student\",\"active\":true,\"display_name\":\"Test\"}]", 200, NULL, NULL},
        //【讲解 L133】声明或更新 status（状态码或业务状态）、devices（设备列表）；按本行表达式保存对应状态。
        {"/rest/v1/devices?select=number,status&order=number&limit=1000&offset=0",
         //【讲解 L134】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
         "[{\"number\":1,\"status\":\"available\"}]", 200, NULL, NULL},
        //【讲解 L135】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
        {"/rest/v1/"
         //【讲解 L136】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
         "reservations?select=id,device_number,user_id,reservation_date,start_hour,status,created_at&order=id&limit="
         //【讲解 L137】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "1000&offset=0",
         //【讲解 L138】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "[]", 200, NULL, NULL},
        //【讲解 L139】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
        {"/rest/v1/"
         //【讲解 L140】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
         "issue_reports?select=id,device_number,reporter_id,description,status,created_at,resolved_at,"
         //【讲解 L141】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "resolved_by,resolution_note&order=id&limit=1000&offset=0",
         //【讲解 L142】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "[]", 200, NULL, NULL},
        //【讲解 L143】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
        {"/rest/v1/"
         //【讲解 L144】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "reservation_usage_records?select=reservation_id,checked_in_at,checked_out_at&order=reservation_id&"
         //【讲解 L145】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "limit=1000&offset=0",
         //【讲解 L146】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "[]", 200, NULL, NULL}};
    //【讲解 L147】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Mock m = {steps, 5, 0};
    //【讲解 L148】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    LabClient c = client(&m);
    //【讲解 L149】声明或更新 model（当前数据模型）；按本行表达式保存对应状态。
    LabModel model = {0};
    //【讲解 L150】声明或更新 uid（用户唯一标识）；按本行表达式保存对应状态。
    strcpy(c.session.uid, "u");
    //【讲解 L151】声明或更新 role（角色）；按本行表达式保存对应状态。
    strcpy(c.session.role, "admin");
    //【讲解 L152】断言条件必须成立，失败立即暴露测试错误；调用 lab_load_model，先加载账号资料并确认 active，再加载设备、预约、故障及使用记录。
    assert(lab_load_model(&c, &model));
    //【讲解 L153】断言条件必须成立，失败立即暴露测试错误。
    assert(m.index == 5);
    //【讲解 L154】断言条件必须成立，失败立即暴露测试错误；比较字符串内容。
    assert(!strcmp(c.session.role, "student"));
    //【讲解 L155】断言条件必须成立，失败立即暴露测试错误；读取数组元素数。
    assert(cJSON_GetArraySize(model.devices) == 1);
    //【讲解 L156】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体。
    lab_model_free(&model);
    //【讲解 L157】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step disabled[] = {
        //【讲解 L158】声明或更新 role（角色）、profiles（用户资料）；按本行表达式保存对应状态。
        {"/rest/v1/profiles?select=id,login_id,display_name,role,active&order=id&limit=1000&offset=0",
         //【讲解 L159】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "[{\"id\":\"u\",\"active\":false}]", 200, NULL, NULL}};
    //【讲解 L160】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    m = (Mock){disabled, 1, 0};
    //【讲解 L161】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    c = client(&m);
    //【讲解 L162】声明或更新 uid（用户唯一标识）；按本行表达式保存对应状态。
    strcpy(c.session.uid, "u");
    //【讲解 L163】声明或更新 access（访问令牌）；按本行表达式保存对应状态。
    strcpy(c.session.access, "token");
    //【讲解 L164】断言条件必须成立，失败立即暴露测试错误；调用 lab_load_model，先加载账号资料并确认 active，再加载设备、预约、故障及使用记录。
    assert(!lab_load_model(&c, &model));
    //【讲解 L165】断言条件必须成立，失败立即暴露测试错误。
    assert(!*c.session.access);
    //【讲解 L166】断言条件必须成立，失败立即暴露测试错误。
    assert(!model.profiles);
    //【讲解 L167】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step signup[] = {{"/auth/v1/signup",
                      //【讲解 L168】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                      "{\"access_token\":\"jwt\",\"refresh_token\":\"r\",\"user\":{\"id\":\"u\"}}", 200,
                      //【讲解 L169】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                      "admin_invite_code", NULL}};
    //【讲解 L170】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    m = (Mock){signup, 1, 0};
    //【讲解 L171】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态。
    c = client(&m);
    //【讲解 L172】断言条件必须成立，失败立即暴露测试错误；调用 lab_register，组织学生或带邀请码的注册参数，调用云端注册流程。
    assert(lab_register(&c, "Name \"Test\"", "admin@example.test", "password", "INVITE"));
    //【讲解 L173】调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
    lab_session_clear(&c.session);
//【讲解 L174】结束当前代码块。
}
//【讲解 L175】函数 parity_contracts：检查 V3 助手字段、统计完整性、null 容量和实时使用边界。
static void parity_contracts(void) {
    //【讲解 L176】声明或更新 role（角色）；按本行表达式保存对应状态。
    LabSession student={0},admin={0};strcpy(student.role,"student");strcpy(admin.role,"admin");
    //【讲解 L177】声明或更新 devices（设备列表）；按本行表达式保存对应状态。
    int devices[]={4,8};
    //【讲解 L178】解析 JSON 文本。
    cJSON *history=cJSON_Parse("[{\"role\":\"user\",\"message\":\"previous\"}]");
    //【讲解 L179】解析 JSON 文本。
    cJSON *confirmation=cJSON_Parse("{\"proposalId\":\"proposal\",\"idempotencyKey\":\"confirm-key\"}");
    //【讲解 L180】调用 lab_agent_payload，构造与网页版一致的 V3 助手请求字段。
    cJSON *p=lab_agent_payload(&student,"tomorrow","2026-11-06",9,12,devices,2,"request", "session",history,confirmation);
    //【讲解 L181】断言条件必须成立，失败立即暴露测试错误；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    assert(p && !strcmp(lab_str(p,"agentType"),"booking_planner"));
    //【讲解 L182】断言条件必须成立，失败立即暴露测试错误；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    assert(!strcmp(lab_str(p,"sessionId"),"session"));
    //【讲解 L183】按精确字段名借用 JSON 子节点（字段 input）。
    const cJSON *selection=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(p,"input"),"bookingSelection");
    //【讲解 L184】断言条件必须成立，失败立即暴露测试错误；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
    assert(lab_int(selection,"durationHours")==3 && lab_int(selection,"requiredDeviceCount")==2);
    //【讲解 L185】断言条件必须成立，失败立即暴露测试错误；读取数组元素数；按精确字段名借用 JSON 子节点（字段 preferredDevices）。
    assert(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(selection,"preferredDevices"))==2);
    //【讲解 L186】释放 JSON 对象及其子节点。
    cJSON_Delete(history);cJSON_Delete(confirmation);
    //【讲解 L187】断言条件必须成立，失败立即暴露测试错误；读取数组元素数；按精确字段名借用 JSON 子节点（字段 history）。
    assert(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(p,"history"))==1);
    //【讲解 L188】断言条件必须成立，失败立即暴露测试错误；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针（字段 confirmation）。
    assert(!strcmp(lab_str(cJSON_GetObjectItemCaseSensitive(p,"confirmation"),"proposalId"),"proposal"));cJSON_Delete(p);
    //【讲解 L189】调用 lab_agent_payload，构造与网页版一致的 V3 助手请求字段。
    p=lab_agent_payload(&admin,"usage","2026-11-06",8,9,NULL,0,"request",NULL,NULL,NULL);
    //【讲解 L190】断言条件必须成立，失败立即暴露测试错误；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    assert(p && !strcmp(lab_str(p,"agentType"),"admin_analyst"));
    //【讲解 L191】断言条件必须成立，失败立即暴露测试错误；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针（字段 input）。
    assert(!strcmp(lab_str(cJSON_GetObjectItemCaseSensitive(p,"input"),"startDate"),"2026-11-06"));cJSON_Delete(p);
    //【讲解 L192】断言条件必须成立，失败立即暴露测试错误；调用 lab_agent_payload，构造与网页版一致的 V3 助手请求字段。
    assert(!lab_agent_payload(&student,"x","2026-11-06",9,9,NULL,0,"id",NULL,NULL,NULL));
    //【讲解 L193】断言条件必须成立，失败立即暴露测试错误；调用 lab_agent_payload，构造与网页版一致的 V3 助手请求字段。
    assert(!lab_agent_payload(&student,"x","2026-11-06",8,9,NULL,2,"id",NULL,NULL,NULL));
    //【讲解 L194】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step steps[]={{"/rest/v1/rpc/v3_admin_usage_metrics","{\"actual_used_minutes\":50,\"booked_device_minutes\":60,\"capacity_device_minutes\":null}",200,"\"p_device_number\":null",NULL}};
    //【讲解 L195】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态；调用 lab_usage_metrics，调用 v3_admin_usage_metrics，device=0 表示全部设备。
    Mock mock={steps,1,0};LabClient c=client(&mock);p=lab_usage_metrics(&c,"2026-11-01","2026-11-07",0);
    //【讲解 L196】断言条件必须成立，失败立即暴露测试错误；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围；按精确字段名借用 JSON 子节点（字段 capacity_device_minutes）。
    assert(p && lab_int(p,"actual_used_minutes")==50 && cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(p,"capacity_device_minutes")));
    //【讲解 L197】释放 JSON 对象及其子节点；断言条件必须成立，失败立即暴露测试错误。
    cJSON_Delete(p);assert(mock.index==1);
    //【讲解 L198】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    Step malformed[]={{"/rest/v1/rpc/v3_admin_usage_metrics","{}",200,NULL,NULL}};
    //【讲解 L199】调用 client，构造使用 send_mock 的测试客户端，通过 context 指向模拟步骤状态；断言条件必须成立，失败立即暴露测试错误；调用 lab_usage_metrics，调用 v3_admin_usage_metrics，device=0 表示全部设备。
    mock=(Mock){malformed,1,0};c=client(&mock);assert(!lab_usage_metrics(&c,"2026-11-01","2026-11-07",4));
    //【讲解 L200】断言条件必须成立，失败立即暴露测试错误。
    assert(*c.error && mock.index==1);
    //【讲解 L201】解析 JSON 文本。
    p=cJSON_Parse("{\"reservation_date\":\"2026-11-06\",\"start_hour\":9,\"status\":\"in_use\"}");
    //【讲解 L202】调用 lab_booking_start，校验日期、闰年和 08–16 点起始小时；断言条件必须成立，失败立即暴露测试错误；调用 lab_booking_is_live，只有状态为 in_use 且当前时间在该小时内才显示正在使用，避免历史状态误亮。
    int64_t start=lab_booking_start(p);assert(!lab_booking_is_live(p,start-1));
    //【讲解 L203】断言条件必须成立，失败立即暴露测试错误；调用 lab_booking_is_live，只有状态为 in_use 且当前时间在该小时内才显示正在使用，避免历史状态误亮。
    assert(lab_booking_is_live(p,start));assert(lab_booking_is_live(p,start+3599999));
    //【讲解 L204】断言条件必须成立，失败立即暴露测试错误；调用 lab_booking_is_live，只有状态为 in_use 且当前时间在该小时内才显示正在使用，避免历史状态误亮；释放 JSON 对象及其子节点。
    assert(!lab_booking_is_live(p,start+3600000));cJSON_Delete(p);
//【讲解 L205】结束当前代码块。
}
//【讲解 L206】函数 main：程序入口。界面程序初始化 GTK 并进入事件循环；测试程序按顺序执行断言用例。
int main(void) {
    //【讲解 L207】调用 parity_contracts，检查 V3 助手字段、统计完整性、null 容量和实时使用边界。
    parity_contracts();
    //【讲解 L208】解析 JSON 文本；创建 JSON 数组；创建 JSON 对象（字段 description）。
    LabModel exports={0};exports.bookings=cJSON_Parse("[{\"id\":\"cancelled-record\",\"status\":\"cancelled\",\"reservation_date\":\"2025-01-01\"}]");exports.issues=cJSON_CreateArray();cJSON *issue=cJSON_CreateObject();cJSON_AddStringToObject(issue,"description","=SUM(A1)");cJSON_AddStringToObject(issue,"resolution_note","repair, note");cJSON_AddItemToArray(exports.issues,issue);
    //【讲解 L209】断言条件必须成立，失败立即暴露测试错误；调用 lab_export_csv，以 UTF-8 BOM 输出当前已加载模型中的全部预约或故障；回到临时文件开头。
    FILE *csv=tmpfile();assert(csv&&lab_export_csv(&exports,0,csv));rewind(csv);char exported[2048];size_t exportedSize=fread(exported,1,sizeof(exported)-1,csv);exported[exportedSize]=0;assert(strstr(exported,"cancelled-record")&&strstr(exported,"2025-01-01"));fclose(csv);csv=tmpfile();assert(csv&&lab_export_csv(&exports,1,csv));rewind(csv);exportedSize=fread(exported,1,sizeof(exported)-1,csv);exported[exportedSize]=0;assert(strstr(exported,"'=SUM(A1)")&&strstr(exported,"repair, note"));fclose(csv);lab_model_free(&exports);

    //【讲解 L211】调用 lab_agent_bind，把助手状态绑定到账号。
    LabAgentState agent={0};lab_agent_bind(&agent,"user-a");
    //【讲解 L212】解析 JSON 文本。
    cJSON *reply=cJSON_Parse("{\"sessionId\":\"session-a\",\"message\":\"choose\",\"outcome\":\"suggested\",\"proposals\":[{\"proposalId\":\"proposal-a\",\"candidate\":{\"items\":[{\"reservation_date\":\"2026-11-06\",\"start_hour\":9,\"device_numbers\":[4,8]}]}}]}");
    //【讲解 L213】调用 lab_agent_accept，拒绝其他账号的迟到响应，记录对话并最多保留 20 条消息；断言条件必须成立，失败立即暴露测试错误；读取数组元素数。
    lab_agent_accept(&agent,"user-a","two devices",reply);assert(cJSON_GetArraySize(agent.history)==2);
    //【讲解 L214】调用 lab_agent_confirmation，根据所选方案构造确认体。
    cJSON *first=lab_agent_confirmation(&agent,0,"key-first"),*retry=lab_agent_confirmation(&agent,0,"key-second");
    //【讲解 L215】断言条件必须成立，失败立即暴露测试错误；比较字符串内容；调用 lab_str，安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
    assert(!strcmp(lab_str(first,"idempotencyKey"),lab_str(retry,"idempotencyKey")));
    //【讲解 L216】调用 lab_agent_option，把一个候选方案的日期、小时、设备拼成下拉框摘要，并在写入时检查剩余容量；按下标借用数组元素；调用 lab_agent_proposals，借用最近响应中的候选方案数组。
    char option[512];lab_agent_option(cJSON_GetArrayItem(lab_agent_proposals(&agent),0),1,option,sizeof(option));assert(strstr(option,"2026-11-06")&&strstr(option,"#04"));
    //【讲解 L217】释放 JSON 对象及其子节点。
    cJSON_Delete(first);cJSON_Delete(retry);
    //【讲解 L218】按本行初值、条件和步长循环；调用 lab_agent_accept，拒绝其他账号的迟到响应，记录对话并最多保留 20 条消息。
    for(int k=0;k<20;k++)lab_agent_accept(&agent,"user-a","another",reply);
    //【讲解 L219】断言条件必须成立，失败立即暴露测试错误；读取数组元素数。
    assert(cJSON_GetArraySize(agent.history)==20);
    //【讲解 L220】调用 lab_agent_bind，把助手状态绑定到账号；断言条件必须成立，失败立即暴露测试错误；读取数组元素数。
    lab_agent_bind(&agent,"user-b");assert(!*agent.session&&!agent.response&&cJSON_GetArraySize(agent.history)==0);
    //【讲解 L221】调用 lab_agent_accept，拒绝其他账号的迟到响应，记录对话并最多保留 20 条消息；断言条件必须成立，失败立即暴露测试错误；读取数组元素数。
    lab_agent_accept(&agent,"user-a","late",reply);assert(!agent.response&&cJSON_GetArraySize(agent.history)==0);
    //【讲解 L222】调用 lab_agent_accept，拒绝其他账号的迟到响应，记录对话并最多保留 20 条消息。
    lab_agent_accept(&agent,"user-b","new",reply);
    //【讲解 L223】解析 JSON 文本。
    cJSON *confirmed=cJSON_Parse("{\"outcome\":\"confirmed\",\"message\":\"done\",\"proposals\":[{\"proposalId\":\"old\"}]}");
    //【讲解 L224】调用 lab_agent_accept，拒绝其他账号的迟到响应，记录对话并最多保留 20 条消息；断言条件必须成立，失败立即暴露测试错误；调用 lab_agent_confirmation，根据所选方案构造确认体。
    lab_agent_accept(&agent,"user-b","",confirmed);assert(!lab_agent_confirmation(&agent,0,"next-key"));
    //【讲解 L225】释放 JSON 对象及其子节点；调用 lab_agent_clear，释放助手历史及最近响应并清空状态。
    cJSON_Delete(confirmed);cJSON_Delete(reply);lab_agent_clear(&agent);
    //【讲解 L226】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    LabClient previewClient={0};LabModel previewModel={0};char display[160],prefs[256];
    //【讲解 L227】调用 ui_preview_model，生成离线截图用示例用户、设备和预约，不读取生产数据库。
    ui_preview_model(&previewClient,&previewModel);
    //【讲解 L228】调用 ui_time，将云端时间转换为北京时间文本；断言条件必须成立，失败立即暴露测试错误；比较字符串内容。
    ui_time("2026-11-04T18:02:00Z",display,sizeof(display));assert(!strcmp(display,"2026-11-05 02:02"));
    //【讲解 L229】断言条件必须成立，失败立即暴露测试错误；调用 ui_timestamp，解析云端 ISO 时间文本和时区偏移，转换为 Unix 毫秒。
    assert(ui_timestamp("2026-11-04T09:02:00+08:00")==ui_timestamp("2026-11-04T01:02:00.000+00:00"));
    //【讲解 L230】调用 ui_duration，有签到和有效签退时显示分钟差，否则显示进行中或占位符；调用 ui_usage，按 reservation_id 查找对应的签到签退记录，返回模型内部借用指针；按下标借用数组元素。
    ui_duration(ui_usage(&previewModel,cJSON_GetArrayItem(previewModel.bookings,0)),display,sizeof(display));assert(!strcmp(display,"50 分钟"));
    //【讲解 L231】调用 ui_summary，根据当前用户非取消预约统计当日记录、常用设备及时段；断言条件必须成立，失败立即暴露测试错误；查找子字符串。
    ui_summary(&previewModel,&previewClient.session,"2026-11-04",display,sizeof(display),prefs,sizeof(prefs));assert(strstr(display,"1 个预约"));
    //【讲解 L232】断言条件必须成立，失败立即暴露测试错误；比较字符串内容；调用 ui_page_title，按页面编号返回统一中文标题。
    assert(!strcmp(ui_page_title(4),"智能助手"));assert(!strcmp(ui_status("confirmed"),"已预约"));
    //【讲解 L233】调用 lab_model_free，释放模型持有的五组 JSON，然后清零结构体；调用 lab_session_clear，用 volatile 字节写入清空会话，降低清理被优化器省略的风险。
    lab_model_free(&previewModel);lab_session_clear(&previewClient.session);
    //【讲解 L234】调用 model_and_signup，验证分页模型、账号资料和注册相关请求。
    model_and_signup();
    //【讲解 L235】调用 auth，验证学号解析兼容分支和登录认证请求。
    auth();
    //【讲解 L236】调用 refresh，验证 401 后刷新令牌和重试原请求，避免无限递归。
    refresh();
    //【讲解 L237】调用 attendance，验证北京时间边界、签到签退动作及当前用户归属。
    attendance();
    //【讲解 L238】调用 partial_booking，模拟跨小时预约中途失败，确认已成功小时数不会被误报为全部成功。
    partial_booking();
    //【讲解 L239】断言条件必须成立，失败立即暴露测试错误；调用 lab_semver_newer，只比较三段纯数字版本号。
    assert(lab_semver_newer("3.0.1", "3.0.0"));
    //【讲解 L240】断言条件必须成立，失败立即暴露测试错误；调用 lab_semver_newer，只比较三段纯数字版本号。
    assert(lab_semver_newer("4.0.0", "3.9.9"));
    //【讲解 L241】断言条件必须成立，失败立即暴露测试错误；调用 lab_semver_newer，只比较三段纯数字版本号。
    assert(!lab_semver_newer("-1.0.0", "3.0.0"));
    //【讲解 L242】断言条件必须成立，失败立即暴露测试错误；调用 lab_semver_newer，只比较三段纯数字版本号。
    assert(!lab_semver_newer("3.0.0-beta", "3.0.0"));
    //【讲解 L243】断言条件必须成立，失败立即暴露测试错误；调用 lab_semver_newer，只比较三段纯数字版本号。
    assert(!lab_semver_newer("3.0.0", "3.0.0"));
    //【讲解 L244】断言条件必须成立，失败立即暴露测试错误；调用 lab_int，读取 JSON 整数，先检查数值类型和 int 范围。
    assert(lab_int(NULL, "x") == 0);
    //【讲解 L245】声明或更新 refresh（刷新令牌）；按本行表达式保存对应状态。
    puts("Native C tests passed: authentication, refresh, CST boundaries, ownership, attendance RPCs, "
         //【讲解 L246】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
         "partial booking and versions.");
    //【讲解 L247】结束当前函数并返回 0；具体成功或失败含义见函数说明。
    return 0;
//【讲解 L248】结束当前代码块。
}

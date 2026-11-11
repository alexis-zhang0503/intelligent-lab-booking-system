//【答辩注释】共享接口声明：会话、数据模型、HTTP 传输回调和业务函数。先看此文件，再阅读 client.c。JSON 返回值通常由调用者释放；const 指针通常是借用，不要重复释放。
//【讲解 L1】按 LAB_NATIVE_CLIENT_H 是否已定义选择编译分支，避免重复定义或适配平台。
#ifndef LAB_NATIVE_CLIENT_H
//【讲解 L2】定义 LAB_NATIVE_CLIENT_H，用于编译配置或头文件保护。
#define LAB_NATIVE_CLIENT_H
//【讲解 L3】引入 ../vendor/cJSON.h 的类型和函数声明。
#include "../vendor/cJSON.h"
//【讲解 L4】引入 stddef.h 的类型和函数声明。
#include <stddef.h>
//【讲解 L5】引入 stdio.h 的类型和函数声明。
#include <stdio.h>
//【讲解 L6】引入 stdint.h 的类型和函数声明。
#include <stdint.h>
//【讲解 L7】定义 LAB_NATIVE_VERSION，当前值为 "3.1.0"。
#define LAB_NATIVE_VERSION "3.1.0"
//【讲解 L8】定义 LAB_API_HOST，当前值为 "ikfqcappastopzjbncea.supabase.co"。
#define LAB_API_HOST "ikfqcappastopzjbncea.supabase.co"
//【讲解 L9】定义 LAB_PUBLIC_KEY，当前值为 "sb_publishable_5Q5xIYHsAeEfO195kJ7jNg_Z5uBwxkV"。
#define LAB_PUBLIC_KEY "sb_publishable_5Q5xIYHsAeEfO195kJ7jNg_Z5uBwxkV"
//【讲解 L10】定义 LAB_SITE_HOST，当前值为 "alexis-zhang0503.github.io"。
#define LAB_SITE_HOST "alexis-zhang0503.github.io"
//【讲解 L11】定义 LAB_SITE_BASE，当前值为 "/intelligent-lab-booking-system/"。
#define LAB_SITE_BASE "/intelligent-lab-booking-system/"
//【讲解 L12】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef char *(*LabTransport)(void *, const char *, const char *, const char *, const char *, const char *,
                              //【讲解 L13】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
                              const char *, int *, char *, size_t);
//【讲解 L14】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef struct {
    //【讲解 L15】声明或更新 access（访问令牌）、refresh（刷新令牌）、uid（用户唯一标识）、role（角色）；按本行表达式保存对应状态。
    char access[8192], refresh[4096], uid[64], name[128], login[128], role[16];
//【讲解 L16】结束当前代码块或类型声明。
} LabSession;
//【讲解 L17】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef struct {
    //【讲解 L18】声明或更新 devices（设备列表）、bookings（预约列表）、profiles（用户资料）、issues（故障列表）；按本行表达式保存对应状态。
    cJSON *devices, *bookings, *profiles, *issues, *usage;
//【讲解 L19】结束当前代码块或类型声明。
} LabModel;
//【讲解 L20】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef struct {
    //【讲解 L21】声明或更新 transport（HTTP 传输回调）；按本行表达式保存对应状态。
    LabTransport transport;
    //【讲解 L22】声明或更新 context（回调上下文）；按本行表达式保存对应状态。
    void *context;
    //【讲解 L23】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    LabSession session;
    //【讲解 L24】声明或更新 error（错误文本）；按本行表达式保存对应状态。
    char error[1024];
    //【讲解 L25】声明或更新 status（状态码或业务状态）；按本行表达式保存对应状态。
    int status;
//【讲解 L26】结束当前代码块或类型声明。
} LabClient;
//【讲解 L27】函数 lab_str：安全读取 JSON 字符串：缺失或类型不匹配时返回空字符串，返回的是 JSON 内部借用指针。
const char *lab_str(const cJSON *, const char *);
//【讲解 L28】函数 lab_int：读取 JSON 整数，先检查数值类型和 int 范围；缺失或越界返回 0。
int lab_int(const cJSON *, const char *);
//【讲解 L29】函数 lab_model_free：释放模型持有的五组 JSON，然后清零结构体；调用后旧记录指针不再有效。
void lab_model_free(LabModel *);
//【讲解 L30】函数 lab_session_clear：用 volatile 字节写入清空会话，降低清理被优化器省略的风险；并不等同于磁盘加密。
void lab_session_clear(LabSession *);
//【讲解 L31】函数 lab_request：对外提供统一请求入口，开启一次令牌刷新重试。
cJSON *lab_request(LabClient *, const char *, const char *, const cJSON *);
//【讲解 L32】函数 lab_rpc：检查 RPC 名称只包含允许的字符，再构造 /rest/v1/rpc/ 路径并发送 POST。
cJSON *lab_rpc(LabClient *, const char *, const cJSON *);
//【讲解 L33】函数 lab_login：支持邮箱或学号登录；学号先解析为认证邮箱，再向 Auth 获取会话。角色稍后从 profiles 读取。
int lab_login(LabClient *, const char *, const char *);
//【讲解 L34】函数 lab_register：组织学生或带邀请码的注册参数，调用云端注册流程；管理员资格由服务端验证。
int lab_register(LabClient *, const char *, const char *, const char *, const char *);
//【讲解 L35】函数 lab_load_model：先加载账号资料并确认 active，再加载设备、预约、故障及使用记录；失败释放临时模型。usage 加载失败降级为空。
int lab_load_model(LabClient *, LabModel *);
//【讲解 L36】函数 lab_booking_start：校验日期、闰年和 08–16 点起始小时；把北京时间预约开始转换为 Unix 毫秒，无效返回 -1。
int64_t lab_booking_start(const cJSON *);
//【讲解 L37】函数 lab_attendance_action：根据状态和时间给出可执行动作：0 不可操作，1 签到，2 签退。in_use 可进入签退分支，最终规则仍由云端校验。
int lab_attendance_action(const cJSON *, int64_t);
//【讲解 L38】函数 lab_attend：校验预约归属及动作，再调用 v2_check_in 或 v2_check_out；本地检查不是最终授权。
int lab_attend(LabClient *, const cJSON *, int, int64_t);
//【讲解 L39】函数 lab_reserve_range：按小时提交多个设备，每小时使用一个幂等键。失败立即停止，committed 记录已成功的小时数；整个时间范围不是一个事务。
int lab_reserve_range(LabClient *, const char *, int, int, const int *, size_t, const char *keys[9], int *);
//【讲解 L40】函数 lab_semver_newer：只比较三段纯数字版本号；不支持 beta 等预发布标签。
int lab_semver_newer(const char *, const char *);

/* Shared contracts used by both native frontends; caller owns returned JSON. */
//【讲解 L43】函数 lab_agent_payload：构造与网页版一致的 V3 助手请求字段；依据当前角色选择助手类型，对历史和确认对象做深复制。
cJSON *lab_agent_payload(const LabSession *, const char *message, const char *date, int start, int end, const int *devices, size_t count, const char *request_id, const char *session_id, const cJSON *history, const cJSON *confirmation);
//【讲解 L44】函数 lab_usage_metrics：调用 v3_admin_usage_metrics，device=0 表示全部设备；校验关键时长字段存在，返回值由调用者释放。
cJSON *lab_usage_metrics(LabClient *, const char *start_date, const char *end_date, int device);
//【讲解 L45】函数 lab_booking_is_live：只有状态为 in_use 且当前时间在该小时内才显示正在使用，避免历史状态误亮。
int lab_booking_is_live(const cJSON *, int64_t now);

//【讲解 L47】声明共享的数据类型；字段的容量、所有权和用途见本结构体内各行。
typedef struct { char owner[64], session[80], proposal[80], confirmation_key[80]; cJSON *history, *response; } LabAgentState;
//【讲解 L48】函数 lab_agent_clear：释放助手历史及最近响应并清空状态；当前实现只在内存中保存会话。
void lab_agent_clear(LabAgentState *);
//【讲解 L49】函数 lab_agent_bind：把助手状态绑定到账号；账号变化时清空旧历史，避免跨账号显示。
void lab_agent_bind(LabAgentState *, const char *owner);
//【讲解 L50】函数 lab_agent_accept：拒绝其他账号的迟到响应，记录对话并最多保留 20 条消息；确认成功后清除待确认标记。
void lab_agent_accept(LabAgentState *, const char *owner, const char *message, const cJSON *response);
//【讲解 L51】函数 lab_agent_confirmation：根据所选方案构造确认体；同一方案重试复用原幂等键，换方案才更新键。
cJSON *lab_agent_confirmation(LabAgentState *, int option, const char *new_key);
//【讲解 L52】函数 lab_agent_proposals：借用最近响应中的候选方案数组；若 outcome 已 confirmed 则不再返回旧方案。
const cJSON *lab_agent_proposals(const LabAgentState *);
//【讲解 L53】函数 lab_agent_transcript：把内存中的对话拼成有容量限制的中文显示文本；没有历史时显示引导语。
void lab_agent_transcript(const LabAgentState *, char *out, size_t cap);
//【讲解 L54】函数 lab_agent_option：把一个候选方案的日期、小时、设备拼成下拉框摘要，并在写入时检查剩余容量。
void lab_agent_option(const cJSON *, int number, char *out, size_t cap);
//【讲解 L55】函数 lab_export_csv：以 UTF-8 BOM 输出当前已加载模型中的全部预约或故障；包含历史及取消记录，实际可见范围取决于云端权限。
int lab_export_csv(const LabModel *,int issues,FILE *);
//【讲解 L56】函数 lab_metrics_report：将云端统计结果转换为总览、逐设备和逐时段报表；不在客户端重算完整统计口径。
void lab_metrics_report(const cJSON *,char *,size_t);
//【讲解 L57】结束当前条件编译区域。
#endif

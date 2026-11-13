/*
 * 2_booking.c —— 【第 2 人】预约与签到的业务规则
 *
 * 一句话：手动预约怎么提交、签到签退怎么判断。
 *
 * ============================== 讲解提纲（按顺序讲） ==============================
 * 1. lab_reserve_range：提交预约
 *    - 三层检查：时间在 8~17 点之间且开始早于结束；设备号 1~40；同一台设备不能选两次（双重循环）。
 *    - 按小时拆开，每小时调用一次云端 v2_create_reservations，一次请求里包含所有选中的设备。
 *    - 每小时带一个"防重复编号"（GUID，由界面在 other/main.c 生成）：网络超时后再点一次，
 *      云端看到同一个编号只处理一次，不会约出两份。
 *    - committed 记录已经成功了几个小时：中途某小时冲突就停下，前面成功的保留，并告诉用户。
 * 2. lab_booking_start：把"2026-11-05 + 14 点"换成 Unix 毫秒，方便和当前时间比较大小
 *    - 同时负责数据校验：格式 YYYY-MM-DD、月份 1~12、闰年 2 月 29 天、开始小时 8~16。
 *    - 北京时间 h 点 = UTC (h-8) 点，所以全程不依赖电脑的时区设置。
 * 3. lab_attendance_action：现在能做什么？0 不能操作 / 1 可以签到 / 2 可以签退
 *    - in_use（已签到）→ 只能签退；reserved / confirmed 且现在在预约的那一小时内 → 可以签到。
 * 4. lab_attend：再核对一遍（是不是我的预约、现在还能不能做），然后调用云端 v2_check_in / v2_check_out。
 *
 * 和别人的衔接：请求都通过第 1 人的 lab_rpc 发出；第 3 人的 AI 确认方案时，也沿用同样的"防重复编号"思路。
 * 界面里的对应位置：在 other/main.c 里搜索【答辩·第2人】（生成 9 个防重复编号、签到前的本地检查）。
 *
 * ============================== 可能被问到的问题 ==============================
 * 问：两个人同时约同一台设备的同一个小时，会怎样？
 * 答：冲突检查在云端数据库里做，只有一个人能约上；另一个人的请求返回错误，界面提示失败原因。
 * 问：约 3 个小时，第 2 个小时冲突了怎么办？
 * 答：第 1 个小时已经成功、会保留；程序立即停止，并提示"已确认成功 1 个小时"，让用户刷新核实。
 * 问：客户端已经检查过能不能签到，为什么云端还要再检查？
 * 答：客户端的检查只是为了及早给出友好提示，可以被绕过；真正是否允许由云端决定。
 * 问：为什么时间要换成毫秒，而不是直接比较字符串？
 * 答：签到判断要算"开始时刻 ≤ 现在 < 开始时刻 + 1 小时"，换成同一种数字才能直接比较大小。
 */
#include "client.h"   // 共用的数据结构和函数声明
#include <ctype.h>    // isdigit：检查日期里的字符
#include <stdio.h>    // snprintf / sscanf
#include <string.h>   // strlen / strcmp

// 日期换算成天数的算法写在文件后半部分（补充代码），先在这里声明
static int64_t civil_days(int y, unsigned m, unsigned d);

/* ################################ 第一部分：核心代码（答辩讲这些） ################################ */

/* ---------- 1. 提交预约 ----------
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

/* ---------- 2. 时间换算 ----------
 * 功能：判断"现在能不能签到"、"设备是不是正在使用"，都要把预约的日期和小时变成一个可以比较大小的数字。
 *       统一用 Unix 毫秒（从 1970-01-01 00:00 UTC 起经过的毫秒数），全程不依赖电脑的时区设置，
 *       所以即使电脑时区设错了，签到判断也照样按北京时间来。
 */

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

/* ---------- 3. 签到签退 ----------
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


/* ######################## 第二部分：补充代码（答辩不讲，被问到时再翻到这里） ######################## */

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

// 设备按钮是否显示"使用中"：状态是 in_use，并且现在正处在这条预约的那一小时内。
// 两个条件都要：如果用户忘了签退，状态会一直是 in_use，只看状态就会让设备一直显示"使用中"
int lab_booking_is_live(const cJSON *booking,int64_t now) {
    int64_t start=lab_booking_start(booking);
    return !strcmp(lab_str(booking,"status"),"in_use") && start>0 && now>=start && now<start+3600000;
}
